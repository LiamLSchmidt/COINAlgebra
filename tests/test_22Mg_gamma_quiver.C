#include "COINAlgebra/DecayQuiverBuilder.h"

#include <iostream>


void test_22Mg_gamma_quiver()
{
    std::cout << "\n";
    std::cout
        << "============================================================\n";
    std::cout
        << "        22Mg -> beta+ -> 22Na -> gamma decay quiver\n";
    std::cout
        << "============================================================\n\n";


    // ------------------------------------------------------------------------
    // Read the nuclear data
    // ------------------------------------------------------------------------

    RadioactiveDecayReader radioactiveReader(
        "RadioactiveDecay5.5"
    );

    PhotonEvaporationReader photonReader(
        "PhotonEvaporation5.5"
    );


    RadioactiveIsotope mg22 =
        radioactiveReader.read(12, 22);

    PhotonIsotope na22 =
        photonReader.read(11, 22);


    // ------------------------------------------------------------------------
    // Build the gamma-decay quiver following beta+ decay
    // ------------------------------------------------------------------------

    DecayQuiver quiver =
        DecayQuiverBuilder::BuildGammaQuiver(
            mg22,
            na22,
            "BetaPlus"
        );


    // ------------------------------------------------------------------------
    // Print the resulting quiver
    // ------------------------------------------------------------------------

    std::cout
        << "Resulting gamma-decay quiver:\n\n";

    quiver.Print();


    std::cout << "\n";

    std::cout
        << "Number of levels      = "
        << quiver.GetLevels().size()
        << "\n";

    std::cout
        << "Number of transitions = "
        << quiver.GetTransitions().size()
        << "\n";


    std::cout << "\n";
}