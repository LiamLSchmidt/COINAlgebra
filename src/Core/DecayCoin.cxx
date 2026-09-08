#include "COINAlgebra/Core/DecayCoin.h"
#include <algorithm>
#include <sstream>
#include <stdexcept>
#include <set>

// Constructors share one validation/normalization route: transitions become
// length-one DecayPath values, then the general factor constructor is used.
DecayCoin::DecayCoin(const std::vector<DecayLevel*>& order,
                     const std::vector<DecayTransition>& transitions)
    : DecayCoin(order, [&] {
        std::vector<DecayPath> factors;
        for (const auto& transition : transitions) factors.emplace_back(transition);
        return factors;
    }()) {}

// General construction: validate the order, expand paths, sort factors, reject
// overlap, and finally reduce stationary local identities.
DecayCoin::DecayCoin(const std::vector<DecayLevel*>& order,
                     const std::vector<DecayPath>& factors)
    : fLevelOrder(order)
{
    // Pointer identity defines a level, consistent with DecayPath. A duplicate
    // or null entry would make the rank relation ambiguous or invalid.
    std::set<DecayLevel*> seen;
    for (auto* level : order)
        if (!level || !seen.insert(level).second)
            throw std::invalid_argument("DecayCoin: level order contains null or duplicate levels");
    // Embed connected paths by expanding them into degree-one factors.
    for (const auto& factor : factors) {
        if (factor.Empty()) throw std::invalid_argument("DecayCoin: empty factor");
        if (factor.IsStationary()) fFactors.push_back(factor);
        else for (const auto& transition : factor.GetTransitions())
            fFactors.emplace_back(transition);
    }
    // Rank increases downward. A transition occupies [source,target], with
    // source < target; a stationary factor occupies a single rank. Rank also
    // checks that every endpoint belongs to the supplied ordering.
    for (const auto& factor : fFactors) {
        const auto source = Rank(factor.GetSource());
        const auto target = Rank(factor.GetTarget());
        if (!factor.IsStationary() && source >= target)
            throw std::invalid_argument("DecayCoin: transition does not descend in level order");
    }
    // Canonicalize the unordered input by source, highest first (III.2).
    // The target tie-break puts e_s before a transition starting at s.
    // Transition ties need no further ordering: overlapping edges are rejected.
    std::sort(fFactors.begin(), fFactors.end(), [&](const auto& a, const auto& b) {
        if (Rank(a.GetSource()) != Rank(b.GetSource()))
            return Rank(a.GetSource()) < Rank(b.GetSource());
        return Rank(a.GetTarget()) < Rank(b.GetTarget());
    });
    // Eq. (35), in descending-rank notation: t(p_i) <= s(p_{i+1}).
    // Equality allows a shared endpoint; strict inequality allows a gap.
    // Sorted, descending intervals need only adjacent comparisons to detect
    // any overlap. Repeated transitions are rejected, not deduplicated.
    for (std::size_t i = 1; i < fFactors.size(); ++i)
        if (Rank(fFactors[i-1].GetTarget()) > Rank(fFactors[i].GetSource()))
            throw std::invalid_argument("DecayCoin: overlapping factors");
    // Quotient by local identities at transition endpoints. Repeated
    // stationary factors collapse as e_v e_v = e_v.
    // Eq. (37) removes e_s tensor p and p tensor e_t. A stationary level
    // strictly inside a transition has already failed the overlap check; a
    // stationary level separated from every transition endpoint is retained.
    std::vector<DecayPath> normalized;
    for (const auto& factor : fFactors) {
        if (factor.IsStationary()) {
            const auto* level = factor.GetSource();
            if (std::any_of(fFactors.begin(), fFactors.end(), [&](const auto& p) {
                return !p.IsStationary() && (p.GetSource() == level || p.GetTarget() == level);
            })) continue;
            if (!normalized.empty() && normalized.back() == factor) continue;
        }
        normalized.push_back(factor);
    }
    fFactors = std::move(normalized);
}

std::size_t DecayCoin::Rank(DecayLevel* level) const {
    const auto it = std::find(fLevelOrder.begin(), fLevelOrder.end(), level);
    if (it == fLevelOrder.end()) throw std::invalid_argument("DecayCoin: level outside order");
    return static_cast<std::size_t>(it - fLevelOrder.begin());
}
bool DecayCoin::Empty() const { return fFactors.empty(); }
bool DecayCoin::IsStationary() const {
    return fFactors.size() == 1 && fFactors.front().IsStationary();
}
std::size_t DecayCoin::Degree() const {
    return static_cast<std::size_t>(std::count_if(fFactors.begin(), fFactors.end(),
        [](const auto& p) { return !p.IsStationary(); }));
}
const std::vector<DecayPath>& DecayCoin::GetFactors() const { return fFactors; }
const std::vector<DecayLevel*>& DecayCoin::GetLevelOrder() const { return fLevelOrder; }
DecayLevel* DecayCoin::GetSource() const { return Empty() ? nullptr : fFactors.front().GetSource(); }
DecayLevel* DecayCoin::GetTarget() const { return Empty() ? nullptr : fFactors.back().GetTarget(); }
// Since each operand is canonical, only the boundary between them needs a
// new ordering check. Requiring the same order also keeps Rank meaningful
// across operands; this operation never infers connectivity from the graph.
bool DecayCoin::IsComposableWith(const DecayCoin& other) const {
    return !Empty() && !other.Empty() && fLevelOrder == other.fLevelOrder &&
        Rank(GetTarget()) <= Rank(other.GetSource());
}
DecayCoin DecayCoin::Compose(const DecayCoin& other) const {
    if (!IsComposableWith(other)) throw std::invalid_argument("DecayCoin: invalid ordered product");
    auto factors = fFactors;
    factors.insert(factors.end(), other.fFactors.begin(), other.fFactors.end());
    // Reuse constructor normalization, including identities at the new join.
    return DecayCoin(fLevelOrder, factors);
}
// Canonical storage makes equality a value comparison; DecayPath compares
// transition identity independently of any probability metadata.
bool DecayCoin::operator==(const DecayCoin& other) const {
    return (Empty() && other.Empty()) ||
        (fLevelOrder == other.fLevelOrder && fFactors == other.fFactors);
}
bool DecayCoin::operator!=(const DecayCoin& other) const { return !(*this == other); }
std::string DecayCoin::ToString() const {
    if (Empty()) return "<empty coincidence>";
    std::ostringstream out;
    for (std::size_t i = 0; i < fFactors.size(); ++i) {
        if (i) out << " tensor ";
        out << "(" << fFactors[i].ToString() << ")";
    }
    return out.str();
}
