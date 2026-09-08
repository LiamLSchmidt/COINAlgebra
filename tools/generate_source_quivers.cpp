// Run from the repository root; see examples/sources/README.md.
#include "COINAlgebra/Builders/DecayQuiverBuilder.h"
#include "QuiverJson.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QDir>
#include <cmath>
#include <iostream>
#include <map>
#include <stdexcept>

int main() {
    try {
        RadioactiveDecayReader radioactive("RadioactiveDecay5.5");
        PhotonEvaporationReader photon("PhotonEvaporation5.5");
        struct Source { const char* name; int z, a; const char* lower; const char* upper; };
        const Source sources[] = {
            {"22Na",11,22,"22Ne","22Mg"}, {"54Mn",25,54,"54Cr","54Fe"},
            {"56Co",27,56,"56Fe","56Ni"}, {"57Co",27,57,"57Fe","57Ni"},
            {"60Co",27,60,"60Fe","60Ni"}, {"65Zn",30,65,"65Cu","65Ga"},
            {"88Y",39,88,"88Sr","88Zr"}, {"137Cs",55,137,"137Xe","137Ba"},
            {"152Eu",63,152,"152Sm","152Gd"}, {"154Eu",63,154,"154Sm","154Gd"}
        };
        if (!QDir().mkpath("examples/sources")) throw std::runtime_error("Cannot create output directory");
        for (const auto& s : sources) {
            auto parent = radioactive.read(s.z, s.a);
            if (parent.parentStates().front().energy_keV != 0)
                throw std::runtime_error("Expected parent ground state");
            QJsonArray levels, transitions, modes;
            std::map<QString,int> indices;
            std::map<QString,QJsonObject> edges;
            for (const auto& mode : parent.parentStates().front().decayModes) {
                const auto& type = mode.decayType;
                const bool minus = type == "BetaMinus";
                if (!minus && type != "BetaPlus" && type != "KshellEC" &&
                    type != "LshellEC" && type != "MshellEC") continue;
                if (mode.channels.empty() || mode.totalBranchingFraction <= 0) continue;
                const QString daughter = minus ? s.upper : s.lower;
                const int z = s.z + (minus ? 1 : -1);
                auto daughterData = photon.read(z, s.a);
                auto quiver = DecayQuiverBuilder::BuildGammaQuiver(parent, daughterData, type);
                QJsonArray channels;
                for (const auto& c : mode.channels)
                    channels.append(QJsonObject{{"daughter_energy_keV",c.daughterEnergy_keV},
                        {"raw_branching_percentage",c.branchingPercentage}});
                modes.append(QJsonObject{{"daughter",daughter},{"decay_type",QString::fromStdString(type)},
                    {"total_branching_fraction",mode.totalBranchingFraction},{"channels",channels}});
                auto label = [&](const std::string& n) { return daughter + ":" + QString::fromStdString(n); };
                for (const auto* l : quiver.GetLevels()) {
                    const auto n = label(l->GetName());
                    if (!indices.count(n)) { indices[n] = levels.size(); levels.append(n); }
                }
                for (const auto* t : quiver.GetTransitions()) {
                    const auto n = label(t->GetName());
                    QJsonObject edge{{"name",n},{"source_index",indices.at(label(t->GetSource()->GetName()))},
                        {"target_index",indices.at(label(t->GetTarget()->GetName()))},
                        {"probability",t->GetProbability()}};
                    if (edges.count(n) && edges.at(n) != edge)
                        throw std::runtime_error("Inconsistent shared gamma transition");
                    edges[n] = edge;
                }
            }
            for (const auto& edge : edges) transitions.append(edge.second);
            QJsonObject root{{"levels",levels},{"transitions",transitions},{"vectors",QJsonArray{}},
                {"metadata",QJsonObject{{"source",s.name},{"parent_Z",s.z},{"parent_A",s.a},
                    {"parent_energy_keV",0},{"energy_tolerance_keV",1.0},
                    {"radioactive_data","RadioactiveDecay5.5"},{"photon_data","PhotonEvaporation5.5"},
                    {"builder","DecayQuiverBuilder::BuildGammaQuiver"},
                    {"probability_semantics","Conditional gamma branch: relativeIntensity / sum of positive outgoing relativeIntensity; not absolute source photon yield."},
                    {"feeding_modes",modes}}}};
            const auto bytes = QJsonDocument(root).toJson();
            // Exercise the actual Studio importer before writing any artifact.
            auto imported = Studio::readJson(bytes);
            if (imported.quiver->GetTransitions().empty()) throw std::runtime_error("Empty gamma quiver");
            for (const auto* l : imported.quiver->GetLevels()) {
                const double p = imported.quiver->GetOutgoingProbability(l);
                if (p != 0 && std::abs(p-1) > 1e-12) throw std::runtime_error("Unnormalized level");
            }
            QFile output(QString("examples/sources/") + s.name + "_gamma_quiver.json");
            if (!output.open(QIODevice::WriteOnly) || output.write(bytes) != bytes.size())
                throw std::runtime_error("Could not write quiver");
            std::cout << s.name << ": " << levels.size() << " levels, " << transitions.size() << " transitions\n";
        }
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
