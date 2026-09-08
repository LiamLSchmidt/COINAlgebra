#ifndef COINALGEBRA_CALGEBRA_H
#define COINALGEBRA_CALGEBRA_H
#include "COINAlgebra/Core/DecayCoin.h"
#include "COINAlgebra/Core/DecayVector.h"
class DecayQuiver;

// A fixed fiber C_d (Definitions III.5-6). The level order is explicit,
// highest first, including every quiver level. Decay coefficients are copied;
// quiver levels must remain alive and unchanged while this algebra is used.
class CAlgebra
{
public:
    // A coefficient times one basis coincidence. Vector is a sparse list;
    // callers may supply repeated terms, collected by algebra operations.
    // An empty Vector represents the algebraic zero.
    struct Term { DecayCoin coin; double coefficient; };
    using Vector = std::vector<Term>;
    // Snapshot the quiver transitions and d = b + tau. Require every level
    // exactly once in descending order and every edge to descend. The defining
    // decay vector may contain only stationary and single-transition paths.
    // Invalid structure throws std::invalid_argument. No normalization or
    // positivity of coefficients is imposed by this algebra.
    CAlgebra(const DecayQuiver& quiver,
             const std::vector<DecayLevel*>& levelOrder,
             const DecayVector& decay);
    // Construct a basis element in this order and check quiver membership.
    // A valid basis element need not have nonzero connection weight in d.
    DecayCoin Coin(const std::vector<DecayTransition>& transitions) const;
    // Also accepts stationary factors and longer paths, expanded by DecayCoin.
    DecayCoin Coin(const std::vector<DecayPath>& factors) const;
    // Embed path terms into C without changing their coefficients. This input
    // may contain longer paths, unlike the decay vector defining the fiber.
    Vector Embed(const DecayVector& vector) const;
    // Eq. (41): sum of products of tau coefficients over all paths from
    // t(first) to s(second), including the stationary path of weight 1 when
    // endpoints coincide. Returns 0 for incompatible order/overlap or no
    // weighted connection. Invalid context/membership throws instead.
    // Uses fDecay coefficients, not transition.GetProbability(), and excludes
    // stationary populations and coefficients of the observed factors.
    double ScalarConnection(const DecayCoin& first, const DecayCoin& second) const;
    // Eq. (45): first * second = c_d(first,second) (first tensor second).
    // Returns zero or one term. Operands are never swapped to make a product
    // valid; ordered multiplication differs from unordered basis construction.
    Vector Multiply(const DecayCoin& first, const DecayCoin& second) const;
    // Bilinear extension: multiply coefficients and collect equal bases.
    Vector Multiply(const Vector& first, const Vector& second) const;
    // Positive integer powers by repeated ordered multiplication. n = 1
    // validates and collects the supplied terms.
    // There is no global identity (paper, Sec. IV); exponent zero is rejected.
    Vector Power(const Vector& vector, std::size_t n) const;
private:
    // Owned vector of borrowed level pointers; also a topological order.
    std::vector<DecayLevel*> fLevelOrder;
    // Snapshot for checking transition identity by name and endpoint pointers.
    std::vector<DecayTransition> fTransitions;
    // Fixed fiber data: later edits to the caller's vector do not affect C_d.
    DecayVector fDecay;
    // Check algebra context and transition membership; allow the empty sentinel.
    void Validate(const DecayCoin& coin) const;
    // Collect equal bases and remove exact zeros, without tolerance truncation.
    static void AddTerm(Vector& vector, const DecayCoin& coin, double coefficient);
};
#endif
