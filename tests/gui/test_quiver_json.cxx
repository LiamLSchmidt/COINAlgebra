#include "QuiverJson.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <cassert>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <sstream>
#include "COINAlgebra/Algebra/PathAlgebra.h"
#include "COINAlgebra/Algebra/PathProjectors.h"
#include "COINAlgebra/Probability/DecayProbability.h"

int main() {
    const QByteArray fixture = R"({
      "levels":["d2","d1","d0"],
      "transitions":[
        {"name":"a","source_index":0,"target_index":1,"probability":0.4},
        {"name":"b","source_index":1,"target_index":2,"probability":0.5}],
      "vectors":[{"terms":[
        {"path_names":["a","b"],"coefficient":2.5,"path_probability":999},
        {"path_names":[],"stationary_level":"d1","coefficient":-0.3}]}]
    })";
    auto document = Studio::readJson(fixture);
    assert(document.quiver->GetLevels().size() == 3);
    assert(document.quiver->GetTransitions().size() == 2);
    assert(document.vectors.size() == 1);
    const auto& terms = document.vectors[0].GetTerms();
    assert(terms.size() == 2);
    assert(terms[0].path.Length() == 2);
    assert(terms[0].path.GetSource() == document.quiver->GetLevel("d2"));
    assert(terms[0].path.GetTarget() == document.quiver->GetLevel("d0"));
    assert(terms[0].coefficient == 2.5);
    assert(std::abs(terms[0].path.GetProbability() - 0.2) < 1e-12);
    assert(terms[1].path.IsStationary());
    assert(terms[1].path.GetSource() == document.quiver->GetLevel("d1"));
    assert(terms[1].coefficient == -0.3);
    assert(Studio::readJson(R"({"levels":[],"transitions":[]})").vectors.empty());

    const auto reject = [](const QByteArray& json) {
        bool threw = false;
        try { auto unused = Studio::readJson(json); }
        catch (const std::exception&) { threw = true; }
        assert(threw);
    };
    for (const auto& invalid : {"{", "[]", "{}", "{\"levels\":[],\"transitions\":{},\"vectors\":[]}"})
        reject(invalid);
    const auto original = QJsonDocument::fromJson(fixture).object();
    const auto check = [&](const QJsonObject& root) { reject(QJsonDocument(root).toJson()); };
    auto root = original;
    root["levels"] = QJsonArray{"d2", "d2"}; check(root);
    root = original; root["levels"] = QJsonArray{5}; check(root);
    for (const QJsonValue& probability : {QJsonValue(-0.1), QJsonValue(1.1), QJsonValue("0.5"), QJsonValue()}) {
        root = original;
        auto transitions = root["transitions"].toArray();
        auto t = transitions[0].toObject(); t["probability"] = probability;
        transitions[0] = t; root["transitions"] = transitions; check(root);
    }
    for (const auto badIndex : {-1.0, 0.5, 3.0}) {
        root = original;
        auto transitions = root["transitions"].toArray();
        auto t = transitions[0].toObject(); t["source_index"] = badIndex;
        transitions[0] = t; root["transitions"] = transitions; check(root);
    }
    root = original;
    auto transitions = root["transitions"].toArray();
    transitions.append(transitions[0]); root["transitions"] = transitions; check(root);
    for (const auto paths : {QJsonArray{"missing"}, QJsonArray{"b", "a"}, QJsonArray{}}) {
        root = original;
        root["vectors"] = QJsonArray{QJsonObject{{"terms", QJsonArray{
            QJsonObject{{"coefficient", 1}, {"path_names", paths}}}}}};
        check(root);
    }
    root = original; root["vectors"] = "invalid"; check(root);
    auto source = Studio::readJson(R"({
      "levels":["D:level_0.000000keV","D:level_100.000000keV","D:level_200.000000keV"],
      "transitions":[{"name":"hi","source_index":2,"target_index":1,"probability":0.5},
                     {"name":"lo","source_index":1,"target_index":0,"probability":0.75}],
      "metadata":{"feeding_modes":[
        {"daughter":"D","total_branching_fraction":0.6,"channels":[{"daughter_energy_keV":200,"raw_branching_percentage":60}]},
        {"daughter":"D","total_branching_fraction":0.4,"channels":[{"daughter_energy_keV":200,"raw_branching_percentage":20},{"daughter_energy_keV":100,"raw_branching_percentage":20}]}]}
    })");
    auto decay = Studio::createDecayVector(*source.quiver, source.metadata);
    assert(decay.Size() == 4); // Shared feeding merged, not multiplied by mode fractions.
    PathAlgebra algebra(*source.quiver);
    PathProjectors projectors;
    DecayProbability probability(algebra, projectors);
    DecayPath low(std::vector<DecayTransition>{*source.quiver->GetTransition("lo")});
    assert(std::abs(probability.FeedingProbability(decay, low) - 0.45) < 1e-12);
    const auto branch = projectors.BranchingProjector(decay);
    const auto transitionPart = decay - branch;
    const auto reference = algebra.Multiply(projectors.TargetVertexProjector(
        algebra.Multiply(branch, algebra.PowerExpand(transitionPart, algebra.MaxPower()))), transitionPart);
    const auto feeding = probability.FeedingVector(decay);
    assert(std::abs(algebra.PathForm(reference, DecayVector(low, 1)) -
                    algebra.PathForm(feeding, DecayVector(low, 1))) < 1e-12);
    auto coulex = Studio::createDecayVector(*source.quiver, QJsonObject{});
    assert(std::abs(probability.FeedingProbability(coulex, low) - 0.375) < 1e-12);
    std::ostringstream table;
    coulex.PrintTable(table);
    assert(table.str().find("Probability") != std::string::npos);
    assert(table.str().find("0.50000000") != std::string::npos);
    std::cout << "PASS: JSON quivers, vector ownership, long/stationary paths, invalid documents\n";
}
