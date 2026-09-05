#include "COINAlgebra/DecayQuiverBuilder.h"

#include <cmath>
#include <stdexcept>
#include <string>
#include <vector>


// ============================================================
// FindLevelByEnergy
// ============================================================

const PhotonLevel*
DecayQuiverBuilder::FindLevelByEnergy(
    const PhotonIsotope& daughter,
    double energy_keV,
    double tolerance_keV
)
{
    const PhotonLevel* bestLevel = nullptr;

    double bestDifference =
        tolerance_keV;

    for(const PhotonLevel& level :
        daughter.levels())
    {
        const double difference =
            std::abs(
                level.energy_keV -
                energy_keV
            );

        if(difference <= bestDifference)
        {
            bestDifference = difference;
            bestLevel = &level;
        }
    }

    return bestLevel;
}


// ============================================================
// CollectReachableGammaLevels
// ============================================================

void
DecayQuiverBuilder::CollectReachableGammaLevels(
    const PhotonIsotope& daughter,
    int levelID,
    std::set<int>& reachableLevels
)
{
    // Already visited
    if(reachableLevels.count(levelID) > 0)
    {
        return;
    }

    reachableLevels.insert(levelID);

    const PhotonLevel& level =
        daughter.level(levelID);

    for(const PhotonTransition& transition :
        level.transitions)
    {
        CollectReachableGammaLevels(
            daughter,
            transition.daughterLevel,
            reachableLevels
        );
    }
}


// ============================================================
// IsElectronCaptureMode
// ============================================================

bool
DecayQuiverBuilder::IsElectronCaptureMode(
    const std::string& decayType
)
{
    return
        decayType == "EC" ||
        decayType == "ElectronCapture" ||
        decayType == "KshellEC" ||
        decayType == "LshellEC" ||
        decayType == "MshellEC";
}


// ============================================================
// ModeMatchesDecayType
// ============================================================

bool
DecayQuiverBuilder::ModeMatchesDecayType(
    const std::string& mode,
    const std::string& decayType
)
{
    // --------------------------------------------------------
    // Physical EC:
    //
    // Combine K-, L-, and M-shell electron capture.
    // --------------------------------------------------------

    if(decayType == "EC" ||
       decayType == "ElectronCapture")
    {
        return IsElectronCaptureMode(mode);
    }

    // --------------------------------------------------------
    // Individual EC subshells
    // --------------------------------------------------------

    if(decayType == "KshellEC")
    {
        return mode == "KshellEC";
    }

    if(decayType == "LshellEC")
    {
        return mode == "LshellEC";
    }

    if(decayType == "MshellEC")
    {
        return mode == "MshellEC";
    }

    // --------------------------------------------------------
    // Other radioactive decay modes
    // --------------------------------------------------------

    return mode == decayType;
}


// ============================================================
// BuildGammaQuiver
// ============================================================

DecayQuiver
DecayQuiverBuilder::BuildGammaQuiver(
    const RadioactiveIsotope& parent,
    const PhotonIsotope& daughter,
    const std::string& decayType,
    double energyTolerance_keV
)
{
    // --------------------------------------------------------
    // Validate input
    // --------------------------------------------------------

    if(parent.parentStates().empty())
    {
        throw std::runtime_error(
            "DecayQuiverBuilder: parent radioactive "
            "isotope contains no parent states."
        );
    }

    if(daughter.levels().empty())
    {
        throw std::runtime_error(
            "DecayQuiverBuilder: daughter PhotonEvaporation "
            "isotope contains no levels."
        );
    }

    // --------------------------------------------------------
    // The radioactive-decay data describe the parent.
    //
    // For now we use the first parent state, which is the
    // ground state for the isotopes currently being tested.
    // --------------------------------------------------------

    const RadioactiveParentState& parentState =
        parent.parentStates().front();

    // --------------------------------------------------------
    // Find radioactive-decay modes matching the requested
    // physical decay type.
    // --------------------------------------------------------

    std::vector<const RadioactiveDecayMode*>
        matchingModes;

    for(const RadioactiveDecayMode& mode :
        parentState.decayModes)
    {
        if(ModeMatchesDecayType(
               mode.decayType,
               decayType))
        {
            matchingModes.push_back(&mode);
        }
    }

    if(matchingModes.empty())
    {
        throw std::runtime_error(
            "DecayQuiverBuilder: no radioactive decay "
            "modes matching '" +
            decayType +
            "' were found in the parent nucleus."
        );
    }

    // --------------------------------------------------------
    // Match radioactive daughter energies to
    // PhotonEvaporation daughter levels.
    // --------------------------------------------------------

    std::set<int> initialLevels;

    for(const RadioactiveDecayMode* mode :
        matchingModes)
    {
        for(const RadioactiveDecayChannel& channel :
            mode->channels)
        {
            const PhotonLevel* level =
                FindLevelByEnergy(
                    daughter,
                    channel.daughterEnergy_keV,
                    energyTolerance_keV
                );

            if(level == nullptr)
            {
                throw std::runtime_error(
                    "DecayQuiverBuilder: could not match "
                    "radioactive-decay daughter excitation "
                    "energy " +
                    std::to_string(
                        channel.daughterEnergy_keV
                    ) +
                    " keV to a PhotonEvaporation level "
                    "in the daughter nucleus."
                );
            }

            initialLevels.insert(level->id);
        }
    }

    if(initialLevels.empty())
    {
        throw std::runtime_error(
            "DecayQuiverBuilder: the requested radioactive "
            "decay mode contains no daughter levels."
        );
    }

    // --------------------------------------------------------
    // Starting from the radioactive-decay populated levels,
    // recursively follow all gamma transitions.
    //
    // This is important: we do NOT simply include every
    // PhotonEvaporation level below some energy cutoff.
    // Only levels actually reachable from the radioactive
    // decay branches are included.
    // --------------------------------------------------------

    std::set<int> reachableLevels;

    for(const int levelID :
        initialLevels)
    {
        CollectReachableGammaLevels(
            daughter,
            levelID,
            reachableLevels
        );
    }

    // --------------------------------------------------------
    // Create the quiver vertices
    // --------------------------------------------------------

    DecayQuiver quiver;

    for(const PhotonLevel& level :
        daughter.levels())
    {
        if(reachableLevels.count(level.id) == 0)
        {
            continue;
        }

        const std::string name =
            "level_" +
            std::to_string(level.energy_keV) +
            "keV";

        quiver.AddLevel(name);
    }

    // --------------------------------------------------------
    // Create the gamma transitions
    // --------------------------------------------------------

    for(const PhotonLevel& sourceLevel :
        daughter.levels())
    {
        if(reachableLevels.count(sourceLevel.id) == 0)
        {
            continue;
        }

        const std::string sourceName =
            "level_" +
            std::to_string(sourceLevel.energy_keV) +
            "keV";

        DecayLevel* source =
            quiver.GetLevel(sourceName);

        if(source == nullptr)
        {
            throw std::runtime_error(
                "DecayQuiverBuilder: failed to create "
                "source quiver level."
            );
        }

        // ----------------------------------------------------
        // Calculate total relative gamma intensity for the
        // reachable transitions from this level.
        // ----------------------------------------------------

        double totalIntensity = 0.0;

        for(const PhotonTransition& gamma :
            sourceLevel.transitions)
        {
            if(reachableLevels.count(
                   gamma.daughterLevel) == 0)
            {
                continue;
            }

            if(gamma.relativeIntensity <= 0.0)
            {
                continue;
            }

            totalIntensity +=
                gamma.relativeIntensity;
        }

        // No gamma transitions from this level.
        // It is therefore a terminal/stable level in the
        // constructed gamma-decay quiver.
        if(totalIntensity <= 0.0)
        {
            continue;
        }

        // ----------------------------------------------------
        // Add each reachable gamma branch.
        //
        // The probability is conditional on being in the
        // source level:
        //
        //     P(i -> j | i)
        //       = I_ij / sum_k I_ik
        //
        // ----------------------------------------------------

        for(std::size_t i = 0;
            i < sourceLevel.transitions.size();
            ++i)
        {
            const PhotonTransition& gamma =
                sourceLevel.transitions[i];

            if(reachableLevels.count(
                   gamma.daughterLevel) == 0)
            {
                continue;
            }

            if(gamma.relativeIntensity <= 0.0)
            {
                continue;
            }

            const PhotonLevel& daughterLevel =
                daughter.level(
                    gamma.daughterLevel
                );

            const std::string targetName =
                "level_" +
                std::to_string(daughterLevel.energy_keV) +
                "keV";

            DecayLevel* target =
                quiver.GetLevel(targetName);

            if(target == nullptr)
            {
                throw std::runtime_error(
                    "DecayQuiverBuilder: failed to create "
                    "target quiver level."
                );
            }

            const double probability =
                gamma.relativeIntensity /
                totalIntensity;

            // ------------------------------------------------
            // Use the transition index to guarantee uniqueness
            // even when two transitions have the same rounded
            // source and target energies.
            // ------------------------------------------------

            const std::string transitionName =
                "gamma_" +
                std::to_string(
                    static_cast<int>(
                        std::round(
                            sourceLevel.energy_keV
                        )
                    )
                ) +
                "_to_" +
                std::to_string(
                    static_cast<int>(
                        std::round(
                            daughterLevel.energy_keV
                        )
                    )
                ) +
                "_" +
                std::to_string(i);

            quiver.AddTransition(
                transitionName,
                source,
                target,
                probability
            );
        }
    }

    return quiver;
}
