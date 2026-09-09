#ifndef COINALGEBRA_DECAYQUIVERBUILDER_H
#define COINALGEBRA_DECAYQUIVERBUILDER_H

#include "COINAlgebra/Core/DecayQuiver.h"
#include "COINAlgebra/NuclearData/PhotonEvaporationReader.h"
#include "COINAlgebra/NuclearData/RadioactiveDecayReader.h"

#include <set>
#include <string>
#include <unordered_map>

class DecayQuiverBuilder
{
public:

    // --------------------------------------------------------
    // Build gamma-decay quiver
    // --------------------------------------------------------
    //
    // parent:
    //     RadioactiveDecay data for the parent nucleus.
    //
    // daughter:
    //     PhotonEvaporation data for the daughter nucleus.
    //
    // decayType:
    //     Radioactive decay mode to follow, e.g.
    //     "BetaPlus", "EC", "KshellEC", "LshellEC",
    //     or "MshellEC".
    //
    // energyTolerance_keV:
    //     Maximum allowed difference when matching a
    //     radioactive-decay daughter energy to a
    //     PhotonEvaporation level.
    //
    // The resulting quiver contains only the daughter
    // levels reachable from the selected radioactive-decay
    // channels through gamma transitions.
    //
    // Edge probabilities include gamma emission AND internal conversion:
    // I_gamma * (1 + alpha), normalized for each excited level.
    //
    static DecayQuiver BuildGammaQuiver(
        const RadioactiveIsotope& parent,
        const PhotonIsotope& daughter,
        const std::string& decayType = "BetaPlus",
        double energyTolerance_keV = 1.0
    );

    // Also extract total internal conversion coefficients, keyed by the
    // generated transition names. Replaces the output map on success.
    static DecayQuiver BuildGammaQuiver(
        const RadioactiveIsotope& parent,
        const PhotonIsotope& daughter,
        std::unordered_map<std::string, double>& conversionCoefficients,
        const std::string& decayType = "BetaPlus",
        double energyTolerance_keV = 1.0
    );

private:

    // --------------------------------------------------------
    // Find daughter level by excitation energy
    // --------------------------------------------------------

    static const PhotonLevel* FindLevelByEnergy(
        const PhotonIsotope& daughter,
        double energy_keV,
        double tolerance_keV
    );

    // --------------------------------------------------------
    // Recursively collect all gamma-reachable levels
    // --------------------------------------------------------

    static void CollectReachableGammaLevels(
        const PhotonIsotope& daughter,
        int levelID,
        std::set<int>& reachableLevels
    );

    // --------------------------------------------------------
    // Determine whether a decay type is an EC mode
    // --------------------------------------------------------

    static bool IsElectronCaptureMode(
        const std::string& decayType
    );

    // --------------------------------------------------------
    // Determine whether a radioactive-decay mode matches
    // the requested physical decay type
    // --------------------------------------------------------

    static bool ModeMatchesDecayType(
        const std::string& mode,
        const std::string& decayType
    );
};

#endif

