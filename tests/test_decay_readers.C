#include "COINAlgebra/PhotonEvaporationReader.h"
#include "COINAlgebra/RadioactiveDecayReader.h"

#include <cmath>
#include <iostream>
#include <string>

// ============================================================
// Helper
// ============================================================

bool check(
    bool condition,
    const std::string& message
)
{
    if(condition)
    {
        std::cout << "  PASS: " << message << std::endl;
        return true;
    }

    std::cout << "  FAIL: " << message << std::endl;
    return false;
}


// ============================================================
// Test RadioactiveDecayReader
// ============================================================

bool test_radioactive_decay_reader()
{
    std::cout
        << "\n"
        << "============================================\n"
        << " Testing RadioactiveDecayReader\n"
        << "============================================\n";

    bool success = true;

    RadioactiveDecayReader reader(
        "RadioactiveDecay5.5"
    );

    // --------------------------------------------------------
    // Check data directory
    // --------------------------------------------------------

    success &= check(
        reader.dataDirectory() == "RadioactiveDecay5.5",
        "Data directory"
    );


    // --------------------------------------------------------
    // Check file path
    // --------------------------------------------------------

    const std::string path =
        reader.filePath(12, 22);

    std::cout
        << "  File: "
        << path
        << std::endl;

    success &= check(
        path.find("z12.a22") != std::string::npos,
        "22Mg file path"
    );


    // --------------------------------------------------------
    // Read 22Mg
    // --------------------------------------------------------

    RadioactiveIsotope mg22 =
        reader.read(12, 22);

    success &= check(
        mg22.atomicNumber() == 12,
        "Atomic number Z = 12"
    );

    success &= check(
        mg22.massNumber() == 22,
        "Mass number A = 22"
    );


    // --------------------------------------------------------
    // Parent states
    // --------------------------------------------------------

    std::cout
        << "  Number of parent states: "
        << mg22.numberOfParentStates()
        << std::endl;

    success &= check(
        mg22.numberOfParentStates() > 0,
        "At least one parent state"
    );

    if(mg22.numberOfParentStates() == 0)
        return false;


    // --------------------------------------------------------
    // First parent state
    // --------------------------------------------------------

    const auto& parent =
        mg22.parentStates().front();

    std::cout
        << "\n"
        << "  First parent state:\n"
        << "    Energy:    "
        << parent.energy_keV
        << " keV\n"
        << "    Floating:  "
        << parent.floating
        << "\n"
        << "    Half-life: "
        << parent.halfLife_s
        << " s\n";


    // --------------------------------------------------------
    // 22Mg half-life
    // --------------------------------------------------------

    success &= check(
        std::abs(parent.halfLife_s - 3.8755) < 1.0e-4,
        "22Mg half-life = 3.8755 s"
    );


    // --------------------------------------------------------
    // Decay modes
    // --------------------------------------------------------

    std::cout
        << "\n"
        << "  Decay modes: "
        << parent.numberOfDecayModes()
        << std::endl;

    success &= check(
        parent.numberOfDecayModes() > 0,
        "At least one decay mode"
    );


    double totalBranching = 0.0;
    bool foundBetaPlus = false;
    bool foundEC = false;


    for(const auto& mode : parent.decayModes)
    {
        std::cout
            << "\n"
            << "    Mode: "
            << mode.decayType
            << "\n"
            << "      Branching fraction: "
            << mode.totalBranchingFraction
            << "\n"
            << "      Number of channels: "
            << mode.numberOfChannels()
            << std::endl;

        totalBranching +=
            mode.totalBranchingFraction;


        // ----------------------------------------------------
        // Beta+
        // ----------------------------------------------------

        if(mode.decayType == "BetaPlus")
        {
            foundBetaPlus = true;

            success &= check(
                std::abs(
                    mode.totalBranchingFraction
                    -
                    0.99918
                ) < 1.0e-5,
                "BetaPlus branching fraction = 0.99918"
            );
        }


        // ----------------------------------------------------
        // Electron capture
        // ----------------------------------------------------

        if(
            mode.decayType == "KshellEC" ||
            mode.decayType == "LshellEC" ||
            mode.decayType == "MshellEC" ||
            mode.decayType == "NshellEC"
        )
        {
            foundEC = true;
        }


        // ----------------------------------------------------
        // Channels
        // ----------------------------------------------------

        for(std::size_t i = 0;
            i < mode.channels.size();
            ++i)
        {
            const auto& channel =
                mode.channels[i];

            std::cout
                << "        Channel "
                << i
                << ":\n"
                << "          Daughter energy: "
                << channel.daughterEnergy_keV
                << " keV\n"
                << "          Floating: "
                << channel.daughterFloating
                << "\n"
                << "          Branching: "
                << channel.branchingPercentage
                << " %\n"
                << "          Q: "
                << channel.qValue_keV
                << " keV"
                << std::endl;

            success &= check(
                channel.branchingPercentage >= 0.0,
                "Channel branching percentage >= 0"
            );

            success &= check(
                channel.modeFraction() >= 0.0 &&
                channel.modeFraction() <= 1.0,
                "Channel branching fraction in [0,1]"
            );
        }
    }


    success &= check(
        foundBetaPlus,
        "BetaPlus decay mode found"
    );


    if(foundEC)
    {
        std::cout
            << "  PASS: Electron-capture mode found"
            << std::endl;
    }
    else
    {
        std::cout
            << "  INFO: No separate electron-capture "
            << "mode found"
            << std::endl;
    }


    // --------------------------------------------------------
    // Branching normalization
    // --------------------------------------------------------

    std::cout
        << "\n"
        << "  Total decay-mode branching: "
        << totalBranching
        << std::endl;

    success &= check(
        std::abs(totalBranching - 1.0) < 1.0e-3,
        "Decay branching fractions approximately sum to 1"
    );


    // --------------------------------------------------------
    // Parent-state lookup
    // --------------------------------------------------------

    const auto& lookup =
        mg22.parentState(parent.energy_keV);

    success &= check(
        std::abs(
            lookup.energy_keV -
            parent.energy_keV
        ) < 1.0e-9,
        "Parent-state lookup by energy"
    );


    return success;
}


// ============================================================
// Test PhotonTransition
// ============================================================

bool test_photon_transition()
{
    std::cout
        << "\n"
        << "============================================\n"
        << " Testing PhotonTransition\n"
        << "============================================\n";

    bool success = true;

    std::array<double, 10> shells{};

    PhotonTransition transition(
        1,
        583.0,
        100.0,
        3,
        0.0,
        0.0,
        shells
    );


    success &= check(
        transition.daughterLevel == 1,
        "Daughter level"
    );

    success &= check(
        std::abs(
            transition.energy_keV - 583.0
        ) < 1.0e-9,
        "Gamma energy"
    );

    success &= check(
        std::abs(
            transition.relativeIntensity - 100.0
        ) < 1.0e-9,
        "Relative intensity"
    );

    success &= check(
        transition.multipolarity == 3,
        "Multipolarity"
    );

    success &= check(
        std::abs(
            transition.gammaProbability() - 1.0
        ) < 1.0e-12,
        "Gamma probability for alpha = 0"
    );

    success &= check(
        std::abs(
            transition.conversionProbability()
        ) < 1.0e-12,
        "Conversion probability for alpha = 0"
    );


    // --------------------------------------------------------
    // Test internal conversion
    // --------------------------------------------------------

    PhotonTransition converted(
        1,
        100.0,
        50.0,
        3,
        0.0,
        1.0,
        shells
    );

    success &= check(
        std::abs(
            converted.gammaProbability() - 0.5
        ) < 1.0e-12,
        "Gamma probability for alpha = 1"
    );

    success &= check(
        std::abs(
            converted.conversionProbability() - 0.5
        ) < 1.0e-12,
        "Conversion probability for alpha = 1"
    );

    success &= check(
        std::abs(
            converted.gammaProbability()
            +
            converted.conversionProbability()
            -
            1.0
        ) < 1.0e-12,
        "Gamma + conversion probability = 1"
    );


    return success;
}


// ============================================================
// Test PhotonLevel
// ============================================================

bool test_photon_level()
{
    std::cout
        << "\n"
        << "============================================\n"
        << " Testing PhotonLevel\n"
        << "============================================\n";

    bool success = true;

    PhotonLevel level(
        0,
        "-",
        0.0,
        -1.0,
        0.0
    );


    success &= check(
        level.id == 0,
        "Level ID"
    );

    success &= check(
        level.isStable(),
        "Stable level"
    );

    success &= check(
        level.hasKnownJPi(),
        "Known JPi"
    );

    success &= check(
        level.numberOfGammas() == 0,
        "Initial gamma count"
    );


    std::array<double, 10> shells{};

    level.transitions.emplace_back(
        0,
        100.0,
        1.0,
        3,
        0.0,
        0.0,
        shells
    );


    success &= check(
        level.numberOfGammas() == 1,
        "Gamma count after adding transition"
    );


    return success;
}


// ============================================================
// Test PhotonEvaporationReader
// ============================================================

bool test_photon_evaporation_reader()
{
    std::cout
        << "\n"
        << "============================================\n"
        << " Testing PhotonEvaporationReader\n"
        << "============================================\n";

    bool success = true;

    PhotonEvaporationReader reader(
        "PhotonEvaporation5.5"
    );


    // --------------------------------------------------------
    // File path
    // --------------------------------------------------------

    const std::string path =
        reader.filePath(12, 22);

    std::cout
        << "  File: "
        << path
        << std::endl;

    success &= check(
        path.find("z12.a22") != std::string::npos,
        "22Mg PhotonEvaporation file path"
    );


    // --------------------------------------------------------
    // Read isotope
    // --------------------------------------------------------

    PhotonIsotope mg22 =
        reader.read(12, 22);

    success &= check(
        mg22.atomicNumber() == 12,
        "Atomic number Z = 12"
    );

    success &= check(
        mg22.massNumber() == 22,
        "Mass number A = 22"
    );


    // --------------------------------------------------------
    // Levels
    // --------------------------------------------------------

    std::cout
        << "  Number of levels: "
        << mg22.numberOfLevels()
        << std::endl;

    success &= check(
        mg22.numberOfLevels() > 0,
        "At least one photon level"
    );

    if(mg22.numberOfLevels() == 0)
        return false;


    // --------------------------------------------------------
    // Inspect levels
    // --------------------------------------------------------

    for(const auto& level : mg22.levels())
    {
        std::cout
            << "\n"
            << "    Level "
            << level.id
            << ":\n"
            << "      Energy:    "
            << level.energy_keV
            << " keV\n"
            << "      Floating:  "
            << level.floating
            << "\n"
            << "      Half-life: "
            << level.halfLife_s
            << " s\n"
            << "      JPi:       "
            << level.jpi
            << "\n"
            << "      Gammas:    "
            << level.numberOfGammas()
            << std::endl;


        for(const auto& gamma : level.transitions)
        {
            std::cout
                << "        Gamma:\n"
                << "          Daughter: "
                << gamma.daughterLevel
                << "\n"
                << "          Energy:   "
                << gamma.energy_keV
                << " keV\n"
                << "          Intensity: "
                << gamma.relativeIntensity
                << "\n"
                << "          Multipolarity: "
                << gamma.multipolarity
                << "\n"
                << "          Mixing ratio:  "
                << gamma.mixingRatio
                << "\n"
                << "          Alpha:          "
                << gamma.conversionCoefficient
                << std::endl;


            success &= check(
                gamma.energy_keV >= 0.0,
                "Gamma energy >= 0"
            );

            success &= check(
                gamma.relativeIntensity >= 0.0,
                "Gamma intensity >= 0"
            );

            success &= check(
                gamma.gammaProbability() >= 0.0 &&
                gamma.gammaProbability() <= 1.0,
                "Gamma probability in [0,1]"
            );

            success &= check(
                gamma.conversionProbability() >= 0.0 &&
                gamma.conversionProbability() <= 1.0,
                "Conversion probability in [0,1]"
            );

            success &= check(
                std::abs(
                    gamma.gammaProbability()
                    +
                    gamma.conversionProbability()
                    -
                    1.0
                ) < 1.0e-12,
                "Gamma + conversion probability = 1"
            );
        }
    }


    // --------------------------------------------------------
    // Level lookup
    // --------------------------------------------------------

    const int firstID =
        mg22.levels().front().id;

    const auto& lookup =
        mg22.level(firstID);

    success &= check(
        lookup.id == firstID,
        "Level lookup by ID"
    );


    return success;
}


// ============================================================
// Main ROOT test function
// ============================================================

void test_decay_readers()
{
    std::cout
        << "\n"
        << "################################################\n"
        << "#                                              #\n"
        << "#       COINAlgebra Decay Reader Tests         #\n"
        << "#                                              #\n"
        << "################################################\n";


    bool success = true;


    // --------------------------------------------------------
    // Object tests
    // --------------------------------------------------------

    success &= test_photon_transition();

    success &= test_photon_level();


    // --------------------------------------------------------
    // RadioactiveDecay reader
    // --------------------------------------------------------

    success &= test_radioactive_decay_reader();


    // --------------------------------------------------------
    // PhotonEvaporation reader
    // --------------------------------------------------------

    success &= test_photon_evaporation_reader();


    // --------------------------------------------------------
    // Final result
    // --------------------------------------------------------

    std::cout
        << "\n"
        << "################################################\n";

    if(success)
    {
        std::cout
            << "#              ALL TESTS PASSED              #\n";
    }
    else
    {
        std::cout
            << "#               TESTS FAILED                 #\n";
    }

    std::cout
        << "################################################\n"
        << std::endl;
}
