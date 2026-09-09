#include "QuiverJson.h"
#include "StudioModel.h"
#include <QJsonDocument>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QTemporaryDir>
#include <QJsonObject>
#include <QJsonArray>
#include <cassert>
#include <cmath>
#include <algorithm>
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
    assert(Studio::readJson(R"({"levels":[],"transitions":[]})").title.isEmpty());
    assert(Studio::readJson(R"({"title":"56Fe levels","levels":[],"transitions":[]})").title == "56Fe levels");

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
    QJsonObject explicitMetadata{{"branching",QJsonArray{0.,.25,.75}},
        {"level_energies_keV",QJsonObject{{"0",0.},{"1",100.},{"2",200.}}}};
    auto explicitDecay=Studio::createDecayVector(*source.quiver,explicitMetadata);
    assert(std::abs(probability.FeedingProbability(explicitDecay,low)-.46875)<1e-12);
    assert(Studio::branchingVector(*source.quiver,explicitMetadata).Size()==2);
    assert(Studio::transitionVector(*source.quiver).Size()==2);
    explicitMetadata["efficiency_samples"]=Studio::readEfficiencyCsv("energy_keV,efficiency\n0,0.2\n200,0.6\n");
    auto efficiencies=Studio::efficiencyMap(*source.quiver,explicitMetadata);
    assert(std::abs(efficiencies.at("lo")-.4)<1e-12);
    auto detected=probability.DetectionFeedingVector(explicitDecay,efficiencies);
    assert(std::abs(algebra.PathForm(detected,DecayVector(low,1))-.1875)<1e-12);
    for(auto csv:{"0,0.1\n0,0.2", "0,1.2\n200,0.5", "0,nan\n200,0.5", "energy_keV,efficiency\n"}) {
        bool threw=false; try { Studio::readEfficiencyCsv(csv); } catch(const std::exception&) { threw=true; } assert(threw);
    }
    QFile griffin(QDir(QFileInfo(QString::fromUtf8(__FILE__)).absolutePath()).filePath("../../data/GRIFFIN_Eff.csv"));
    assert(griffin.open(QIODevice::ReadOnly));
    auto griffinSamples=Studio::readEfficiencyCsv(griffin.readAll());
    assert(griffinSamples.size()==1991);
    auto griffinMetadata=explicitMetadata;griffinMetadata["efficiency_samples"]=griffinSamples;
    griffinMetadata["level_energies_keV"]=QJsonObject{{"0",0.},{"1",100.},{"2",200.5}};
    source.quiver->GetLevels()[2]->SetEnergy(200.5);
    const auto griffinMap=Studio::efficiencyMap(*source.quiver,griffinMetadata);
    constexpr double at100=.45137141695282584,at101=.45088721304743273;
    assert(std::abs(griffinMap.at("lo")-at100)<1e-14);
    assert(std::abs(griffinMap.at("hi")-(at100+at101)/2)<1e-14);
    auto griffinDetection=probability.DetectionFeedingVector(explicitDecay,griffinMap);
    assert(std::abs(algebra.PathForm(griffinDetection,DecayVector(low,1))-.46875*at100)<1e-12);
    auto outOfRange=griffinMetadata;outOfRange["level_energies_keV"]=QJsonObject{{"0",0.},{"1",9.},{"2",200.5}};
    source.quiver->GetLevels()[1]->SetEnergy(9.);
    bool rangeRejected=false;try{Studio::efficiencyMap(*source.quiver,outOfRange);}catch(const std::exception& e){rangeRejected=std::string(e.what()).find("lo")!=std::string::npos;}assert(rangeRejected);
    source.quiver->GetLevels()[1]->SetEnergy(100.);
    assert(Studio::readEfficiencyCsv("\xef\xbb\xbf\"Energy[keV]\", \"HPGe\"\r\n10, 0.1\r\n11, 0.2\r\n").size()==2);
    auto bad=explicitMetadata; bad["branching"]=QJsonArray{0.,.2,.2};
    bool threw=false; try { Studio::branchingVector(*source.quiver,bad); } catch(const std::exception&) { threw=true; } assert(threw);
    root=QJsonDocument::fromJson(fixture).object(); root["metadata"]=explicitMetadata;
    auto restored=Studio::readJson(QJsonDocument(root).toJson());
    explicitMetadata.remove("level_energies_keV");
    assert(restored.metadata==explicitMetadata);
    assert(restored.quiver->GetLevels()[1]->GetEnergy()==100.);
    restored.quiver->GetLevels()[1]->SetName("renamed without energy");
    assert(Studio::levelEnergy(*restored.quiver,restored.metadata,1)==100.);
    auto unknownEnergy=Studio::readJson(R"({"levels":["level_100keV"],"level_energies_keV":[null],"transitions":[]})");
    assert(!unknownEnergy.quiver->GetLevels()[0]->HasEnergy());
    for(auto energy: {QJsonArray{-1},QJsonArray{"100"},QJsonArray{},QJsonArray{1,2}}) {
        auto badRoot=QJsonObject{{"levels",QJsonArray{"a"}},{"transitions",QJsonArray{}},{"level_energies_keV",energy}};
        check(badRoot);
    }
    QTemporaryDir exportedDir;assert(exportedDir.isValid());
    DecayQuiver native;native.AddLevel("unmeasured");native.AddLevel("ground",0.);native.AddLevel("excited",123.456789123);
    const auto fileName=exportedDir.filePath("energy.json");native.ExportJson(fileName.toStdString());QFile nativeJson(fileName);assert(nativeJson.open(QIODevice::ReadOnly));
    auto roundTrip=Studio::readJson(nativeJson.readAll());
    assert(!roundTrip.quiver->GetLevels()[0]->HasEnergy());assert(roundTrip.quiver->GetLevels()[1]->GetEnergy()==0.);
    assert(roundTrip.quiver->GetLevels()[2]->GetEnergy()==123.456789123);
    // The actual Ba-133 document must reproduce physical propagation followed
    // by gamma survival, rather than silently populating only the top level.
    QFile baFile(QDir(QFileInfo(QString::fromUtf8(__FILE__)).absolutePath()).filePath("../../examples/133Ba_gamma_quiver.json"));
    assert(baFile.open(QIODevice::ReadOnly));auto baBytes=baFile.readAll();auto ba=Studio::readJson(baBytes);
    const double expectedAlpha[]={1.703,1.77,.294,.0975,.0434,.0202,5.66,.0566,.0254};
    const auto alpha=Studio::conversionMap(*ba.quiver,ba.metadata);assert(alpha.size()==9);
    for(int i=0;i<9;++i)assert(std::abs(alpha.at(ba.quiver->GetTransitions()[i]->GetName())-expectedAlpha[i])<1e-14);
    auto baDecay=Studio::createDecayVector(*ba.quiver,ba.metadata);
    PathAlgebra baAlgebra(*ba.quiver);PathProjectors baProjectors;DecayProbability baProbability(baAlgebra,baProjectors);
    ba.metadata["efficiency_samples"]=griffinSamples;auto baEfficiency=Studio::efficiencyMap(*ba.quiver,ba.metadata);
    const auto baEmission=baProbability.EmissionFeedingVector(baDecay,alpha);
    const auto baDetection=baProbability.DetectionFeedingVector(baDecay,baEfficiency,alpha);
    std::vector<double> population{0,0,0,.145,.855};
    const auto& baLevels=ba.quiver->GetLevels();
    for(int level=4;level>=0;--level)for(auto* t:ba.quiver->GetTransitions())if(t->GetSource()==baLevels[level]) {
        const double physical=population[level]*t->GetProbability();
        int target=std::find(baLevels.begin(),baLevels.end(),t->GetTarget())-baLevels.begin();population[target]+=physical;
        const double emitted=physical/(1+alpha.at(t->GetName()));DecayVector observed{DecayPath(*t)};
        assert(std::abs(baAlgebra.PathForm(baEmission,observed)-emitted)<1e-12);
        assert(std::abs(baAlgebra.PathForm(baDetection,observed)-emitted*baEfficiency.at(t->GetName()))<1e-12);
    }
    auto badBa=QJsonDocument::fromJson(baBytes).object();auto badMetadata=badBa["metadata"].toObject();auto incomplete=badMetadata["conversion_coefficients"].toObject();incomplete.remove("gamma_81_to_0_0");badMetadata["conversion_coefficients"]=incomplete;badBa["metadata"]=badMetadata;check(badBa);
    // Exporting again retains both initial populations and the IC map.
    const auto baExport=exportedDir.filePath("ba.json");ba.quiver->ExportJson(baExport.toStdString(),{baDecay},"Ba",alpha,{0,0,0,.145,.855});
    QFile savedBa(baExport);assert(savedBa.open(QIODevice::ReadOnly));auto baAgain=Studio::readJson(savedBa.readAll());
    assert(Studio::conversionMap(*baAgain.quiver,baAgain.metadata)==alpha);
    assert(baAgain.metadata["branching"].toArray()==QJsonArray({0,0,0,.145,.855}));
    std::cout << "PASS: JSON quivers, vector ownership, long/stationary paths, invalid documents\n";
}
