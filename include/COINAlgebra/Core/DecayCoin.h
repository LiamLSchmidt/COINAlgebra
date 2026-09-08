#ifndef COINALGEBRA_DECAYCOIN_H
#define COINALGEBRA_DECAYCOIN_H

#include "COINAlgebra/Core/DecayPath.h"

// One basis element of the coincidence space C (Definitions III.1-4).
// Stores ordered factors, not a linear combination or a probability. Factors
// are stationary paths or single transitions; connected paths are expanded.
//
// The supplied total order runs from highest to lowest level. Its vector is
// copied, but its level pointers are non-owning: levels must remain alive and
// unchanged. This class does not infer energy order or compute the paper's N(v).
// Non-overlap is geometric in this order, independent of connecting-path weight.
class DecayCoin
{
public:
    // Empty sentinel: neither a stationary path nor a global tensor identity.
    DecayCoin() = default;
    // Canonicalize unordered input and reject invalid levels/overlap with
    // std::invalid_argument. Empty input produces an empty sentinel.
    DecayCoin(const std::vector<DecayLevel*>& levelOrder,
              const std::vector<DecayTransition>& transitions);
    // Canonicalize unordered input and reject invalid levels/overlap with
    // std::invalid_argument. Empty input produces an empty sentinel.
    DecayCoin(const std::vector<DecayLevel*>& levelOrder,
              const std::vector<DecayPath>& factors);
    // No retained factors. A stationary coincidence is therefore not Empty().
    bool Empty() const;
    // Exactly one stationary factor e_v. Several separated stationary factors
    // have degree zero too, but are not a single stationary coincidence.
    bool IsStationary() const;
    // Number of transition factors (Definition III.3); stationary degree is 0.
    std::size_t Degree() const;
    // Read-only canonical factors, after reducing local stationary identities.
    const std::vector<DecayPath>& GetFactors() const;
    // Index 0 is highest; increasing indices descend through the quiver.
    const std::vector<DecayLevel*>& GetLevelOrder() const;
    // Source of the first factor and target of the last (corrected Eq. 40).
    // Both return nullptr for the empty sentinel.
    DecayLevel* GetSource() const;
    DecayLevel* GetTarget() const;
    // Ordered tensor compatibility: both nonempty, identical level orders,
    // and this target is at or above the other source. Gaps are allowed;
    // this does not assert that a nonzero weighted connecting path exists.
    bool IsComposableWith(const DecayCoin& other) const;
    // Join in operand order and reduce local identities, without a scalar.
    // Throws std::invalid_argument for incompatible operands. Use CAlgebra
    // multiplication to obtain the weighted product or an algebraic zero.
    DecayCoin Compose(const DecayCoin& other) const;
    // Canonical equality includes the complete level order and DecayPath
    // factor equality (transition names/endpoints, not probabilities). All
    // empty sentinels compare equal, regardless of their stored orders.
    bool operator==(const DecayCoin& other) const;
    bool operator!=(const DecayCoin& other) const;
    // Human-readable tensor expression; contains no algebra coefficient.
    std::string ToString() const;
private:
    // Copied ordering context, with borrowed level pointers.
    std::vector<DecayLevel*> fLevelOrder;
    // Owned factor values; their endpoint pointers remain borrowed.
    std::vector<DecayPath> fFactors;
    // Position in fLevelOrder; throws if the level is outside this context.
    std::size_t Rank(DecayLevel* level) const;
};
#endif
