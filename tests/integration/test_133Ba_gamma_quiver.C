#include "COINAlgebra/Builders/DecayQuiverBuilder.h"
#include "COINAlgebra/Algebra/PathAlgebra.h"
#include "COINAlgebra/Algebra/PathProjectors.h"
#include "COINAlgebra/Probability/DecayProbability.h"
#include "COINAlgebra/Detection/DetectionMaps.h"

#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cmath>
#include <stdexcept>

void test_133Ba_gamma_quiver()
{
    // Detector setup: whole-array total efficiency, conditional on gamma emission.
    const double totalEfficiency = 0.7;
    const std::size_t detectorCount = 64; // Set to the number of detectors in your array.

    std::cout
        << "============================================================"
        << std::endl;

    std::cout
        << "             133Ba -> EC -> 133Cs decay quiver"
        << std::endl;

    std::cout
        << "============================================================"
        << std::endl;


    // ========================================================
    // Read radioactive decay data for the PARENT nucleus
    //
    // 133Ba:
    //     Z = 56
    //     A = 133
    // ========================================================

    RadioactiveDecayReader radioactiveReader(
        "RadioactiveDecay5.5"
    );

    RadioactiveIsotope ba133 =
        radioactiveReader.read(56, 133);


    // ========================================================
    // Read photon evaporation data for the DAUGHTER nucleus
    //
    // 133Cs:
    //     Z = 55
    //     A = 133
    // ========================================================

    PhotonEvaporationReader photonReader(
        "PhotonEvaporation5.5"
    );

    PhotonIsotope cs133 =
        photonReader.read(55, 133);


    // ========================================================
    // Print radioactive decay information
    // ========================================================

    std::cout
        << std::endl
        << "133Ba radioactive decay data"
        << std::endl;

    std::cout
        << "--------------------------------------------"
        << std::endl;

    std::cout
        << "Atomic number  = "
        << ba133.atomicNumber()
        << std::endl;

    std::cout
        << "Mass number    = "
        << ba133.massNumber()
        << std::endl;

    std::cout
        << "Parent states  = "
        << ba133.numberOfParentStates()
        << std::endl;


    // ========================================================
    // Print PhotonEvaporation information
    // ========================================================

    std::cout
        << std::endl
        << "133Cs PhotonEvaporation data"
        << std::endl;

    std::cout
        << "--------------------------------------------"
        << std::endl;

    std::cout
        << "Atomic number  = "
        << cs133.atomicNumber()
        << std::endl;

    std::cout
        << "Mass number    = "
        << cs133.massNumber()
        << std::endl;

    std::cout
        << "Number of levels = "
        << cs133.numberOfLevels()
        << std::endl;


    // ========================================================
    // Build the gamma-decay quiver
    //
    // "EC" means physical electron capture, so the builder
    // combines:
    //
    //     KshellEC
    //     LshellEC
    //     MshellEC
    //
    // from the Ba-133 radioactive-decay data.
    //
    // The resulting daughter levels and gamma transitions
    // come from Cs-133 PhotonEvaporation data.
    // ========================================================

    std::unordered_map<std::string, double> conversionCoefficients;
    DecayQuiver quiver =
        DecayQuiverBuilder::BuildGammaQuiver(
            ba133,
            cs133,
            conversionCoefficients,
            "EC"
        );


    if (conversionCoefficients.size() != quiver.GetTransitions().size() ||
        std::abs(conversionCoefficients.at("gamma_81_to_0_0") - 1.703) > 1e-12 ||
        std::abs(conversionCoefficients.at("gamma_437_to_384_0") - 5.66) > 1e-12)
        throw std::runtime_error("Incorrect extracted Ba-133 conversion coefficients");

    // ========================================================
    // Print resulting quiver
    // ========================================================

    std::cout
        << std::endl
        << "Resulting gamma-decay quiver:"
        << std::endl
        << std::endl;

    quiver.Print();


    // ========================================================
    // Summary
    // ========================================================

    std::cout
        << std::endl
        << "--------------------------------------------"
        << std::endl;

    std::cout
        << "Number of levels      = "
        << quiver.GetLevels().size()
        << std::endl;

    std::cout
        << "Number of transitions = "
        << quiver.GetTransitions().size()
        << std::endl;

    std::cout
        << "============================================================"
        << std::endl;

    // Probabilities:

    DecayVector decay_vector;
    // Builder names use six decimal places and the case-sensitive suffix keV.
    const auto requireLevel = [&quiver](const std::string& name) {
        DecayLevel* level = quiver.GetLevel(name);
        if(level == nullptr)
        {
            throw std::runtime_error("Missing quiver level: " + name);
        }
        return level;
    };
    DecayLevel* e0 = requireLevel("level_0.000000keV");
    DecayLevel* e1 = requireLevel("level_80.997900keV");
    DecayLevel* e2 = requireLevel("level_160.612100keV");
    DecayLevel* e3 = requireLevel("level_383.849100keV");
    DecayLevel* e4 = requireLevel("level_437.011300keV");

    DecayTransition* g10 = quiver.GetTransition("gamma_81_to_0_0");
    DecayTransition* g21 = quiver.GetTransition("gamma_161_to_81_0");
    DecayTransition* g20 = quiver.GetTransition("gamma_161_to_0_1");
    DecayTransition* g30 = quiver.GetTransition("gamma_384_to_0_2");
    DecayTransition* g31 = quiver.GetTransition("gamma_384_to_81_1");
    DecayTransition* g41 = quiver.GetTransition("gamma_437_to_81_2");
    DecayTransition* g32 = quiver.GetTransition("gamma_384_to_161_0");
    DecayTransition* g42 = quiver.GetTransition("gamma_437_to_161_1");
    DecayTransition* g43 = quiver.GetTransition("gamma_437_to_384_0");

    DecayPath p0(e0);
    DecayPath p1(e1);
    DecayPath p2(e2);
    DecayPath p3(e3);
    DecayPath p4(e4);


       // --------------------------------------------------------
    // Stationary components
    // --------------------------------------------------------

    decay_vector.AddTerm(
        p0,
        0.0 //e0.GetProbability()
    );

    decay_vector.AddTerm(
        p1,
        0.0 //e1.GetProbability()
    );

    decay_vector.AddTerm(
        p2,
        0.0 // e2.GetProbability()
    );

    decay_vector.AddTerm(
        p3,
        0.145 //e3.GetProbability()
    );

    decay_vector.AddTerm(
        p4,
        0.855 //e4.GetProbability()
    );

     // --------------------------------------------------------
    // Length-one components
    // --------------------------------------------------------

    for(const auto* transition :
        quiver.GetTransitions())
    {
        DecayPath path(*transition);

        decay_vector.AddTerm(
            path,
            path.GetProbability()
        );
    }
    PathAlgebra algebra(quiver);
    PathProjectors projectors;
    DecayProbability pb(algebra,projectors);
    DecayVector feedingVector = pb.FeedingVector(decay_vector);
    std::cout << "Total transition feeding (gamma + IC):\n";
    feedingVector.PrintTable();
    std::cout << "Gamma emission feeding:\n";
    pb.EmissionFeedingVector(decay_vector, conversionCoefficients).PrintTable();

    std::vector<double> initialPopulations(quiver.GetLevels().size(), 0.0);
    for(const auto& term:decay_vector.GetTerms()) if(term.path.IsStationary())
        for(std::size_t i=0;i<quiver.GetLevels().size();++i)
            if(term.path.GetSource()==quiver.GetLevels()[i]) initialPopulations[i]+=term.coefficient;
    quiver.ExportJson("examples/133Ba_gamma_quiver.json",
                      {decay_vector}, "133Ba -> EC -> 133Cs",
                      conversionCoefficients, initialPopulations);

    std::unordered_map<std::string, double> efficiencies;
    efficiencies["gamma_81_to_0_0"] = 0.455954;
    efficiencies["gamma_161_to_81_0"] = 0.455949;
    efficiencies["gamma_161_to_0_1"] = 0.40154;
    efficiencies["gamma_384_to_0_2"] = 0.239605;
    efficiencies["gamma_384_to_81_1"] = 0.283232;
    efficiencies["gamma_437_to_81_2"] = 0.253294;
    efficiencies["gamma_384_to_161_0"] = 0.34334284;
    efficiencies["gamma_437_to_161_1"] = 0.30135745;
    efficiencies["gamma_437_to_384_0"] = 0.44011142;

    DecayVector feedingEfficiencyVector = pb.DetectionFeedingVector(decay_vector, efficiencies, conversionCoefficients);
    std::cout << "Gamma detection feeding (including IC):\n";
    feedingEfficiencyVector.PrintTable();
    for (const auto& term : feedingVector.GetTerms())
    {
        const double expected = term.coefficient *
            efficiencies.at(term.path.GetTransitions().front().GetName()) /
            (1.0 + conversionCoefficients.at(term.path.GetTransitions().front().GetName()));
        const double actual = algebra.PathForm(
            feedingEfficiencyVector, DecayVector(term.path, 1.0));
        if (std::abs(actual - expected) > 1e-12)
            throw std::runtime_error("Incorrect detected feeding probability");
    }
    std::unordered_map<std::string, double> totalEfficiencies;
    for (const auto* transition : quiver.GetTransitions())
        totalEfficiencies[transition->GetName()] = totalEfficiency;

    {
        std::cout << "Summing setup: " << detectorCount
                  << " detector(s), uniform whole-array total efficiency = "
                  << totalEfficiency << '\n';
        DetectionMaps maps(efficiencies, totalEfficiencies, detectorCount, conversionCoefficients);
        const auto summed = pb.SummingFeedingVector(decay_vector, maps);
        std::cout << "Connected summing contributions (separate paths):\n";
        summed.PrintTable();
        DecayVector peaks;
        for (const auto* transition : quiver.GetTransitions()) {
            DecayPath path(*transition);
            peaks.AddTerm(path, algebra.PathForm(summed, DecayVector(path)));
        }
        std::cout << "Connected summing-corrected peaks (equivalent endpoints combined):\n";
        peaks.PrintTable();

    }

    // ========================================================
    // Coincidence vectors: physical, emitted, detected, summed
    // ========================================================
    // The fiber contains physical gamma+IC branches. Applying hit maps to
    // observed factors must not also suppress the unobserved connections.
    CAlgebra coins(quiver, {e4,e3,e2,e1,e0}, decay_vector);
    DecayVector branch, tau;
    for (const auto& term : decay_vector.GetTerms()) {
        if (term.path.IsStationary()) branch.AddTerm(term.path,term.coefficient);
        else tau.AddTerm(term.path,term.coefficient);
    }
    const DecayVector terminal(p0);
    DetectionMaps maps(efficiencies,totalEfficiencies,detectorCount,conversionCoefficients);
    DetectionMaps::EfficiencyMap unity;
    for (const auto* edge : quiver.GetTransitions()) unity[edge->GetName()]=1.0;
    // Unit conditional efficiency leaves only the gamma-emission fraction.
    DetectionMaps emissionMap(unity,unity,detectorCount,conversionCoefficients);

    // Transition-identity gate projector: retain terms containing ALL requested
    // arrows, without changing or renormalizing their coefficients. This is the
    // single-arrow form of G1/G2 (Eqs. 65/67), retaining parallel-arrow identity.
    // On summed vectors this is photon membership, NOT a resolved-energy gate.
    const auto gate = [](const CAlgebra::Vector& vector,
                         const std::vector<DecayTransition*>& required) {
        CAlgebra::Vector result;
        for (const auto& term : vector) {
            const auto& factors=term.coin.GetFactors();
            const bool keep=std::all_of(required.begin(),required.end(),[&](const auto* edge) {
                if (!edge) throw std::runtime_error("Missing coincidence gate transition");
                return std::find(factors.begin(),factors.end(),DecayPath(*edge))!=factors.end();
            });
            if (keep) result.push_back(term);
        }
        return result;
    };
    const auto sum = [](const CAlgebra::Vector& vector) {
        double result=0;
        for (const auto& term : vector) result+=term.coefficient;
        return result;
    };
    const auto print = [&](const std::string& title,const CAlgebra::Vector& vector) {
        std::cout << "\n" << title << "\ncoefficient per parent decay | degree | coincidence basis\n";
        if (vector.empty()) std::cout << "0 (zero vector)\n";
        for (const auto& term : vector)
            std::cout << std::setprecision(12) << term.coefficient << " | "
                      << term.coin.Degree() << " | " << term.coin.ToString() << '\n';
        std::cout << "Coefficient sum = " << sum(vector)
                  << " (expected count; not generally an exclusive probability)\n";
    };
    std::cout << "\nCOINCIDENCE RESPONSE: illustrative existing peak efficiencies, total="
              << totalEfficiency << ", N=" << detectorCount << ". IC included.\n"
              << "All nonzero orders are printed; forbidden combinations have coefficient zero.\n"
              << "Stationary markers retain initial populations; no factorial or reverse-order duplication.\n";
    CAlgebra::Vector pairs;
    for (std::size_t k=2;k<quiver.GetLevels().size();++k) {
        auto physical=coins.Multiply(coins.Embed(branch),coins.Power(coins.Embed(tau),k));
        if (physical.empty()) break;
        if (k==2) pairs=physical;
        const auto emitted=emissionMap.FullEnergyHit(physical);
        const auto detected=maps.FullEnergyHit(physical);
        const auto label=std::to_string(k)+"-transition ";
        print(label+"physical coincidences (gamma + IC)",physical);
        print(label+"gamma emission coincidences",emitted);
        print(label+"independent full-energy detections (no summing corrections)",detected);
        print(label+"emission, G1: contains 80.9979-keV gamma",gate(emitted,{g10}));
        print(label+"detection, G1: contains 80.9979-keV gamma",gate(detected,{g10}));
    }

    // Eq. (70): one detector's peaks, including disconnected summed-in photons.
    CAlgebra out(quiver,{e4,e3,e2,e1,e0},maps.SummingOut(tau));
    const auto spectrum=out.Multiply(out.Multiply(out.Embed(branch),
        maps.SummingInExpansion(out,tau)),out.Embed(terminal));
    print("Summing-corrected spectrum: degree 1 singles; degree >=2 same-detector sums",spectrum);
    print("Summed spectrum, G1 photon membership: includes 80.9979-keV gamma (not an 81-keV peak gate)",
          gate(spectrum,{g10}));
    print("Summed spectrum, G2 photon membership: includes 356.0134- and 80.9979-keV gammas",
          gate(spectrum,{g41,g10}));

    // Resolved two-detector peaks: all other radiation must avoid BOTH detectors.
    // This does not implement general gated sums of photon groups (see CA-002).
    CAlgebra::Vector cleanPairs;
    if (detectorCount>=2) {
        CAlgebra twoOut(quiver,{e4,e3,e2,e1,e0},maps.AvoidDetectors(tau,2));
        cleanPairs=twoOut.Multiply(twoOut.Multiply(twoOut.Embed(branch),
            twoOut.Power(twoOut.Embed(maps.FullEnergyHit(tau)),2)),twoOut.Embed(terminal));
        for (auto& term : cleanPairs)
            term.coefficient*=double(detectorCount-1)/double(detectorCount);
    }
    print("Two distinct clean full-energy peaks, including summing-out (no summed-in groups)",cleanPairs);
    print("Clean pairs, G1: 80.9979-keV peak",gate(cleanPairs,{g10}));
    print("Clean pairs, G2: 356.0134-keV AND 80.9979-keV peaks",gate(cleanPairs,{g41,g10}));

    // Independent checks against the existing ordered path-probability API
    // and explicit response factors for this adjacent two-transition pair.
    const double physicalPair=sum(gate(pairs,{g41,g10}));
    const double pathPair=pb.CoincidenceProbability(decay_vector,DecayPath(*g41),DecayPath(*g10));
    const double gammaPair=physicalPair/(1+conversionCoefficients.at(g41->GetName()))/
                                          (1+conversionCoefficients.at(g10->GetName()));
    const double detectedPair=gammaPair*efficiencies.at(g41->GetName())*efficiencies.at(g10->GetName());
    const auto check = [](double actual,double expected) {
        if (std::abs(actual-expected)>1e-12)
            throw std::runtime_error("Incorrect Ba-133 coincidence vector or gate");
    };
    check(physicalPair,pathPair);
    check(sum(gate(emissionMap.FullEnergyHit(pairs),{g41,g10})),gammaPair);
    check(sum(gate(maps.FullEnergyHit(pairs),{g41,g10})),detectedPair);
    // This pair spans the full cascade from the populated top level to ground:
    // there are no hidden photons, so only distinct-detector acceptance remains.
    check(sum(gate(cleanPairs,{g41,g10})),detectedPair*double(detectorCount-1)/double(detectorCount));
    check(sum(gate(spectrum,{g41,g10})),detectedPair/double(detectorCount));
    check(sum(gate(gate(pairs,{g10}),{g10})),sum(gate(pairs,{g10})));
    check(sum(gate(pairs,{g41,g43})),0.0); // mutually exclusive outgoing branches
    std::cout << "PASS: Ba-133 coincidence response, clean/summed pair and gate checks\n";

}
