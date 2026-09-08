#include "QuiverJson.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
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
    for (const auto value : root["levels"].toArray())
        document.quiver->AddLevel(name(value, "Level name").toStdString());
    const auto& levels = document.quiver->GetLevels();
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
    return document;
}
