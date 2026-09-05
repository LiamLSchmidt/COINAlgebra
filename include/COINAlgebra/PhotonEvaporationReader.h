#ifndef COINALGEBRA_PHOTONEVAPORATIONREADER_H
#define COINALGEBRA_PHOTONEVAPORATIONREADER_H

#include <array>
#include <cstddef>
#include <string>
#include <vector>

// ============================================================
// PhotonTransition
// ============================================================
//
// Represents one gamma deexcitation transition from an initial
// nuclear level to a lower daughter level.
//
// This corresponds to one gamma line in the Geant4
// PhotonEvaporation level-gamma data file.
//

struct PhotonTransition
{
    // --------------------------------------------------------
    // Daughter level
    // --------------------------------------------------------

    int daughterLevel;

    // --------------------------------------------------------
    // Gamma transition properties
    // --------------------------------------------------------

    double energy_keV;
    double relativeIntensity;

    // Geant4 multipolarity encoding:
    //
    //   1 = E0
    //   2 = E1
    //   3 = M1
    //   4 = E2
    //   5 = M2
    //   6 = E3
    //   7 = M3
    //
    //   100*Nx + Ny = mixed transition
    //
    // Example:
    //   304 = M1 + E2
    //
    //   0 = unknown multipolarity
    //

    int multipolarity;

    // --------------------------------------------------------
    // Multipolarity mixing ratio
    // --------------------------------------------------------
    //
    // Geant4 uses 0 when no mixing ratio is given in the
    // numerical representation of the data.
    //
    // The original file description refers to this as "O"
    // (not given) in the conceptual format.
    //

    double mixingRatio;

    // --------------------------------------------------------
    // Internal conversion
    // --------------------------------------------------------
    //
    // alpha = Ic / Ig
    //

    double conversionCoefficient;

    // --------------------------------------------------------
    // Partial internal conversion probabilities
    // --------------------------------------------------------
    //
    // Ordered according to the Geant4 documentation:
    //
    //   [0] K
    //   [1] L1
    //   [2] L2
    //   [3] L3
    //   [4] M1
    //   [5] M2
    //   [6] M3
    //   [7] M4
    //   [8] M5
    //   [9] Outer shells
    //
    // These values are zero when the total conversion
    // coefficient is zero or when no partial data are given.
    //

    std::array<double, 10> shellConversionProbabilities{};

    // --------------------------------------------------------
    // Constructors
    // --------------------------------------------------------

    PhotonTransition();

    PhotonTransition(
        int daughterLevel,
        double energy_keV,
        double relativeIntensity,
        int multipolarity,
        double mixingRatio,
        double conversionCoefficient,
        const std::array<double, 10>& shellConversionProbabilities
    );

    // --------------------------------------------------------
    // Probability helpers
    // --------------------------------------------------------

    // Probability that the transition proceeds by gamma
    // emission rather than internal conversion.
    //
    // P(gamma) = 1 / (1 + alpha)
    //
    double gammaProbability() const;

    // Probability that the transition proceeds by internal
    // conversion.
    //
    // P(IC) = alpha / (1 + alpha)
    //
    double conversionProbability() const;
};


// ============================================================
// PhotonLevel
// ============================================================
//
// Represents one nuclear level in a PhotonEvaporation isotope
// file.
//

struct PhotonLevel
{
    // --------------------------------------------------------
    // Level identification
    // --------------------------------------------------------

    int id;

    // --------------------------------------------------------
    // Floating-level flag
    // --------------------------------------------------------
    //
    // Geant4 level files contain values such as:
    //
    //   -
    //   +X
    //   +Y
    //   +Z
    //   +U
    //   ...
    //
    // We preserve this field as a string rather than attempting
    // to interpret it at the reader level.
    //

    std::string floating;

    // --------------------------------------------------------
    // Level properties
    // --------------------------------------------------------

    double energy_keV;

    // Level half-life in seconds.
    //
    // Geant4 uses -1 for a stable ground state.
    //

    double halfLife_s;

    // --------------------------------------------------------
    // JPi information
    // --------------------------------------------------------
    //
    // The sign encodes parity.
    //
    // 99 indicates that JPi is unknown.
    //

    double jpi;

    // --------------------------------------------------------
    // Gamma deexcitation transitions
    // --------------------------------------------------------

    std::vector<PhotonTransition> transitions;

    // --------------------------------------------------------
    // Constructors
    // --------------------------------------------------------

    PhotonLevel();

    PhotonLevel(
        int id,
        const std::string& floating,
        double energy_keV,
        double halfLife_s,
        double jpi
    );

    // --------------------------------------------------------
    // Queries
    // --------------------------------------------------------

    std::size_t numberOfGammas() const;

    bool isStable() const;

    bool hasKnownJPi() const;
};


// ============================================================
// PhotonIsotope
// ============================================================
//
// Represents the complete PhotonEvaporation data for one
// isotope.
//
// For example:
//
//     Z = 11, A = 22
//
// corresponds to:
//
//     PhotonEvaporation5.5/z11.a22
//

class PhotonIsotope
{
public:

    // --------------------------------------------------------
    // Constructor
    // --------------------------------------------------------

    PhotonIsotope();

    PhotonIsotope(
        int atomicNumber,
        int massNumber
    );

    // --------------------------------------------------------
    // Isotope identification
    // --------------------------------------------------------

    int atomicNumber() const;

    int massNumber() const;

    // --------------------------------------------------------
    // Level access
    // --------------------------------------------------------

    const std::vector<PhotonLevel>& levels() const;

    std::vector<PhotonLevel>& levels();

    std::size_t numberOfLevels() const;

    // --------------------------------------------------------
    // Level lookup
    // --------------------------------------------------------

    const PhotonLevel& level(int id) const;

    PhotonLevel& level(int id);

    // --------------------------------------------------------
    // Add level
    // --------------------------------------------------------

    void addLevel(const PhotonLevel& level);

private:

    int fAtomicNumber;
    int fMassNumber;

    std::vector<PhotonLevel> fLevels;
};


// ============================================================
// PhotonEvaporationReader
// ============================================================
//
// Reader for the Geant4 PhotonEvaporation level-gamma database.
//
// Example:
//
//     PhotonEvaporationReader reader(
//         "PhotonEvaporation5.5"
//     );
//
//     PhotonIsotope na22 = reader.read(11, 22);
//

class PhotonEvaporationReader
{
public:

    // --------------------------------------------------------
    // Constructor
    // --------------------------------------------------------

    explicit PhotonEvaporationReader(
        const std::string& dataDirectory
    );

    // --------------------------------------------------------
    // Read isotope
    // --------------------------------------------------------
    //
    // Reads:
    //
    //     <dataDirectory>/z<Z>.a<A>
    //
    // For example:
    //
    //     read(11, 22)
    //
    // reads:
    //
    //     PhotonEvaporation5.5/z11.a22
    //

    PhotonIsotope read(
        int atomicNumber,
        int massNumber
    ) const;

    // --------------------------------------------------------
    // File path
    // --------------------------------------------------------
    //
    // Returns the expected path for an isotope data file.
    //

    std::string filePath(
        int atomicNumber,
        int massNumber
    ) const;

    // --------------------------------------------------------
    // Data directory
    // --------------------------------------------------------

    const std::string& dataDirectory() const;

private:

    std::string fDataDirectory;

    // --------------------------------------------------------
    // Parsing helpers
    // --------------------------------------------------------

    PhotonLevel parseLevel(
        const std::string& line
    ) const;

    PhotonTransition parseTransition(
        const std::string& line
    ) const;
};

#endif // COINALGEBRA_PHOTONEVAPORATIONREADER_H