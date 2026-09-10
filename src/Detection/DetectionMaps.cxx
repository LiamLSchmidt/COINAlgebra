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

DecayVector DetectionMaps::Apply(const DecayVector& vector, int kind, std::size_t avoided) const
{
    EfficiencyMap factors;
    for (const auto& term : vector.GetTerms())
        for (const auto& edge : term.path.GetTransitions()) {
            const auto& name = edge.GetName();
            const double q = fConversion.empty() ? 1.0 : 1.0 / (1.0 + Require(fConversion, name));
            const double hit = kind == 0 ? q * Require(fPeak, name)
                : q * Require(fTotal, name) / static_cast<double>(fDetectorCount);
            factors[name] = kind == 2 ? 1.0 - static_cast<double>(avoided) * hit : hit;
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

CAlgebra::Vector DetectionMaps::Apply(const CAlgebra::Vector& vector, int kind) const
{
    CAlgebra::Vector result;
    for (const auto& term : vector) {
        if (term.coin.Empty()) continue;
        double weight = term.coefficient;
        for (const auto& factor : term.coin.GetFactors()) {
            const auto mapped = Apply(DecayVector(factor), kind);
            if (mapped.Empty()) { weight = 0.0; break; }
            weight *= mapped.GetTerms().front().coefficient;
        }
        if (weight != 0.0) result.push_back({term.coin, weight});
    }
    return result;
}
CAlgebra::Vector DetectionMaps::FullEnergyHit(const CAlgebra::Vector& v) const { return Apply(v, 0); }
CAlgebra::Vector DetectionMaps::TotalHit(const CAlgebra::Vector& v) const { return Apply(v, 1); }
CAlgebra::Vector DetectionMaps::SummingOut(const CAlgebra::Vector& v) const { return Apply(v, 2); }

CAlgebra::Vector DetectionMaps::SummingInExpansion(const CAlgebra& fiber,
                                                  const DecayVector& transition) const
{
    for (const auto& term : transition.GetTerms())
        if (term.path.Length() != 1)
            throw std::invalid_argument("DetectionMaps: summing expansion requires single edges");
    const auto hit = fiber.Embed(FullEnergyHit(transition));
    auto nextHit = hit;
    for (auto& term : nextHit) term.coefficient /= static_cast<double>(fDetectorCount);
    CAlgebra::Vector result, power = hit;
    // Every multiplication adds a strictly descending transition; nilpotence
    // follows from the finite level order validated by CAlgebra.
    while (!power.empty()) {
        result.insert(result.end(), power.begin(), power.end());
        power = fiber.Multiply(power, nextHit);
    }
    return result;
}

DecayVector DetectionMaps::AvoidDetectors(const DecayVector& vector, std::size_t m) const
{
    if (m > fDetectorCount)
        throw std::invalid_argument("DetectionMaps: avoided detector count exceeds array size");
    if (m == 0) return vector;
    return Apply(vector, 2, m);
}
