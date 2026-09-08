#include "COINAlgebra/Algebra/CAlgebra.h"
#include "COINAlgebra/Core/DecayQuiver.h"
#include <algorithm>
#include <map>
#include <set>
#include <stdexcept>

CAlgebra::CAlgebra(const DecayQuiver& quiver,
                   const std::vector<DecayLevel*>& order, const DecayVector& decay)
    : fLevelOrder(order), fDecay(decay)
{
    // Validate uniqueness and nulls even for an empty decay vector.
    DecayCoin(order, std::vector<DecayPath>{});
    if (std::set<DecayLevel*>(order.begin(), order.end()) !=
        std::set<DecayLevel*>(quiver.GetLevels().begin(), quiver.GetLevels().end()))
        throw std::invalid_argument("CAlgebra: order must contain every quiver level");
    // Validate all edges, including those absent from d. Strict descent
    // rules out cycles and makes propagation through levelOrder valid.
    for (const auto* transition : quiver.GetTransitions()) {
        DecayCoin(order, std::vector<DecayTransition>{*transition});
        fTransitions.push_back(*transition);
    }
    // The fiber is specified by d = b + tau, not an arbitrary higher-degree
    // path vector. Stationary terms are allowed but ignored by the connection.
    for (const auto& term : decay.GetTerms()) {
        if (term.path.Empty() || term.path.Length() > 1)
            throw std::invalid_argument("CAlgebra: decay must contain only stationary paths and transitions");
        Validate(Coin(std::vector<DecayPath>{term.path}));
    }
}
void CAlgebra::Validate(const DecayCoin& coin) const {
    if (coin.Empty()) return;
    if (coin.GetLevelOrder() != fLevelOrder)
        throw std::invalid_argument("CAlgebra: incompatible level order");
    // DecayCoin already checked endpoints against its order. Only transition
    // membership remains: match the same name/endpoints used by DecayPath.
    for (const auto& factor : coin.GetFactors()) {
        if (factor.IsStationary()) continue;
        if (std::none_of(fTransitions.begin(), fTransitions.end(), [&](const auto& transition) {
            return DecayPath(transition) == factor;
        })) throw std::invalid_argument("CAlgebra: transition outside quiver");
    }
}
DecayCoin CAlgebra::Coin(const std::vector<DecayTransition>& transitions) const {
    DecayCoin coin(fLevelOrder, transitions); Validate(coin); return coin;
}
DecayCoin CAlgebra::Coin(const std::vector<DecayPath>& factors) const {
    DecayCoin coin(fLevelOrder, factors); Validate(coin); return coin;
}
// Keep algebra-produced vectors sparse and combine all contributions to the
// same basis. Exact cancellation removes a term; small nonzero weights survive.
void CAlgebra::AddTerm(Vector& vector, const DecayCoin& coin, double coefficient) {
    if (coin.Empty() || coefficient == 0.0) return;
    for (auto it = vector.begin(); it != vector.end(); ++it) {
        if (it->coin == coin) {
            it->coefficient += coefficient;
            if (it->coefficient == 0.0) vector.erase(it);
            return;
        }
    }
    vector.push_back({coin, coefficient});
}
CAlgebra::Vector CAlgebra::Embed(const DecayVector& vector) const {
    Vector result;
    for (const auto& term : vector.GetTerms())
        AddTerm(result, Coin(std::vector<DecayPath>{term.path}), term.coefficient);
    return result;
}
double CAlgebra::ScalarConnection(const DecayCoin& first, const DecayCoin& second) const {
    Validate(first); Validate(second);
    if (!first.IsComposableWith(second)) return 0.0;
    // Eq. (41), evaluated without enumerating the finite path expansion:
    //
    //   c_d(x,y) = sum_{gamma: t(x) -> s(y)} product_{edge in gamma} tau_edge.
    //
    // At each level u, weights[u] is the accumulated sum of all paths from
    // t(x) to u. Propagating weights[u] * tau_(u,v) extends every such path
    // by one edge. Topological order ensures all incoming contributions are
    // complete before u is processed. Parallel edges contribute separately.
    // The map value-initializes unseen levels to zero.
    //
    // Stationary populations in d do not contribute: this expands tau only.
    // Unlike DecayProbability::PathConnection, no final observed transition
    // is applied. Its coefficient belongs to the vector being multiplied.
    std::map<DecayLevel*, double> weights;
    // Seed the length-zero connecting path with the empty product 1.
    weights[first.GetTarget()] = 1.0;
    for (auto* level : fLevelOrder) {
        // Stop before leaving the destination. In particular, equal boundary
        // endpoints return 1 without traversing an edge. Earlier unreachable
        // levels have weight zero, so starting the loop at the top is harmless.
        if (level == second.GetSource()) return weights[level];
        for (const auto& term : fDecay.GetTerms())
            if (!term.path.IsStationary() && term.path.GetSource() == level)
                weights[term.path.GetTarget()] += weights[level] * term.coefficient;
    }
    return 0.0;
}
CAlgebra::Vector CAlgebra::Multiply(const DecayCoin& first, const DecayCoin& second) const {
    const double connection = ScalarConnection(first, second);
    // A structurally valid coincidence may still have zero coefficient in
    // this fiber. Represent that as the zero vector, not an invalid basis.
    if (connection == 0.0) return {};
    return {{first.Compose(second), connection}};
}
CAlgebra::Vector CAlgebra::Multiply(const Vector& first, const Vector& second) const {
    Vector result;
    // Validate even when the other vector is empty, so invalid context is
    // reported consistently rather than hidden by multiplication by zero.
    for (const auto& term : first) Validate(term.coin);
    for (const auto& term : second) Validate(term.coin);
    // Distributivity: a_x b_y c_d(x,y) multiplies the joined basis x tensor y.
    // Different pairs can produce the same basis, hence AddTerm collection.
    for (const auto& a : first) for (const auto& b : second)
        for (const auto& product : Multiply(a.coin, b.coin))
            AddTerm(result, product.coin, a.coefficient * b.coefficient * product.coefficient);
    return result;
}
CAlgebra::Vector CAlgebra::Power(const Vector& vector, std::size_t n) const {
    if (n == 0) throw std::invalid_argument("CAlgebra: no global identity for zeroth power");
    Vector result;
    for (const auto& term : vector) {
        Validate(term.coin); AddTerm(result, term.coin, term.coefficient);
    }
    // Left-associated positive power. Once the result is zero it stays zero;
    // stationary factors mean that degree alone cannot bound all powers.
    for (std::size_t i = 1; i < n && !result.empty(); ++i) result = Multiply(result, vector);
    return result;
}
