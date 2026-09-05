#ifndef COINALGEBRA_RADIOACTIVEDECAYREADER_H
#define COINALGEBRA_RADIOACTIVEDECAYREADER_H

#include <cstddef>
#include <string>
#include <vector>

struct RadioactiveDecayChannel
{
    std::string decayType;

    // Daughter nuclear level
    double daughterEnergy_keV;
    std::string daughterFloating;

    // Percentage of the total branching fraction
    // for this decay mode.
    double branchingPercentage;

    // Q value associated with this decay channel.
    double qValue_keV;

    // Optional beta forbiddenness classification.
    std::string forbiddenness;

    RadioactiveDecayChannel();

    RadioactiveDecayChannel(
        const std::string& decayType,
        double daughterEnergy_keV,
        const std::string& daughterFloating,
        double branchingPercentage,
        double qValue_keV,
        const std::string& forbiddenness = ""
    );

    // Convert percentage to fraction.
    double modeFraction() const;
};


struct RadioactiveDecayMode
{
    std::string decayType;

    // Total branching fraction for this decay mode.
    // Example:
    // BetaPlus   0  0.99918
    double totalBranchingFraction;

    std::vector<RadioactiveDecayChannel> channels;

    RadioactiveDecayMode();

    RadioactiveDecayMode(
        const std::string& decayType,
        double totalBranchingFraction
    );

    std::size_t numberOfChannels() const;

    void addChannel(
        const RadioactiveDecayChannel& channel
    );

    // Branching fraction of this particular channel
    // relative to the total decay probability.
    double channelBranchingFraction(
        std::size_t index
    ) const;
};


struct RadioactiveParentState
{
    double energy_keV;
    std::string floating;
    double halfLife_s;

    std::vector<RadioactiveDecayMode> decayModes;

    RadioactiveParentState();

    RadioactiveParentState(
        double energy_keV,
        const std::string& floating,
        double halfLife_s
    );

    std::size_t numberOfDecayModes() const;

    bool isStable() const;

    void addDecayMode(
        const RadioactiveDecayMode& mode
    );
};


class RadioactiveIsotope
{
public:

    RadioactiveIsotope();

    RadioactiveIsotope(
        int atomicNumber,
        int massNumber
    );

    int atomicNumber() const;

    int massNumber() const;

    const std::vector<RadioactiveParentState>&
    parentStates() const;

    std::vector<RadioactiveParentState>&
    parentStates();

    std::size_t numberOfParentStates() const;

    const RadioactiveParentState&
    parentState(
        double energy_keV
    ) const;

    RadioactiveParentState&
    parentState(
        double energy_keV
    );

    void addParentState(
        const RadioactiveParentState& state
    );

private:

    int fAtomicNumber;
    int fMassNumber;

    std::vector<RadioactiveParentState>
        fParentStates;
};


class RadioactiveDecayReader
{
public:

    explicit RadioactiveDecayReader(
        const std::string& dataDirectory
    );

    RadioactiveIsotope read(
        int atomicNumber,
        int massNumber
    ) const;

    std::string filePath(
        int atomicNumber,
        int massNumber
    ) const;

    const std::string&
    dataDirectory() const;

private:

    std::string fDataDirectory;

    RadioactiveParentState parseParentState(
        const std::string& line
    ) const;

    RadioactiveDecayMode parseDecayMode(
        const std::string& line
    ) const;

    RadioactiveDecayChannel parseDecayChannel(
        const std::string& line
    ) const;
};

#endif // COINALGEBRA_RADIOACTIVEDECAYREADER_H