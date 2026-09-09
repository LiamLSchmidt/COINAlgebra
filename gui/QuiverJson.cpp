#include "QuiverJson.h"
#include "StudioModel.h"
#include "ViewState.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QRegularExpression>
#include <cmath>
#include <stdexcept>

namespace {
void require(bool condition, const QString& message) {
    if (!condition) throw std::runtime_error(message.toStdString());
}
QString name(const QJsonValue& value, const QString& context) {
    require(value.isString() && !value.toString().trimmed().isEmpty(),
            context + " must be a non-empty string.");
    return value.toString();
}
double number(const QJsonValue& value, const QString& context) {
    require(value.isDouble() && std::isfinite(value.toDouble()),
            context + " must be a finite number.");
    return value.toDouble();
}
int index(const QJsonValue& value, int size, const QString& context) {
    const double n = number(value, context);
    require(n >= 0 && n < size && std::floor(n) == n,
            context + " must reference an existing level by integer index.");
    return static_cast<int>(n);
}
}

Studio::Document Studio::readJson(const QByteArray& bytes) {
    QJsonParseError error;
    const auto json = QJsonDocument::fromJson(bytes, &error);
    require(error.error == QJsonParseError::NoError,
            "Invalid JSON: " + error.errorString());
    require(json.isObject(), "The JSON document must be an object.");
    const auto root = json.object();
    require(root["levels"].isArray(), "'levels' must be an array of names.");
    require(root["transitions"].isArray(), "'transitions' must be an array.");
    Document document{std::make_unique<DecayQuiver>(), {}};
    require(!root.contains("metadata") || root["metadata"].isObject(), "Metadata must be an object.");
    document.metadata = root["metadata"].toObject();
    require(!root.contains("title") || root["title"].isString(), "Title must be a string.");
    document.title = root["title"].toString();
    for (const auto value : root["levels"].toArray())
        document.quiver->AddLevel(name(value, "Level name").toStdString());
    const auto& levels = document.quiver->GetLevels();
    // Canonical energies are aligned with the names array. Explicit null remains
    // unknown, even if a display label happens to contain a numeric energy.
    require(!root.contains("level_energies_keV") || root["level_energies_keV"].isArray(),
            "level_energies_keV must be an array aligned with levels.");
    const auto energies=root["level_energies_keV"].toArray();
    require(!root.contains("level_energies_keV") || energies.size()==int(levels.size()),
            "Provide one energy (or null) per level.");
    const auto legacyValue=document.metadata.value("level_energies_keV");
    require(legacyValue.isUndefined() || legacyValue.isObject(),"Legacy level energies must be an object.");
    const auto legacy=legacyValue.toObject();
    for(auto it=legacy.begin();it!=legacy.end();++it) {
        bool ok; int i=it.key().toInt(&ok);
        require(ok && i>=0 && i<int(levels.size()),"Legacy energy references an unknown level.");
        require(number(it.value(),"Level energy")>=0,"Level energies must be nonnegative.");
    }
    for(int i=0;i<int(levels.size());++i) {
        if(root.contains("level_energies_keV")) {
            if(!energies[i].isNull()) levels[i]->SetEnergy(number(energies[i],"Level energy"));
        } else if(legacy.contains(QString::number(i))) {
            levels[i]->SetEnergy(number(legacy[QString::number(i)],"Level energy"));
        } else {
            const auto match=QRegularExpression("(?:level_)?([0-9]+(?:\\.[0-9]+)?)keV$").match(QString::fromStdString(levels[i]->GetName()));
            if(match.hasMatch()) levels[i]->SetEnergy(match.captured(1).toDouble());
        }
    }
    document.metadata.remove("level_energies_keV");
    for (const auto value : root["transitions"].toArray()) {
        require(value.isObject(), "Each transition must be an object.");
        const auto t = value.toObject();
        const auto label = name(t["name"], "Transition name");
        const int source = index(t["source_index"], levels.size(), label + ": source_index");
        const int target = index(t["target_index"], levels.size(), label + ": target_index");
        const double probability = number(t["probability"], label + ": probability");
        require(probability >= 0 && probability <= 1, label + ": probability must be between 0 and 1.");
        document.quiver->AddTransition(label.toStdString(), levels[source], levels[target], probability);
    }
    require(!root.contains("vectors") || root["vectors"].isArray(), "'vectors' must be an array if provided.");
    for (const auto value : root["vectors"].toArray()) {
        require(value.isObject() && value.toObject()["terms"].isArray(),
                "Each vector must contain a 'terms' array.");
        DecayVector vector;
        for (const auto termValue : value.toObject()["terms"].toArray()) {
            require(termValue.isObject(), "Each vector term must be an object.");
            const auto term = termValue.toObject();
            const double coefficient = number(term["coefficient"], "Vector coefficient");
            require(term["path_names"].isArray(), "Vector 'path_names' must be an array.");
            std::vector<DecayTransition> transitions;
            for (const auto pathName : term["path_names"].toArray()) {
                const auto label = name(pathName, "Path transition name");
                const auto* t = document.quiver->GetTransition(label.toStdString());
                require(t != nullptr, "Unknown path transition: " + label);
                transitions.push_back(*t);
            }
            DecayPath path;
            if (transitions.empty()) {
                const auto label = name(term["stationary_level"],
                    "An empty path_names array requires stationary_level");
                auto* level = document.quiver->GetLevel(label.toStdString());
                require(level != nullptr, "Unknown stationary level: " + label);
                path = DecayPath(level);
            } else {
                require(!term.contains("stationary_level"),
                        "A non-stationary path cannot specify stationary_level.");
                path = DecayPath(transitions); // Also validates composability.
            }
            // Display text and path_probability are derived export metadata.
            // Recompute probability from the imported transitions.
            vector.AddTerm(path, coefficient);
        }
        document.vectors.push_back(std::move(vector));
    }
    validateStudio(*document.quiver, document.metadata);
    document.metadata=normalizeViewMetadata(document.metadata);
    return document;
}

DecayVector Studio::createDecayVector(const DecayQuiver& quiver, const QJsonObject& metadata) {
    require(!quiver.GetLevels().empty(), "Add or import levels first.");
    DecayVector result;
    for (const auto* t : quiver.GetTransitions())
        result.AddTerm(DecayPath(std::vector<DecayTransition>{*t}), t->GetProbability());
    if (metadata.contains("branching")) {
        return result + branchingVector(quiver, metadata);
    } else if (metadata.contains("feeding_modes")) {
        require(metadata["feeding_modes"].isArray() && !metadata["feeding_modes"].toArray().empty(),
                "Source feeding metadata is missing or invalid.");
        const double tolerance = metadata.value("energy_tolerance_keV").toDouble(1.0);
        require(std::isfinite(tolerance) && tolerance >= 0, "Invalid feeding energy tolerance.");
        double population = 0;
        for (const auto modeValue : metadata["feeding_modes"].toArray()) {
            const auto mode = modeValue.toObject();
            const auto daughter = name(mode["daughter"], "Feeding daughter");
            require(mode["channels"].isArray(), "Feeding channels must be an array.");
            for (const auto channelValue : mode["channels"].toArray()) {
                const auto channel = channelValue.toObject();
                const double energy = number(channel["daughter_energy_keV"], "Feeding energy");
                const double coefficient = number(channel["raw_branching_percentage"], "Feeding percentage") / 100.;
                require(coefficient >= 0 && coefficient <= 1, "Invalid feeding percentage.");
                DecayLevel* best = nullptr;
                double distance = tolerance;
                for (auto* level : quiver.GetLevels()) {
                    const auto label = QString::fromStdString(level->GetName());
                    if (!label.startsWith(daughter + ":") || !level->HasEnergy()) continue;
                    const double e = level->GetEnergy();
                    if (std::abs(e-energy) <= distance) { best = level; distance = std::abs(e-energy); }
                }
                require(best != nullptr, "Cannot match feeding level; restore its imported name or reimport the source.");
                result.AddTerm(DecayPath(best), coefficient);
                population += coefficient;
            }
        }
        require(population > 0, "Source metadata has no positive feeding population.");
    } else {
        require(!metadata.contains("source"), "Source decay requires feeding metadata; reimport the original source file.");
        auto* top = quiver.GetLevels().back();
        if (metadata.contains("upper_level")) {
            top = quiver.GetLevel(metadata["upper_level"].toString().toStdString());
            require(top != nullptr, "The imported upper level was renamed or removed. Reimport the scheme to restore its population.");
        }
        result.AddTerm(DecayPath(top), 1.0);
    }
    return result;
}
