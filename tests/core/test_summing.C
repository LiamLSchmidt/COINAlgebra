#include "COINAlgebra/Core/DecayQuiver.h"
#include "COINAlgebra/Algebra/PathAlgebra.h"
#include "COINAlgebra/Algebra/PathProjectors.h"
#include "COINAlgebra/Probability/DecayProbability.h"
#include "COINAlgebra/Detection/DetectionMaps.h"
#include <cassert>
#include <cmath>
#include <functional>
#include <limits>
#include <stdexcept>

namespace {
void near(double actual, double expected) { assert(std::abs(actual - expected) < 1e-12); }
template<class F> void rejects(F action) {
    bool threw = false;
    try { action(); } catch (const std::invalid_argument&) { threw = true; }
    assert(threw);
}
double coefficient(const DecayVector& vector, const DecayPath& path) {
    double result = 0;
    for (const auto& term : vector.GetTerms()) if (term.path == path) result += term.coefficient;
    return result;
}
}

void test_summing()
{
    DecayQuiver quiver;
    auto* top = quiver.AddLevel("top");
    auto* middle = quiver.AddLevel("middle");
    auto* ground = quiver.AddLevel("ground");
    DecayPath a(*quiver.AddTransition("a", top, middle));
    DecayPath b(*quiver.AddTransition("b", middle, ground));
    DecayPath ab = a.Compose(b);
    DecayVector decay(DecayPath(top), 1.0);
    decay.AddTerm(a, 1); decay.AddTerm(b, 1);
    PathAlgebra algebra(quiver);
    PathProjectors projectors;
    DecayProbability probability(algebra, projectors);
    DetectionMaps response({{"a", 0.4}, {"b", 0.3}}, {{"a", 0.8}, {"b", 0.6}}, 2);
    const auto summed = probability.SummingFeedingVector(decay, response);
    near(coefficient(summed, a), 0.4 * (1 - 0.6/2)); // loss from below
    near(coefficient(summed, b), (1 - 0.8/2) * 0.3); // loss from above
    near(coefficient(summed, ab), 0.4 * 0.3 / 2);
    near(algebra.PathForm(summed, DecayVector(ab)), 0.06);
    const auto out = response.SummingOut(decay);
    const auto any = response.TotalHit(decay);
    near(coefficient(out, a) + coefficient(any, a), 1);
    near(coefficient(out, DecayPath(top)), 1);
    near(coefficient(response.SummingOut(DecayVector(ab)), ab), 0.6 * 0.7);

    // IC belongs inside the complement; conversion must not itself cause loss.
    DetectionMaps ic({{"a", 0.4}, {"b", 0.3}}, {{"a", 0.8}, {"b", 0.6}}, 2,
                     {{"a", 3}, {"b", 1}});
    const auto icSummed = probability.SummingFeedingVector(decay, ic);
    near(coefficient(icSummed, a), 0.1 * 0.85);
    near(coefficient(icSummed, b), 0.9 * 0.15);
    near(coefficient(icSummed, ab), 0.1 * 0.15 / 2);
    DetectionMaps compton({{"a", 0.4}, {"b", 0}}, {{"a", 0.8}, {"b", 0.6}}, 1);
    near(coefficient(probability.SummingFeedingVector(decay, compton), a), 0.4 * 0.4);
    DetectionMaps perfect({{"a", 1}, {"b", 1}}, {{"a", 1}, {"b", 1}}, 1);
    const auto single = probability.SummingFeedingVector(decay, perfect);
    assert(single.Size() == 1); near(coefficient(single, ab), 1);
    DetectionMaps four({{"a", 1}, {"b", 1}}, {{"a", 1}, {"b", 1}}, 4);
    const auto fourResult = probability.SummingFeedingVector(decay, four);
    near(coefficient(fourResult, a), 0.75); near(coefficient(fourResult, ab), 0.25);
    DetectionMaps zero({{"a", 0}, {"b", 0}}, {{"a", 0}, {"b", 0}}, 1);
    assert(probability.SummingFeedingVector(decay, zero).Empty());
    rejects([&] { DetectionMaps bad({}, {}, 0); });
    rejects([&] { DetectionMaps bad({{"a", 0.9}}, {{"a", 0.8}}, 1); });
    for (double bad : {-0.1, 1.1, std::numeric_limits<double>::quiet_NaN()})
        rejects([&] { DetectionMaps invalid({{"a", 0}}, {{"a", bad}}, 1); });
    rejects([&] { DetectionMaps incomplete({}, {}, 1); probability.SummingFeedingVector(decay, incomplete); });
    rejects([&] { response.SummingInExpansion(algebra, decay); });
    rejects([&] { probability.SummingFeedingVector(decay * 0.5, response); });

    // Independent reference: enumerate complete physical cascades, then all
    // contiguous nonempty subsets of observed photons. Compare path outputs.
    DecayQuiver q;
    std::vector<DecayLevel*> levels;
    for (const auto* name : {"3", "2", "1", "0", "other_terminal"}) levels.push_back(q.AddLevel(name));
    struct Edge { DecayTransition* edge; double weight, peak, total, alpha; };
    std::vector<Edge> edges;
    const auto add = [&](const char* name, int from, int to, double w, double p, double t, double alpha) {
        // Deliberately different stored probability: the vector coefficients
        // define the physical model for these calculations.
        edges.push_back({q.AddTransition(name, levels[from], levels[to], 0.5), w, p, t, alpha});
    };
    add("32", 0, 1, 0.6, 0.4, 0.8, 1);
    add("31", 0, 2, 0.3, 0.5, 0.7, 0);
    add("30", 0, 3, 0.1, 0.2, 0.6, 0.5);
    add("21", 1, 2, 0.7, 0.3, 0.9, 0);
    add("20", 1, 3, 0.3, 0.6, 0.8, 2);
    add("10", 2, 3, 0.8, 0.7, 0.95, 0.3);
    add("1x", 2, 4, 0.2, 0.1, 0.4, 0);
    DecayVector d;
    d.AddTerm(DecayPath(levels[0]), 0.75); d.AddTerm(DecayPath(levels[1]), 0.25);
    DetectionMaps::EfficiencyMap peaks, totals, alphas;
    for (const auto& e : edges) {
        d.AddTerm(DecayPath(*e.edge), e.weight);
        peaks[e.edge->GetName()] = e.peak; totals[e.edge->GetName()] = e.total; alphas[e.edge->GetName()] = e.alpha;
    }
    PathAlgebra pa(q);
    DecayProbability pb(pa, projectors);
    DetectionMaps maps(peaks, totals, 3, alphas);
    DecayVector expectedPaths;
    std::function<void(DecayLevel*, DecayLevel*, double, std::vector<const Edge*>)> visit;
    visit = [&](DecayLevel* start, DecayLevel* at, double weight, std::vector<const Edge*> cascade) {
        bool terminal = true;
        for (const auto& e : edges) if (e.edge->GetSource() == at) {
            terminal = false; auto next = cascade; next.push_back(&e);
            visit(start, e.edge->GetTarget(), weight * e.weight, next);
        }
        if (!terminal) return;
        for (unsigned mask = 1; mask < (1u << cascade.size()); ++mask) {
            double value = weight;
            std::vector<DecayTransition> selected;
            int first = -1, last = -1;
            for (std::size_t i = 0; i < cascade.size(); ++i) {
                const auto& e = *cascade[i];
                if (mask & (1u << i)) {
                    value *= e.peak / (1 + e.alpha);
                    if (!selected.empty()) value /= 3;
                    selected.push_back(*e.edge);
                    if (first < 0) first = static_cast<int>(i);
                    last = static_cast<int>(i);
                } else value *= 1 - e.total / (3 * (1 + e.alpha));
            }
            if (last - first + 1 == static_cast<int>(selected.size()))
                expectedPaths.AddTerm(DecayPath(selected), value);
        }
    };
    visit(levels[0], levels[0], 0.75, {}); visit(levels[1], levels[1], 0.25, {});
    const auto actualPaths = pb.SummingFeedingVector(d, maps);
    assert(actualPaths.Size() == expectedPaths.Size());
    for (const auto& term : expectedPaths.GetTerms()) near(coefficient(actualPaths, term.path), term.coefficient);
    // Endpoint form adds the direct 3->0 edge and every connected summed route.
    near(pa.PathForm(actualPaths, DecayVector(DecayPath(*edges[2].edge))),
         pa.PathForm(expectedPaths, DecayVector(DecayPath(*edges[2].edge))));

}
