#include "COINAlgebra/DecayQuiverBuilder.h"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

// Build and validate a gamma-decay quiver from Geant4 radioactive-decay
// and photon-evaporation files.
bool test_reader_gamma_quiver(
    int parentZ,
    int parentA,
    int daughterZ,
    int daughterA,
    const char* decayType = "BetaPlus",
    const char* radioactiveDataDirectory = "RadioactiveDecay5.5",
    const char* photonDataDirectory = "PhotonEvaporation5.5",
    double energyTolerance_keV = 1.0
)
{
    if(decayType == nullptr ||
       radioactiveDataDirectory == nullptr ||
       photonDataDirectory == nullptr)
    {
        throw std::invalid_argument(
            "test_reader_gamma_quiver: string arguments cannot be null"
        );
    }

    RadioactiveDecayReader radioactiveReader(
        radioactiveDataDirectory
    );

    PhotonEvaporationReader photonReader(
        photonDataDirectory
    );

    const RadioactiveIsotope parent =
        radioactiveReader.read(parentZ, parentA);

    const PhotonIsotope daughter =
        photonReader.read(daughterZ, daughterA);

    const DecayQuiver quiver =
        DecayQuiverBuilder::BuildGammaQuiver(
            parent,
            daughter,
            decayType,
            energyTolerance_keV
        );

    std::cout
        << "\n"
        << "============================================================\n"
        << " Reader-built gamma-decay quiver\n"
        << " Parent:   Z=" << parentZ << ", A=" << parentA << "\n"
        << " Daughter: Z=" << daughterZ << ", A=" << daughterA << "\n"
        << " Decay:    " << decayType << "\n"
        << "============================================================\n\n";

    quiver.Print();

    bool success = !quiver.GetLevels().empty();

    if(!success)
    {
        std::cout << "FAIL: quiver contains no levels\n";
    }

    for(const DecayLevel* level : quiver.GetLevels())
    {
        const double outgoing =
            quiver.GetOutgoingProbability(level);

        const bool hasOutgoing =
            outgoing > 0.0;

        const bool normalized =
            !hasOutgoing ||
            std::abs(outgoing - 1.0) <= 1.0e-12;

        if(!normalized)
        {
            std::cout
                << "FAIL: outgoing probability from "
                << level->GetName()
                << " is "
                << outgoing
                << ", not 1\n";
        }

        success = success && normalized;
    }

    for(const DecayTransition* transition :
        quiver.GetTransitions())
    {
        const bool endpointsPresent =
            transition->GetSource() != nullptr &&
            transition->GetTarget() != nullptr;

        if(!endpointsPresent)
        {
            std::cout
                << "FAIL: transition "
                << transition->GetName()
                << " has a null endpoint\n";
        }

        success = success && endpointsPresent;
    }

    std::cout
        << "\nLevels: "
        << quiver.GetLevels().size()
        << "\nTransitions: "
        << quiver.GetTransitions().size()
        << "\nResult: "
        << (success ? "PASS" : "FAIL")
        << "\n"
        << std::endl;

    return success;
}

// Convenience examples:
//
//   test_reader_gamma_quiver(12, 22, 11, 22, "BetaPlus");
//   test_reader_gamma_quiver(56, 133, 55, 133, "EC");
