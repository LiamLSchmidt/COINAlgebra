#ifndef COINALGEBRA_DETECTIONMAPS_H
#define COINALGEBRA_DETECTIONMAPS_H

#include "COINAlgebra/Core/DecayVector.h"
class PathAlgebra;

// Isotropic, identical-detector model of paper Section II.B, Eqs. (28)-(32).
// Efficiencies are whole-array probabilities CONDITIONAL on gamma emission.
// Total efficiency includes full-energy and partial-energy (Compton) events.
// Maps are copied, keyed by transition name; 0 <= peak <= total <= 1.
// An empty conversion map means no IC. Otherwise every used edge needs alpha.
class DetectionMaps
{
public:
    using EfficiencyMap = std::unordered_map<std::string, double>;
    DetectionMaps(const EfficiencyMap& peakEfficiencies,
                  const EfficiencyMap& totalEfficiencies,
                  std::size_t detectorCount,
                  const EfficiencyMap& conversionCoefficients = {});

    std::size_t DetectorCount() const;
    // Each map preserves stationary terms, and applies the product of its
    // edge factors to longer paths. Input coefficients must contain IC-inclusive
    // physical branches, not already gamma-emission-weighted branches.
    DecayVector FullEnergyHit(const DecayVector& vector) const; // q * peak
    DecayVector TotalHit(const DecayVector& vector) const;     // q * total / N
    DecayVector SummingOut(const DecayVector& vector) const;   // 1 - q * total / N

    // Positive powers only: h + h^2/N + h^3/N^2 + ... . Input must consist
    // of single edges. Connected summed paths remain separate basis terms.
    DecayVector SummingInExpansion(const PathAlgebra& algebra,
                                  const DecayVector& transition) const;
private:
    EfficiencyMap fPeak, fTotal, fConversion;
    std::size_t fDetectorCount;
    DecayVector Apply(const DecayVector& vector, int kind) const;
};
#endif
