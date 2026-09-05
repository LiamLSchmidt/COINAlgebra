#include "COINAlgebra/RadioactiveDecayReader.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <vector>


namespace
{

// ------------------------------------------------------------
// Trim whitespace from both ends of a string
// ------------------------------------------------------------

std::string trim(
    const std::string& input
)
{
    const auto first =
        input.find_first_not_of(" \t\r\n");

    if(first == std::string::npos)
    {
        return "";
    }

    const auto last =
        input.find_last_not_of(" \t\r\n");

    return input.substr(
        first,
        last - first + 1
    );
}


// ------------------------------------------------------------
// Split a line into whitespace-separated fields
// ------------------------------------------------------------

std::vector<std::string> splitFields(
    const std::string& line
)
{
    std::istringstream stream(line);

    std::vector<std::string> fields;
    std::string field;

    while(stream >> field)
    {
        fields.push_back(field);
    }

    return fields;
}


// ------------------------------------------------------------
// Convert a string to a double
// ------------------------------------------------------------

double parseDouble(
    const std::string& value,
    const std::string& context
)
{
    try
    {
        std::size_t position = 0;

        const double result =
            std::stod(value, &position);

        if(position != value.size())
        {
            throw std::runtime_error(
                "invalid numeric value"
            );
        }

        return result;
    }
    catch(const std::exception&)
    {
        throw std::runtime_error(
            "RadioactiveDecayReader: could not parse "
            "numeric value '" +
            value +
            "' in " +
            context
        );
    }
}


// ------------------------------------------------------------
// Test whether a line is blank
// ------------------------------------------------------------

bool isBlank(
    const std::string& line
)
{
    return trim(line).empty();
}


// ------------------------------------------------------------
// Test whether a line is a comment
// ------------------------------------------------------------

bool isComment(
    const std::string& line
)
{
    const std::string stripped = trim(line);

    return !stripped.empty() &&
           stripped[0] == '#';
}


// ------------------------------------------------------------
// Test whether a line is a parent-state record
//
// Parent-state records begin with:
//
//     P
//
// Example:
//
//     P  0  -  3.8755
// ------------------------------------------------------------

bool isParentLine(
    const std::string& line
)
{
    const std::string stripped = trim(line);

    return !stripped.empty() &&
           stripped[0] == 'P';
}

} // anonymous namespace


// ============================================================
// RadioactiveDecayChannel
// ============================================================

RadioactiveDecayChannel::RadioactiveDecayChannel()
    :
    decayType(),
    daughterEnergy_keV(0.0),
    daughterFloating(),
    branchingPercentage(0.0),
    qValue_keV(0.0),
    forbiddenness()
{
}


RadioactiveDecayChannel::RadioactiveDecayChannel(
    const std::string& decayType_,
    double daughterEnergy_keV_,
    const std::string& daughterFloating_,
    double branchingPercentage_,
    double qValue_keV_,
    const std::string& forbiddenness_
)
    :
    decayType(decayType_),
    daughterEnergy_keV(daughterEnergy_keV_),
    daughterFloating(daughterFloating_),
    branchingPercentage(branchingPercentage_),
    qValue_keV(qValue_keV_),
    forbiddenness(forbiddenness_)
{
}


// ------------------------------------------------------------
// Convert channel intensity from percentage to fraction
// ------------------------------------------------------------

double
RadioactiveDecayChannel::modeFraction() const
{
    return branchingPercentage / 100.0;
}


// ============================================================
// RadioactiveDecayMode
// ============================================================

RadioactiveDecayMode::RadioactiveDecayMode()
    :
    decayType(),
    totalBranchingFraction(0.0),
    channels()
{
}


RadioactiveDecayMode::RadioactiveDecayMode(
    const std::string& decayType_,
    double totalBranchingFraction_
)
    :
    decayType(decayType_),
    totalBranchingFraction(totalBranchingFraction_),
    channels()
{
}


std::size_t
RadioactiveDecayMode::numberOfChannels() const
{
    return channels.size();
}


void
RadioactiveDecayMode::addChannel(
    const RadioactiveDecayChannel& channel
)
{
    channels.push_back(channel);
}


// ------------------------------------------------------------
// Calculate the absolute branching fraction for a channel
//
// If:
//
//     total mode branching = 0.99918
//     channel intensity     = 41.303 %
//
// then:
//
//     B = 0.99918 * 41.303 / 100
// ------------------------------------------------------------

double
RadioactiveDecayMode::channelBranchingFraction(
    std::size_t index
) const
{
    if(index >= channels.size())
    {
        throw std::out_of_range(
            "RadioactiveDecayMode::channelBranchingFraction: "
            "channel index out of range"
        );
    }

    return totalBranchingFraction *
           channels[index].modeFraction();
}


// ============================================================
// RadioactiveParentState
// ============================================================

RadioactiveParentState::RadioactiveParentState()
    :
    energy_keV(0.0),
    floating(),
    halfLife_s(0.0),
    decayModes()
{
}


RadioactiveParentState::RadioactiveParentState(
    double energy_keV_,
    const std::string& floating_,
    double halfLife_s_
)
    :
    energy_keV(energy_keV_),
    floating(floating_),
    halfLife_s(halfLife_s_),
    decayModes()
{
}


std::size_t
RadioactiveParentState::numberOfDecayModes() const
{
    return decayModes.size();
}


// ------------------------------------------------------------
// The RDM format uses non-positive half-life values for
// states which do not have a radioactive lifetime.
// ------------------------------------------------------------

bool
RadioactiveParentState::isStable() const
{
    return halfLife_s <= 0.0;
}


void
RadioactiveParentState::addDecayMode(
    const RadioactiveDecayMode& mode
)
{
    decayModes.push_back(mode);
}


// ============================================================
// RadioactiveIsotope
// ============================================================

RadioactiveIsotope::RadioactiveIsotope()
    :
    fAtomicNumber(0),
    fMassNumber(0),
    fParentStates()
{
}


RadioactiveIsotope::RadioactiveIsotope(
    int atomicNumber_,
    int massNumber_
)
    :
    fAtomicNumber(atomicNumber_),
    fMassNumber(massNumber_),
    fParentStates()
{
}


int
RadioactiveIsotope::atomicNumber() const
{
    return fAtomicNumber;
}


int
RadioactiveIsotope::massNumber() const
{
    return fMassNumber;
}


const std::vector<RadioactiveParentState>&
RadioactiveIsotope::parentStates() const
{
    return fParentStates;
}


std::vector<RadioactiveParentState>&
RadioactiveIsotope::parentStates()
{
    return fParentStates;
}


std::size_t
RadioactiveIsotope::numberOfParentStates() const
{
    return fParentStates.size();
}


// ------------------------------------------------------------
// Find a parent state by excitation energy
// ------------------------------------------------------------

const RadioactiveParentState&
RadioactiveIsotope::parentState(
    double energy_keV
) const
{
    for(const auto& state : fParentStates)
    {
        if(std::abs(state.energy_keV - energy_keV)
           < 1.0e-9)
        {
            return state;
        }
    }

    std::ostringstream message;

    message
        << "RadioactiveIsotope::parentState: "
        << "no parent state found at "
        << std::setprecision(12)
        << energy_keV
        << " keV";

    throw std::out_of_range(
        message.str()
    );
}


RadioactiveParentState&
RadioactiveIsotope::parentState(
    double energy_keV
)
{
    for(auto& state : fParentStates)
    {
        if(std::abs(state.energy_keV - energy_keV)
           < 1.0e-9)
        {
            return state;
        }
    }

    std::ostringstream message;

    message
        << "RadioactiveIsotope::parentState: "
        << "no parent state found at "
        << std::setprecision(12)
        << energy_keV
        << " keV";

    throw std::out_of_range(
        message.str()
    );
}


void
RadioactiveIsotope::addParentState(
    const RadioactiveParentState& state
)
{
    fParentStates.push_back(state);
}


// ============================================================
// RadioactiveDecayReader
// ============================================================

RadioactiveDecayReader::RadioactiveDecayReader(
    const std::string& dataDirectory
)
    :
    fDataDirectory(dataDirectory)
{
}


// ------------------------------------------------------------
// Construct the path to an RDM file
//
// Example:
//
//     dataDirectory = "/nessa/geant4"
//     Z = 12
//     A = 22
//
// gives:
//
//     /nessa/geant4/z12.a22
//
// Normally dataDirectory should be the RadioactiveDecay
// directory itself:
//
//     /nessa/geant4/RadioactiveDecay5.4
//
// giving:
//
//     /nessa/geant4/RadioactiveDecay5.4/z12.a22
// ------------------------------------------------------------

std::string
RadioactiveDecayReader::filePath(
    int atomicNumber,
    int massNumber
) const
{
    if(atomicNumber <= 0)
    {
        throw std::invalid_argument(
            "RadioactiveDecayReader::filePath: "
            "atomic number must be positive"
        );
    }

    if(massNumber <= 0)
    {
        throw std::invalid_argument(
            "RadioactiveDecayReader::filePath: "
            "mass number must be positive"
        );
    }

    std::ostringstream path;

    path
        << fDataDirectory
        << "/z"
        << atomicNumber
        << ".a"
        << massNumber;

    return path.str();
}


const std::string&
RadioactiveDecayReader::dataDirectory() const
{
    return fDataDirectory;
}


// ============================================================
// Parse parent-state record
//
// Format:
//
//     P  Excitation  flag  Halflife
//
// Examples:
//
//     P  0        -  3.8755
//     P  5005.7   -  1.7e-08
// ============================================================

RadioactiveParentState
RadioactiveDecayReader::parseParentState(
    const std::string& line
) const
{
    const auto fields =
        splitFields(line);

    if(fields.size() != 4)
    {
        throw std::runtime_error(
            "RadioactiveDecayReader: malformed parent-state "
            "line: '" +
            line +
            "'"
        );
    }

    if(fields[0] != "P")
    {
        throw std::runtime_error(
            "RadioactiveDecayReader: expected 'P' at start "
            "of parent-state line: '" +
            line +
            "'"
        );
    }

    const double energy =
        parseDouble(
            fields[1],
            "parent-state excitation energy"
        );

    const std::string floating =
        fields[2];

    const double halfLife =
        parseDouble(
            fields[3],
            "parent-state half-life"
        );

    return RadioactiveParentState(
        energy,
        floating,
        halfLife
    );
}


// ============================================================
// Parse decay-mode record
//
// Format:
//
//     Mode  0  Branching
//
// Examples:
//
//     BetaPlus   0  0.99918
//     KshellEC   0  0.00080246
//     LshellEC   0  1.4511e-05
//     IT         0  1
//
// The second field is unused by COINAlgebra but is retained
// by the parser structurally.
// ============================================================

RadioactiveDecayMode
RadioactiveDecayReader::parseDecayMode(
    const std::string& line
) const
{
    const auto fields =
        splitFields(line);

    if(fields.size() != 3)
    {
        throw std::runtime_error(
            "RadioactiveDecayReader: malformed decay-mode "
            "line: '" +
            line +
            "'"
        );
    }

    const std::string decayType =
        fields[0];

    const double unusedValue =
        parseDouble(
            fields[1],
            "decay-mode unused field"
        );

    const double branching =
        parseDouble(
            fields[2],
            "decay-mode branching fraction"
        );

    // The RDM specification indicates that the second
    // field should be zero. Check it rather than silently
    // accepting malformed data.
    if(std::abs(unusedValue) > 1.0e-12)
    {
        std::ostringstream message;

        message
            << "RadioactiveDecayReader: expected zero in "
            << "second field of decay-mode record for '"
            << decayType
            << "', got "
            << unusedValue;

        throw std::runtime_error(
            message.str()
        );
    }

    if(branching < 0.0)
    {
        throw std::runtime_error(
            "RadioactiveDecayReader: negative total branching "
            "fraction for decay mode '" +
            decayType +
            "'"
        );
    }

    return RadioactiveDecayMode(
        decayType,
        branching
    );
}


// ============================================================
// Parse daughter-channel record
//
// Standard:
//
//     BetaPlus  583.11  -  41.303  4198.47
//
// Optional beta forbiddenness:
//
//     BetaPlus  583.11  -  41.303  4198.47 firstForbidden
//
// Fields:
//
//     0  decay type
//     1  daughter excitation energy
//     2  daughter floating flag
//     3  channel intensity (% of mode)
//     4  Q value
//     5  optional forbiddenness
// ============================================================

RadioactiveDecayChannel
RadioactiveDecayReader::parseDecayChannel(
    const std::string& line
) const
{
    const auto fields =
        splitFields(line);

    if(fields.size() < 5)
    {
        throw std::runtime_error(
            "RadioactiveDecayReader: malformed decay-channel "
            "line: '" +
            line +
            "'"
        );
    }

    const std::string decayType =
        fields[0];

    const double daughterEnergy =
        parseDouble(
            fields[1],
            "daughter excitation energy"
        );

    const std::string daughterFloating =
        fields[2];

    const double branchingPercentage =
        parseDouble(
            fields[3],
            "daughter branching percentage"
        );

    const double qValue =
        parseDouble(
            fields[4],
            "Q value"
        );

    if(branchingPercentage < 0.0)
    {
        throw std::runtime_error(
            "RadioactiveDecayReader: negative channel "
            "branching percentage for decay mode '" +
            decayType +
            "'"
        );
    }

    if(branchingPercentage > 100.0)
    {
        throw std::runtime_error(
            "RadioactiveDecayReader: channel branching "
            "percentage exceeds 100% for decay mode '" +
            decayType +
            "'"
        );
    }

    std::string forbiddenness;

    if(fields.size() >= 6)
    {
        forbiddenness = fields[5];
    }

    return RadioactiveDecayChannel(
        decayType,
        daughterEnergy,
        daughterFloating,
        branchingPercentage,
        qValue,
        forbiddenness
    );
}


// ============================================================
// Read isotope
// ============================================================

RadioactiveIsotope
RadioactiveDecayReader::read(
    int atomicNumber,
    int massNumber
) const
{
    const std::string path =
        filePath(
            atomicNumber,
            massNumber
        );

    std::ifstream input(path);

    if(!input.is_open())
    {
        throw std::runtime_error(
            "RadioactiveDecayReader: could not open file '" +
            path +
            "'"
        );
    }

    RadioactiveIsotope isotope(
        atomicNumber,
        massNumber
    );

    std::string line;

    RadioactiveParentState* currentParent =
        nullptr;

    while(std::getline(input, line))
    {
        // ----------------------------------------------------
        // Ignore blank lines
        // ----------------------------------------------------

        if(isBlank(line))
        {
            continue;
        }

        // ----------------------------------------------------
        // Ignore comments
        // ----------------------------------------------------

        if(isComment(line))
        {
            continue;
        }

        // ----------------------------------------------------
        // Parent state
        // ----------------------------------------------------

        if(isParentLine(line))
        {
            RadioactiveParentState state =
                parseParentState(line);

            isotope.addParentState(state);

            currentParent =
                &isotope.parentStates().back();

            continue;
        }

        // ----------------------------------------------------
        // A decay record cannot occur before a parent state.
        // ----------------------------------------------------

        if(currentParent == nullptr)
        {
            throw std::runtime_error(
                "RadioactiveDecayReader: encountered decay "
                "record before a parent-state record in '" +
                path +
                "': '" +
                line +
                "'"
            );
        }

        const auto fields =
            splitFields(line);

        if(fields.empty())
        {
            continue;
        }

        // ----------------------------------------------------
        // Daughter channel
        //
        // Five fields:
        //
        // BetaPlus  583.11  -  41.303  4198.47
        //
        // Six fields are also supported when a beta
        // forbiddenness classification is present.
        // ----------------------------------------------------

        if(fields.size() >= 5)
        {
            RadioactiveDecayChannel channel =
                parseDecayChannel(line);

            // ------------------------------------------------
            // Find the corresponding mode.
            // ------------------------------------------------

            auto mode =
                std::find_if(
                    currentParent->decayModes.begin(),
                    currentParent->decayModes.end(),
                    [&](const RadioactiveDecayMode& candidate)
                    {
                        return candidate.decayType ==
                               channel.decayType;
                    }
                );

            if(mode == currentParent->decayModes.end())
            {
                throw std::runtime_error(
                    "RadioactiveDecayReader: daughter channel "
                    "for decay mode '" +
                    channel.decayType +
                    "' appears before its mode header"
                );
            }

            mode->addChannel(channel);

            continue;
        }

        // ----------------------------------------------------
        // Decay-mode header
        //
        // Three fields:
        //
        // BetaPlus  0  0.99918
        // IT        0  1
        // ----------------------------------------------------

        if(fields.size() == 3)
        {
            RadioactiveDecayMode mode =
                parseDecayMode(line);

            currentParent->addDecayMode(mode);

            continue;
        }

        // ----------------------------------------------------
        // Anything else is malformed.
        // ----------------------------------------------------

        throw std::runtime_error(
            "RadioactiveDecayReader: unrecognised record "
            "in '" +
            path +
            "': '" +
            line +
            "'"
        );
    }

    input.close();

    // ========================================================
    // Basic validation
    // ========================================================

    if(isotope.numberOfParentStates() == 0)
    {
        throw std::runtime_error(
            "RadioactiveDecayReader: no parent states found "
            "in '" +
            path +
            "'"
        );
    }

    // ========================================================
    // Validate decay-mode branching fractions
    //
    // IMPORTANT:
    //
    // We DO NOT require daughter-channel intensities to sum
    // to 100%.
    //
    // The RDM file can contain only a subset of daughter
    // channels.
    //
    // For example:
    //
    //     BetaPlus  583.11   41.303
    //     BetaPlus  657.16   53.184
    //     BetaPlus  1936.45   5.4305
    //
    // sums to 99.9175%, not 100%, and that is valid.
    //
    // We therefore validate only the total branching of the
    // decay modes.
    // ========================================================

    constexpr double branchingTolerance = 1.0e-4;

    for(const auto& parent :
        isotope.parentStates())
    {
        // A parent with no decay modes is allowed.
        if(parent.decayModes.empty())
        {
            continue;
        }

        double totalBranching = 0.0;

        for(const auto& mode :
            parent.decayModes)
        {
            if(mode.totalBranchingFraction < 0.0)
            {
                throw std::runtime_error(
                    "RadioactiveDecayReader: negative branching "
                    "fraction for decay mode '" +
                    mode.decayType +
                    "'"
                );
            }

            totalBranching +=
                mode.totalBranchingFraction;
        }

        if(std::abs(totalBranching - 1.0)
           > branchingTolerance)
        {
            std::ostringstream message;

            message
                << "RadioactiveDecayReader: total decay-mode "
                << "branching fractions for parent at "
                << std::setprecision(12)
                << parent.energy_keV
                << " keV sum to "
                << totalBranching
                << " instead of 1.0";

            throw std::runtime_error(
                message.str()
            );
        }
    }

    return isotope;
}