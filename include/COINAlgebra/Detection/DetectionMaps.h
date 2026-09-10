#ifndef COINALGEBRA_DETECTIONMAPS_H
#define COINALGEBRA_DETECTIONMAPS_H

#include "COINAlgebra/Core/DecayVector.h"
#include "COINAlgebra/Algebra/CAlgebra.h"
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

    // Avoid all of m distinct detectors: product_e (1 - m*q_e*total_e/N).
    // Useful for resolved coincidences where every selected detector must stay
    // free of unobserved deposits. Requires m <= N; m=0 is the identity.
    DecayVector AvoidDetectors(const DecayVector& vector, std::size_t m) const;

    // Diagonal maps on observed coincidence factors only. They preserve
    // stationary markers and do NOT change the connection weights of a fiber.
    // Mapping the base decay and constructing a new CAlgebra is a separate step.
    CAlgebra::Vector FullEnergyHit(const CAlgebra::Vector& vector) const;
    CAlgebra::Vector TotalHit(const CAlgebra::Vector& vector) const;
    CAlgebra::Vector SummingOut(const CAlgebra::Vector& vector) const;

    // Eq. (69): h + h *_fiber h/N + ... including disconnected hits.
    // Pass the summing-out fiber C_{o(tau)} for Eq. (70), and the ORIGINAL
    // physical single-edge transition vector. No stationary input is allowed.
    // To form Gamma_1, multiply Embed(b), this result, then Embed(sink paths)
    // in that same fiber. Coefficients are expected peak counts per decay.
    CAlgebra::Vector SummingInExpansion(const CAlgebra& fiber,
                                       const DecayVector& transition) const;

    // Positive powers only: h + h^2/N + h^3/N^2 + ... . Input must consist
    // of single edges. Connected summed paths remain separate basis terms.
    DecayVector SummingInExpansion(const PathAlgebra& algebra,
                                  const DecayVector& transition) const;
private:
    EfficiencyMap fPeak, fTotal, fConversion;
    std::size_t fDetectorCount;
    DecayVector Apply(const DecayVector& vector, int kind, std::size_t avoided = 1) const;
    CAlgebra::Vector Apply(const CAlgebra::Vector& vector, int kind) const;
};
#endif
