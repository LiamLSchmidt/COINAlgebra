// Photon-only energy-window quivers; see examples/coulomb_excitation/README.md.
#include "COINAlgebra/NuclearData/PhotonEvaporationReader.h"
#include "QuiverJson.h"
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <cmath>
#include <iostream>
#include <map>
#include <stdexcept>

int main() {
    try {
        PhotonEvaporationReader reader("PhotonEvaporation5.5");
        struct Selection { const char* isotope; int z, a; double top; };
        const Selection selections[] = {
            {"106Cd",48,106,2973.33}, {"108Cd",48,108,2993.11},
            {"110Cd",48,110,2984.46}, {"112Cd",48,112,2980.85},
            {"114Cd",48,114,2941.27}, {"116Cd",48,116,2782.6},
            {"90Zr",40,90,3308.8}, {"92Zr",40,92,2398.36},
            {"94Zr",40,94,2366.12}, {"96Zr",40,96,2857.373},
            {"98Zr",40,98,2276.9}, {"100Zr",40,100,1414.62}
        };
        const QString directory = "examples/coulomb_excitation/";
        if (!QDir().mkpath(directory)) throw std::runtime_error("Cannot create directory");
        for (const auto& s : selections) {
            const auto isotope = reader.read(s.z,s.a);
            QJsonArray levels, transitions, levelData, transitionData, terminals, omitted;
            std::map<int,int> indices;
            std::map<int,QString> names;
            const PhotonLevel* top = nullptr;
            for (const auto& l : isotope.levels()) {
                if (l.energy_keV > s.top + 1e-8) continue;
                // Floating excitation energies are not absolute energies.
                if (l.floating != "-") { omitted.append(l.id); continue; }
                const auto name = QString("%1:L%2_%3keV").arg(s.isotope).arg(l.id)
                    .arg(l.energy_keV,0,'f',6);
                indices[l.id] = levels.size(); names[l.id] = name; levels.append(name);
                levelData.append(QJsonObject{{"name",name},{"photon_level_id",l.id},
                    {"energy_keV",l.energy_keV},{"jpi_raw",l.jpi},{"half_life_s",l.halfLife_s}});
                if (std::abs(l.energy_keV-s.top)<1e-8) top = &l;
            }
            if (!top || (top->jpi != 2 && top->jpi != 4))
                throw std::runtime_error("Upper state must match a known 2+ or 4+ level");
            for (const auto& l : isotope.levels()) {
                if (!indices.count(l.id)) continue;
                double total = 0;
                for (const auto& g : l.transitions) {
                    if (g.relativeIntensity <= 0 || g.multipolarity == 1) continue;
                    if (!indices.count(g.daughterLevel))
                        throw std::runtime_error("Positive gamma branch leaves selected absolute-energy window");
                    if (isotope.level(g.daughterLevel).energy_keV >= l.energy_keV)
                        throw std::runtime_error("Non-descending gamma edge");
                    total += g.relativeIntensity;
                }
                if (total == 0) {
                    if (l.energy_keV > 0) terminals.append(QJsonObject{{"name",names.at(l.id)},
                        {"reason",l.transitions.empty() ? "No transitions tabulated" : "No positive-intensity non-E0 gamma branch tabulated"}});
                    continue;
                }
                for (std::size_t i=0; i<l.transitions.size(); ++i) {
                    const auto& g = l.transitions[i];
                    if (g.relativeIntensity <= 0 || g.multipolarity == 1) continue;
                    const auto name = QString("%1:gamma_L%2_to_L%3_%4_%5keV")
                        .arg(s.isotope).arg(l.id).arg(g.daughterLevel).arg(static_cast<int>(i))
                        .arg(g.energy_keV,0,'f',4);
                    transitions.append(QJsonObject{{"name",name},{"source_index",indices.at(l.id)},
                        {"target_index",indices.at(g.daughterLevel)}, {"probability",g.relativeIntensity/total}});
                    transitionData.append(QJsonObject{{"name",name},{"energy_keV",g.energy_keV},
                        {"relative_intensity",g.relativeIntensity},{"multipolarity_code",g.multipolarity},
                        {"mixing_ratio",g.mixingRatio},{"conversion_coefficient",g.conversionCoefficient}});
                }
            }
            QJsonObject root{{"levels",levels},{"transitions",transitions},{"vectors",QJsonArray{}},
                {"metadata",QJsonObject{{"isotope",s.isotope},{"Z",s.z},{"A",s.a},
                    {"photon_data","PhotonEvaporation5.5"},
                    {"selection","All absolute-energy levels at or below upper state; not only descendants of upper state"},
                    {"maximum_energy_keV",s.top},{"upper_level",names.at(top->id)},
                    {"upper_jpi_raw",top->jpi},{"level_data",levelData},{"transition_data",transitionData},
                    {"excited_gamma_terminal_levels",terminals},{"omitted_floating_level_ids",omitted},
                    {"probability_semantics","Positive non-E0 relative gamma intensity / outgoing sum; conditional gamma branching, not Coulomb-excitation populations or absolute photon yields"},
                    {"limitations","No beta/EC feeding, E0 edges, conversion competition, excitation cross sections, timing cuts, or inferred missing branches. Excited gamma terminals are not necessarily stable."}}}};
            const auto bytes = QJsonDocument(root).toJson();
            auto document = Studio::readJson(bytes);
            if (document.quiver->GetLevels().size()!=static_cast<std::size_t>(levels.size()) ||
                document.quiver->GetTransitions().size()!=static_cast<std::size_t>(transitions.size()) || transitions.empty())
                throw std::runtime_error("Studio import count mismatch or empty quiver");
            for (const auto* l : document.quiver->GetLevels()) {
                const double p = document.quiver->GetOutgoingProbability(l);
                if (p!=0 && std::abs(p-1)>1e-12) throw std::runtime_error("Unnormalized level");
            }
            QFile file(directory + s.isotope + "_coulex_gamma_quiver.json");
            if (!file.open(QIODevice::WriteOnly) || file.write(bytes)!=bytes.size())
                throw std::runtime_error("Could not write output");
            std::cout << s.isotope << " | " << s.top << " | " << top->jpi << "+ | "
                << levels.size() << " | " << transitions.size() << " | " << terminals.size() << '\n';
        }
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
