#include "COINAlgebra/Detection/DetectionMaps.h"
#include "COINAlgebra/Algebra/PathAlgebra.h"
#include <cmath>
#include <stdexcept>

namespace {
double Require(const DetectionMaps::EfficiencyMap& values, const std::string& name)
{
    const auto entry = values.find(name);
    if (entry == values.end())
        throw std::invalid_argument("DetectionMaps: missing transition '" + name + "'");
    return entry->second;
}
}

DetectionMaps::DetectionMaps(const EfficiencyMap& peak, const EfficiencyMap& total,
                             std::size_t detectors, const EfficiencyMap& conversion)
    : fPeak(peak), fTotal(total), fConversion(conversion), fDetectorCount(detectors)
{
    if (detectors == 0)
        throw std::invalid_argument("DetectionMaps: detector count must be positive");
    for (const auto* efficiencies : {&fPeak, &fTotal})
        for (const auto& entry : *efficiencies)
            if (!std::isfinite(entry.second) || entry.second < 0.0 || entry.second > 1.0)
                throw std::invalid_argument("DetectionMaps: efficiency must be finite and in [0,1]");
    for (const auto& entry : fPeak)
        if (entry.second > Require(fTotal, entry.first))
            throw std::invalid_argument("DetectionMaps: peak efficiency exceeds total efficiency");
    for (const auto& entry : fConversion)
        if (!std::isfinite(entry.second) || entry.second < 0.0)
            throw std::invalid_argument("DetectionMaps: alpha must be finite and nonnegative");
}

std::size_t DetectionMaps::DetectorCount() const { return fDetectorCount; }

DecayVector DetectionMaps::Apply(const DecayVector& vector, int kind) const
{
    EfficiencyMap factors;
    for (const auto& term : vector.GetTerms())
        for (const auto& edge : term.path.GetTransitions()) {
            const auto& name = edge.GetName();
            const double q = fConversion.empty() ? 1.0 : 1.0 / (1.0 + Require(fConversion, name));
            const double hit = kind == 0 ? q * Require(fPeak, name)
                : q * Require(fTotal, name) / static_cast<double>(fDetectorCount);
            factors[name] = kind == 2 ? 1.0 - hit : hit;
        }
    return vector.ApplyDetectionMap(factors);
}

DecayVector DetectionMaps::FullEnergyHit(const DecayVector& vector) const { return Apply(vector, 0); }
DecayVector DetectionMaps::TotalHit(const DecayVector& vector) const { return Apply(vector, 1); }
DecayVector DetectionMaps::SummingOut(const DecayVector& vector) const { return Apply(vector, 2); }

DecayVector DetectionMaps::SummingInExpansion(const PathAlgebra& algebra,
                                             const DecayVector& transition) const
{
    for (const auto& term : transition.GetTerms())
        if (term.path.Length() != 1)
            throw std::invalid_argument("DetectionMaps: summing expansion requires single edges");
    const auto hit = FullEnergyHit(transition);
    const auto nextHit = hit * (1.0 / static_cast<double>(fDetectorCount));
    DecayVector result, power = hit;
    for (std::size_t k = 1; k <= algebra.MaxPower() && !power.Empty(); ++k) {
        result += power;
        power = algebra.Multiply(power, nextHit);
    }
    return result;
}
