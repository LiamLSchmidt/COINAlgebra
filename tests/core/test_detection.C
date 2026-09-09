#include "COINAlgebra/Core/DecayQuiver.h"
#include "COINAlgebra/Algebra/PathAlgebra.h"
#include "COINAlgebra/Algebra/PathProjectors.h"
#include "COINAlgebra/Probability/DecayProbability.h"
#include <cassert>
#include <cmath>
#include <limits>
#include <stdexcept>
#include "COINAlgebra/Builders/DecayQuiverBuilder.h"

void test_detection()
{
    DecayQuiver quiver;
    auto* top = quiver.AddLevel("top");
    auto* middle = quiver.AddLevel("middle");
    auto* ground = quiver.AddLevel("ground");
    DecayPath a(*quiver.AddTransition("a", top, middle, 0.5));
    DecayPath b(*quiver.AddTransition("b", middle, ground, 0.8));
    DecayVector decay(DecayPath(top), 1.0);
    decay.AddTerm(a, 0.5);
    decay.AddTerm(b, 0.8);
    PathAlgebra algebra(quiver);
    PathProjectors projectors;
    DecayProbability probability(algebra, projectors);
    // Even an entirely invisible upstream transition feeds the lower level.
    const auto detected = probability.DetectionFeedingVector(
        decay, {{"a", 0.0}, {"b", 0.25}});
    assert(std::abs(algebra.PathForm(detected, DecayVector(a))) < 1e-12);
    assert(std::abs(algebra.PathForm(detected, DecayVector(b)) - 0.1) < 1e-12);
    const auto perfect = probability.DetectionFeedingVector(
        decay, {{"a", 1.0}, {"b", 1.0}});
    assert(std::abs(algebra.PathForm(perfect, DecayVector(b)) - 0.4) < 1e-12);

    const std::unordered_map<std::string, double> alpha{{"a", 3.0}, {"b", 1.0}};
    const auto emitted = probability.EmissionFeedingVector(decay, alpha);
    assert(std::abs(algebra.PathForm(emitted, DecayVector(a)) - 0.125) < 1e-12);
    // Upstream conversion does not attenuate downstream population.
    assert(std::abs(algebra.PathForm(emitted, DecayVector(b)) - 0.2) < 1e-12);
    const auto gammaDetected = probability.DetectionFeedingVector(
        decay, {{"a", 0.5}, {"b", 0.25}}, alpha);
    assert(std::abs(algebra.PathForm(gammaDetected, DecayVector(b)) - 0.05) < 1e-12);
    const auto noConversion = probability.EmissionFeedingVector(
        decay, {{"a", 0.0}, {"b", 0.0}});
    assert(std::abs(algebra.PathForm(noConversion, DecayVector(b)) - 0.4) < 1e-12);

    // Exercise longer paths, including the algebraic feeding fallback.
    DecayPath ab(std::vector<DecayTransition>{a.GetTransitions()[0], b.GetTransitions()[0]});
    DecayVector longDecay(DecayPath(top), 1.0);
    longDecay.AddTerm(ab, 0.4);
    const auto longEmission = probability.EmissionFeedingVector(longDecay, alpha);
    assert(std::abs(algebra.PathForm(longEmission, DecayVector(ab)) - 0.05) < 1e-12);
    const auto mapped = longDecay.ApplyConversionMap(alpha);
    assert(std::abs(algebra.PathForm(mapped, DecayVector(DecayPath(top))) - 1.0) < 1e-12);
    for (double bad : {-1.0, std::numeric_limits<double>::infinity(),
                       std::numeric_limits<double>::quiet_NaN()})
    {
        bool rejected = false;
        try { decay.ApplyConversionMap({{"a", bad}, {"b", 0.0}}); }
        catch (const std::invalid_argument&) { rejected = true; }
        assert(rejected);
    }
    bool missingRejected = false;
    try { decay.ApplyConversionMap({{"a", 0.0}}); }
    catch (const std::invalid_argument&) { missingRejected = true; }
    assert(missingRejected);

    // Equal gamma intensities with alpha=3 and alpha=0 imply total
    // branches 4/5 and 1/5, not 1/2 each.
    PhotonIsotope daughter(1, 3);
    PhotonLevel l0(0, "-", 0.0, -1.0, 0.0);
    PhotonLevel l1(1, "-", 100.0, 1.0, 0.0);
    PhotonLevel l2(2, "-", 200.0, 1.0, 0.0);
    l1.transitions.emplace_back(0, 100.0, 100.0, 3, 0.0, 1.0, std::array<double, 10>{});
    l2.transitions.emplace_back(1, 100.0, 100.0, 3, 0.0, 3.0, std::array<double, 10>{});
    l2.transitions.emplace_back(0, 200.0, 100.0, 3, 0.0, 0.0, std::array<double, 10>{});
    daughter.addLevel(l0);
    daughter.addLevel(l1);
    daughter.addLevel(l2);
    RadioactiveIsotope parent(2, 3);
    RadioactiveParentState state(0.0, "-", 1.0);
    RadioactiveDecayMode mode("KshellEC", 1.0);
    mode.addChannel(RadioactiveDecayChannel("KshellEC", 200.0, "-", 100.0, 1000.0));
    state.addDecayMode(mode);
    parent.addParentState(state);
    std::unordered_map<std::string, double> extracted{{"stale", 10.0}};
    auto built = DecayQuiverBuilder::BuildGammaQuiver(parent, daughter, extracted, "EC");
    assert(extracted.size() == 3);
    assert(extracted.at("gamma_200_to_100_0") == 3.0);
    assert(std::abs(built.GetTransition("gamma_200_to_100_0")->GetProbability() - 0.8) < 1e-12);
    assert(std::abs(built.GetTransition("gamma_200_to_0_1")->GetProbability() - 0.2) < 1e-12);
    auto legacy = DecayQuiverBuilder::BuildGammaQuiver(parent, daughter, "EC");
    assert(std::abs(legacy.GetTransition("gamma_200_to_100_0")->GetProbability() - 0.8) < 1e-12);
}
