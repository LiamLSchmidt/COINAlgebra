#include "COINAlgebra/Builders/DecayQuiverBuilder.h"
#include "COINAlgebra/Algebra/PathAlgebra.h"
#include "COINAlgebra/Algebra/PathProjectors.h"
#include "COINAlgebra/Probability/DecayProbability.h"

#include <iostream>
#include <stdexcept>

void test_133Ba_gamma_quiver()
{
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

    DecayQuiver quiver =
        DecayQuiverBuilder::BuildGammaQuiver(
            ba133,
            cs133,
            "EC"
        );


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
    feedingVector.PrintTable();
}
