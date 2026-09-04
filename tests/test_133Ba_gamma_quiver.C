#include "COINAlgebra/DecayQuiverBuilder.h"

#include <iostream>

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
}
