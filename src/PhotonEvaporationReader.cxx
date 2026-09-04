#include "COINAlgebra/PhotonEvaporationReader.h"

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <cctype>


// ============================================================
// Helper functions
// ============================================================

namespace
{

bool isWhitespaceOnly(const std::string& line)
{
    return std::all_of(
        line.begin(),
        line.end(),
        [](unsigned char c)
        {
            return std::isspace(c);
        }
    );
}

std::string trim(const std::string& value)
{
    const auto first = value.find_first_not_of(" \t\r\n");

    if(first == std::string::npos)
    {
        return "";
    }

    const auto last = value.find_last_not_of(" \t\r\n");

    return value.substr(
        first,
        last - first + 1
    );
}

}


// ============================================================
// PhotonTransition
// ============================================================

PhotonTransition::PhotonTransition()
    : daughterLevel(-1),
      energy_keV(0.0),
      relativeIntensity(0.0),
      multipolarity(0),
      mixingRatio(0.0),
      conversionCoefficient(0.0),
      shellConversionProbabilities{}
{
}


PhotonTransition::PhotonTransition(
    int daughterLevel_,
    double energy_keV_,
    double relativeIntensity_,
    int multipolarity_,
    double mixingRatio_,
    double conversionCoefficient_,
    const std::array<double, 10>& shellConversionProbabilities_
)
    : daughterLevel(daughterLevel_),
      energy_keV(energy_keV_),
      relativeIntensity(relativeIntensity_),
      multipolarity(multipolarity_),
      mixingRatio(mixingRatio_),
      conversionCoefficient(conversionCoefficient_),
      shellConversionProbabilities(
          shellConversionProbabilities_
      )
{
}


// ------------------------------------------------------------
// Gamma probability
// ------------------------------------------------------------

double PhotonTransition::gammaProbability() const
{
    return 1.0 / (1.0 + conversionCoefficient);
}


// ------------------------------------------------------------
// Internal conversion probability
// ------------------------------------------------------------

double PhotonTransition::conversionProbability() const
{
    return conversionCoefficient /
           (1.0 + conversionCoefficient);
}


// ============================================================
// PhotonLevel
// ============================================================

PhotonLevel::PhotonLevel()
    : id(-1),
      floating("-"),
      energy_keV(0.0),
      halfLife_s(-1.0),
      jpi(99.0),
      transitions{}
{
}


PhotonLevel::PhotonLevel(
    int id_,
    const std::string& floating_,
    double energy_keV_,
    double halfLife_s_,
    double jpi_
)
    : id(id_),
      floating(floating_),
      energy_keV(energy_keV_),
      halfLife_s(halfLife_s_),
      jpi(jpi_),
      transitions{}
{
}


// ------------------------------------------------------------
// Number of gamma transitions
// ------------------------------------------------------------

std::size_t PhotonLevel::numberOfGammas() const
{
    return transitions.size();
}


// ------------------------------------------------------------
// Stable level
// ------------------------------------------------------------

bool PhotonLevel::isStable() const
{
    return halfLife_s < 0.0;
}


// ------------------------------------------------------------
// Known JPi
// ------------------------------------------------------------

bool PhotonLevel::hasKnownJPi() const
{
    return jpi != 99.0;
}


// ============================================================
// PhotonIsotope
// ============================================================

PhotonIsotope::PhotonIsotope()
    : fAtomicNumber(0),
      fMassNumber(0),
      fLevels{}
{
}


PhotonIsotope::PhotonIsotope(
    int atomicNumber_,
    int massNumber_
)
    : fAtomicNumber(atomicNumber_),
      fMassNumber(massNumber_),
      fLevels{}
{
}


// ------------------------------------------------------------
// Atomic number
// ------------------------------------------------------------

int PhotonIsotope::atomicNumber() const
{
    return fAtomicNumber;
}


// ------------------------------------------------------------
// Mass number
// ------------------------------------------------------------

int PhotonIsotope::massNumber() const
{
    return fMassNumber;
}


// ------------------------------------------------------------
// Levels
// ------------------------------------------------------------

const std::vector<PhotonLevel>&
PhotonIsotope::levels() const
{
    return fLevels;
}


std::vector<PhotonLevel>&
PhotonIsotope::levels()
{
    return fLevels;
}


// ------------------------------------------------------------
// Number of levels
// ------------------------------------------------------------

std::size_t PhotonIsotope::numberOfLevels() const
{
    return fLevels.size();
}


// ------------------------------------------------------------
// Level lookup
// ------------------------------------------------------------

const PhotonLevel&
PhotonIsotope::level(int id) const
{
    for(const auto& level : fLevels)
    {
        if(level.id == id)
        {
            return level;
        }
    }

    throw std::out_of_range(
        "PhotonIsotope::level(): level ID " +
        std::to_string(id) +
        " does not exist."
    );
}


PhotonLevel&
PhotonIsotope::level(int id)
{
    for(auto& level : fLevels)
    {
        if(level.id == id)
        {
            return level;
        }
    }

    throw std::out_of_range(
        "PhotonIsotope::level(): level ID " +
        std::to_string(id) +
        " does not exist."
    );
}


// ------------------------------------------------------------
// Add level
// ------------------------------------------------------------

void PhotonIsotope::addLevel(
    const PhotonLevel& level_
)
{
    fLevels.push_back(level_);
}


// ============================================================
// PhotonEvaporationReader
// ============================================================

PhotonEvaporationReader::PhotonEvaporationReader(
    const std::string& dataDirectory
)
    : fDataDirectory(dataDirectory)
{
    if(fDataDirectory.empty())
    {
        throw std::invalid_argument(
            "PhotonEvaporationReader: "
            "data directory cannot be empty."
        );
    }
}


// ------------------------------------------------------------
// Data directory
// ------------------------------------------------------------

const std::string&
PhotonEvaporationReader::dataDirectory() const
{
    return fDataDirectory;
}


// ------------------------------------------------------------
// Construct isotope filename
// ------------------------------------------------------------

std::string PhotonEvaporationReader::filePath(
    int atomicNumber,
    int massNumber
) const
{
    if(atomicNumber <= 0)
    {
        throw std::invalid_argument(
            "PhotonEvaporationReader::filePath(): "
            "atomic number must be positive."
        );
    }

    if(massNumber <= 0)
    {
        throw std::invalid_argument(
            "PhotonEvaporationReader::filePath(): "
            "mass number must be positive."
        );
    }

    std::ostringstream path;

    path << fDataDirectory;

    if(!fDataDirectory.empty() &&
       fDataDirectory.back() != '/')
    {
        path << '/';
    }

    path << "z"
         << atomicNumber
         << ".a"
         << massNumber;

    return path.str();
}


// ============================================================
// Parse a level header
// ============================================================
//
// Expected format:
//
//   levelID floating energy halfLife JPi nGammas
//
// Example:
//
//   3 - 4402 2.1e-14 2.0 3
//
// ============================================================

PhotonLevel PhotonEvaporationReader::parseLevel(
    const std::string& line
) const
{
    std::istringstream stream(line);

    int id;
    std::string floating;
    double energy;
    double halfLife;
    double jpi;
    int nGammas;

    if(!(stream
        >> id
        >> floating
        >> energy
        >> halfLife
        >> jpi
        >> nGammas))
    {
        throw std::runtime_error(
            "PhotonEvaporationReader: "
            "malformed level line:\n" +
            line
        );
    }

    if(id < 0)
    {
        throw std::runtime_error(
            "PhotonEvaporationReader: "
            "level ID cannot be negative:\n" +
            line
        );
    }

    if(nGammas < 0)
    {
        throw std::runtime_error(
            "PhotonEvaporationReader: "
            "number of gammas cannot be negative:\n" +
            line
        );
    }

    return PhotonLevel(
        id,
        floating,
        energy,
        halfLife,
        jpi
    );
}


// ============================================================
// Parse gamma transition
// ============================================================
//
// Expected format:
//
//   daughterLevel
//   energy
//   relativeIntensity
//   multipolarity
//   mixingRatio
//   alpha
//   K
//   L1
//   L2
//   L3
//   M1
//   M2
//   M3
//   M4
//   M5
//   outer
//
// ============================================================

PhotonTransition
PhotonEvaporationReader::parseTransition(
    const std::string& line
) const
{
    std::istringstream stream(line);

    int daughterLevel;
    double energy;
    double intensity;
    int multipolarity;

    std::string mixingRatioString;

    double conversionCoefficient;

    std::array<double, 10> shellProbabilities{};

    if(!(stream
        >> daughterLevel
        >> energy
        >> intensity
        >> multipolarity
        >> mixingRatioString
        >> conversionCoefficient))
    {
        throw std::runtime_error(
            "PhotonEvaporationReader: "
            "malformed gamma transition line:\n" +
            line
        );
    }

    // --------------------------------------------------------
    // Mixing ratio
    // --------------------------------------------------------
    //
    // The README refers to "O" for unavailable mixing ratios.
    //
    // The actual files observed so far use numeric values,
    // commonly 0.
    //
    // Preserve the numerical representation as 0 when the
    // mixing ratio is unavailable.
    //

    double mixingRatio = 0.0;

    if(mixingRatioString != "O" &&
       mixingRatioString != "o")
    {
        try
        {
            std::size_t position = 0;

            mixingRatio =
                std::stod(
                    mixingRatioString,
                    &position
                );

            if(position != mixingRatioString.size())
            {
                throw std::invalid_argument(
                    "unused characters"
                );
            }
        }
        catch(const std::exception&)
        {
            throw std::runtime_error(
                "PhotonEvaporationReader: "
                "invalid mixing ratio '" +
                mixingRatioString +
                "' in transition line:\n" +
                line
            );
        }
    }

    // --------------------------------------------------------
    // Partial conversion probabilities
    // --------------------------------------------------------
    //
    // These fields are optional according to the Geant4
    // documentation. Therefore we read whatever is present.
    //
    // Missing values remain zero.
    //

    for(std::size_t i = 0;
        i < shellProbabilities.size();
        ++i)
    {
        if(!(stream >> shellProbabilities[i]))
        {
            break;
        }
    }

    return PhotonTransition(
        daughterLevel,
        energy,
        intensity,
        multipolarity,
        mixingRatio,
        conversionCoefficient,
        shellProbabilities
    );
}


// ============================================================
// Read isotope
// ============================================================

PhotonIsotope PhotonEvaporationReader::read(
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
            "PhotonEvaporationReader: "
            "could not open PhotonEvaporation file:\n" +
            path
        );
    }

    PhotonIsotope isotope(
        atomicNumber,
        massNumber
    );

    std::string line;

    std::size_t lineNumber = 0;

    while(std::getline(input, line))
    {
        ++lineNumber;

        // ----------------------------------------------------
        // Ignore empty lines
        // ----------------------------------------------------

        if(trim(line).empty())
        {
            continue;
        }

        // ----------------------------------------------------
        // Level header
        // ----------------------------------------------------

        PhotonLevel level;

        int nGammas;

        {
            std::istringstream stream(line);

            int id;
            std::string floating;
            double energy;
            double halfLife;
            double jpi;

            if(!(stream
                >> id
                >> floating
                >> energy
                >> halfLife
                >> jpi
                >> nGammas))
            {
                throw std::runtime_error(
                    "PhotonEvaporationReader: "
                    "malformed level header at line " +
                    std::to_string(lineNumber) +
                    ":\n" +
                    line
                );
            }

            level = PhotonLevel(
                id,
                floating,
                energy,
                halfLife,
                jpi
            );
        }

        // ----------------------------------------------------
        // Read transitions belonging to this level
        // ----------------------------------------------------

        for(int i = 0;
            i < nGammas;
            ++i)
        {
            if(!std::getline(input, line))
            {
                throw std::runtime_error(
                    "PhotonEvaporationReader: "
                    "unexpected end of file while reading "
                    "gamma transitions for level " +
                    std::to_string(level.id) +
                    " in " +
                    path
                );
            }

            ++lineNumber;

            if(trim(line).empty())
            {
                throw std::runtime_error(
                    "PhotonEvaporationReader: "
                    "empty gamma transition line at line " +
                    std::to_string(lineNumber) +
                    " for level " +
                    std::to_string(level.id)
                );
            }

            try
            {
                level.transitions.push_back(
                    parseTransition(line)
                );
            }
            catch(const std::exception& error)
            {
                throw std::runtime_error(
                    "PhotonEvaporationReader: "
                    "error parsing transition at line " +
                    std::to_string(lineNumber) +
                    ": " +
                    error.what()
                );
            }
        }

        isotope.addLevel(level);
    }

    // --------------------------------------------------------
    // Validate level IDs
    // --------------------------------------------------------
    //
    // All daughter levels referenced by gamma transitions
    // should exist in the isotope.
    //

    for(const auto& level : isotope.levels())
    {
        for(const auto& transition : level.transitions)
        {
            bool found = false;

            for(const auto& daughter : isotope.levels())
            {
                if(daughter.id == transition.daughterLevel)
                {
                    found = true;
                    break;
                }
            }

            if(!found)
            {
                throw std::runtime_error(
                    "PhotonEvaporationReader: "
                    "level " +
                    std::to_string(level.id) +
                    " contains a transition to nonexistent "
                    "daughter level " +
                    std::to_string(
                        transition.daughterLevel
                    )
                );
            }
        }
    }

    // --------------------------------------------------------
    // Validate level ordering
    // --------------------------------------------------------
    //
    // PhotonEvaporation files use level indices beginning at
    // zero. We don't require that every integer index appears,
    // but we do require the ground state (level 0) to exist.
    //

    bool hasGroundState = false;

    for(const auto& level : isotope.levels())
    {
        if(level.id == 0)
        {
            hasGroundState = true;
            break;
        }
    }

    if(!hasGroundState)
    {
        throw std::runtime_error(
            "PhotonEvaporationReader: "
            "isotope file contains no level 0."
        );
    }

    return isotope;
}