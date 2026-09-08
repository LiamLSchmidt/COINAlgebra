// Diverse nuclear-structure gamma windows; see examples/nuclear_structure/README.md.
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
        struct Selection { const char* isotope; int z, a; double top; double jpi; const char* phenomenon; };
        const Selection selections[] = {
            {"12C",6,12,7654.2,0,"Hoyle-state radiative cascade; alpha competition excluded"},
            {"16O",8,16,7116.85,-1,"Light closed-shell nucleus: E1/E2/E3 branches and E0-only endpoint"},
            {"24Mg",12,24,6432.3,0,"Light deformed nucleus: rotational and interband cascades"},
            {"27Al",13,27,3004.2,4.5,"Odd-mass sd-shell structure and half-integer spins"},
            {"48Ca",20,48,4506.78,-3,"Doubly magic nucleus: sparse high-energy low-lying spectrum"},
            {"94Mo",42,94,3128.66,1,"Mixed-symmetry and scissors-mode context; M1/E2 branching"},
            {"150Nd",60,150,1598.5,10,"Shape-transition/X(5)-candidate context and coexisting bands"},
            {"166Er",68,166,1786.975,-6,"Deformed rare-earth rotational and interband cascades"},
            {"178Hf",72,178,2446.09,16,"High-K 16+ isomer and lower cascades; no timing simulation"},
            {"208Pb",82,208,4085.52,2,"Doubly magic nucleus with collective octupole 3- state"}
        };
        const QString directory = "examples/nuclear_structure/";
        if (!QDir().mkpath(directory)) throw std::runtime_error("Cannot create directory");
        for (const auto& s : selections) {
            const auto isotope = reader.read(s.z,s.a);
            QJsonArray levels, transitions, levelData, transitionData, terminals, omitted, omittedBranches;
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
            if (!top || top->jpi != s.jpi)
                throw std::runtime_error("Upper state energy and spin must match the selected photon level");
            for (const auto& l : isotope.levels()) {
                if (!indices.count(l.id)) continue;
                double total = 0;
                for (const auto& g : l.transitions) {
                    if (g.relativeIntensity <= 0 || g.multipolarity == 1) {
                        omittedBranches.append(QJsonObject{{"source_level_id",l.id},
                            {"target_level_id",g.daughterLevel},{"energy_keV",g.energy_keV},
                            {"relative_intensity",g.relativeIntensity},{"multipolarity_code",g.multipolarity},
                            {"reason",g.multipolarity == 1 ? "E0 is not single-photon emission" : "Nonpositive tabulated gamma intensity"}});
                        continue;
                    }
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
                {"metadata",QJsonObject{{"isotope",s.isotope},{"Z",s.z},{"A",s.a},{"phenomenon",s.phenomenon},
                    {"photon_data","PhotonEvaporation5.5"},
                    {"selection","All absolute-energy levels at or below upper state; not only descendants of upper state"},
                    {"maximum_energy_keV",s.top},{"upper_level",names.at(top->id)},
                    {"upper_jpi_raw",top->jpi},{"level_data",levelData},{"transition_data",transitionData},
                    {"excited_gamma_terminal_levels",terminals},{"omitted_floating_level_ids",omitted},
                    {"omitted_branches",omittedBranches},
                    {"probability_semantics","Positive non-E0 relative gamma intensity / outgoing sum; conditional gamma branching, not Coulomb-excitation populations or absolute photon yields"},
                    {"limitations","No beta/EC feeding, particle decay, E0 edges, conversion competition, excitation cross sections, timing cuts, or inferred missing branches. Excited gamma terminals are not necessarily stable."}}}};
            const auto bytes = QJsonDocument(root).toJson();
            auto document = Studio::readJson(bytes);
            if (document.quiver->GetLevels().size()!=static_cast<std::size_t>(levels.size()) ||
                document.quiver->GetTransitions().size()!=static_cast<std::size_t>(transitions.size()) || transitions.empty())
                throw std::runtime_error("Studio import count mismatch or empty quiver");
            for (const auto* l : document.quiver->GetLevels()) {
                const double p = document.quiver->GetOutgoingProbability(l);
                if (p!=0 && std::abs(p-1)>1e-12) throw std::runtime_error("Unnormalized level");
            }
            QFile file(directory + s.isotope + "_structure_gamma_quiver.json");
            if (!file.open(QIODevice::WriteOnly) || file.write(bytes)!=bytes.size())
                throw std::runtime_error("Could not write output");
            std::cout << s.isotope << " | " << s.top << " | " << top->jpi << " (raw Jpi) | "
                << levels.size() << " | " << transitions.size() << " | " << terminals.size() << '\n';
        }
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
