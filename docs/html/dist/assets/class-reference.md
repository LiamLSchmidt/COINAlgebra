# COINAlgebra class reference and mathematical guide

This guide documents the implemented public C++ API under `include/COINAlgebra`,
including the mathematical core, path projectors, coincidence algebra,
probability calculator, nuclear-data records/readers, builder, and ROOT wrapper.
It describes current behavior, including implementation limitations, rather than
assuming that every construction in the manuscript has already been implemented.
DQStudio types, document metadata, view state, and analysis workflows are also documented.

The mathematical source is [the manuscript](paper/APS_Coincidence_Algebra-14.pdf).
The implementation uses `t(p_i tensor ... tensor p_n) = t(p_n)` for Eq. (40).
Paper discrepancies and their confirmation status are tracked in the
[discrepancy register](paper/ERRATA.md).

## Contents

- [Mathematical objects and conventions](#mathematical-objects-and-conventions)
- [Complete worked example](#complete-worked-example)
- [Ownership and numerical conventions](#ownership-and-numerical-conventions)
- [DecayLevel](#decaylevel)
- [DecayTransition](#decaytransition)
- [DecayQuiver](#decayquiver)
- [DecayPath](#decaypath)
- [DecayVector](#decayvector)
- [PathAlgebra](#pathalgebra)
- [PathProjectors](#pathprojectors)
- [DecayProbability](#decayprobability)
- [DecayCoin](#decaycoin)
- [CAlgebra](#calgebra)
- [DetectionMaps](#detectionmaps)
- [DQStudio architecture](#dqstudio-architecture)
- [Nuclear-data records and readers](#nuclear-data-records-and-readers)
- [DecayQuiverBuilder](#decayquiverbuilder)
- [ROOT environment](#root-environment)
- [Validation and mathematical boundaries](#validation-and-mathematical-boundaries)
- [Complete public declarations](#complete-public-declarations)

## Mathematical objects and conventions

Let a decay quiver be `D = (D_0, D_1, s, t)`, with levels in `D_0` and
transitions in `D_1`. The probability constructions assume a finite acyclic
quiver. The basic quiver container itself does not enforce acyclicity.

| Mathematical object | C++ representation | Meaning |
| --- | --- | --- |
| Vertex `v` | `DecayLevel` | A level identified by its object/pointer, with a display name. |
| Arrow `p: u -> v` | `DecayTransition` | A named directed transition and intrinsic probability metadata. |
| Stationary path `e_v` | `DecayPath(v)` | Length-zero path at a particular level. |
| Connected path | `DecayPath` | Sequence of transitions with matching adjacent endpoints. |
| `sum a_p p` in the path space | `DecayVector` | Sparse linear combination of exact basis paths. |
| Path algebra `kD` | `PathAlgebra` | Ordered concatenation, extended bilinearly, and endpoint forms. |
| `B`, `S_i`, `T_i`, `V_s`, `V_t` | `PathProjectors` | Selection of terms or aggregation onto stationary paths. |
| Coincidence basis in `C` | `DecayCoin` | Canonical non-overlapping factors, including separated transitions. |
| Fixed fiber `C_d` | `CAlgebra` | Product weighted by the scalar connection determined by `d`. |
| `sum a_c c` in `C` | `CAlgebra::Vector` | `std::vector<CAlgebra::Term>`, with basis and coefficient fields. |

In this API `first.Compose(second)` and `Multiply(first, second)` mean
**traverse first, then second**. For paths the join condition is
`t(first) == s(second)`. This traversal convention should be kept explicit when
translating formulas written in a right-to-left composition convention.

A physical decay vector is normally

\[
d=b+\tau,\qquad b=\sum_v b_v e_v,\qquad
\tau=\sum_{p\in D_1}\tau_p p.
\]

Here `b_v` is an initial population and `tau_p` is a conditional transition
weight. A stationary path's intrinsic `GetProbability()` is one; that does
**not** prescribe its vector coefficient. Setting a transition's probability
does not automatically put it into a `DecayVector` either.

## Complete worked example

This macro builds a three-level scheme, constructs `d`, computes the ordinary
feeding vector, and computes the branch-resolved probability vector
`P = b *d tau`. It also demonstrates a connected coincidence product.

Save the code as `class_reference_example.C` and run it from the repository
with `bin/coinalgebra -l -b -q class_reference_example.C` after `source setup.sh`.
The includes are in dependency order for the ROOT interpreter.

```cpp
#include "COINAlgebra/Core/DecayQuiver.h"
#include "COINAlgebra/Core/DecayPath.h"
#include "COINAlgebra/Core/DecayVector.h"
#include "COINAlgebra/Algebra/PathAlgebra.h"
#include "COINAlgebra/Algebra/PathProjectors.h"
#include "COINAlgebra/Probability/DecayProbability.h"
#include "COINAlgebra/Algebra/CAlgebra.h"
#include <iostream>
#include <vector>

void class_reference_example()
{
    DecayQuiver q;
    auto* d0 = q.AddLevel("d0");
    auto* d1 = q.AddLevel("d1");
    auto* d2 = q.AddLevel("d2");
    auto* g21 = q.AddTransition("g21", d2, d1, 0.6);
    q.AddTransition("g20", d2, d0, 0.4);
    auto* g10 = q.AddTransition("g10", d1, d0, 1.0);

    DecayVector d;
    d.AddTerm(DecayPath(d2), 1.0); // all initial population at d2
    for (const auto* edge : q.GetTransitions())
        d.AddTerm(DecayPath(*edge), edge->GetProbability());

    PathAlgebra paths(q);
    PathProjectors projectors;
    DecayProbability probabilities(paths, projectors);
    auto feeding = probabilities.FeedingVector(d);
    feeding.PrintTable(); // g21: 0.6, g20: 0.4, g10: 0.6

    auto b = projectors.BranchingProjector(d);
    auto tau = d - b;
    CAlgebra coins(q, {d2, d1, d0}, d); // highest first
    auto P = coins.Multiply(coins.Embed(b), coins.Embed(tau));
    for (const auto& term : P)
        std::cout << term.coefficient << " * "
                  << term.coin.ToString() << '\n';
    // P contains 0.6*g21, 0.4*g20, and 0.6*(e_d2 tensor g10).
    // The separated stationary factor records the initial branch.

    auto upper = coins.Coin(std::vector<DecayTransition>{*g21});
    auto lower = coins.Coin(std::vector<DecayTransition>{*g10});
    std::cout << "Connection: " << coins.ScalarConnection(upper, lower) << '\n';
    // Connection is 1 because t(g21) == s(g10).

    auto pair = coins.Multiply(CAlgebra::Vector{{upper, 0.6}},
                               CAlgebra::Vector{{lower, 1.0}});
    std::cout << "Pair weight: " << pair.front().coefficient << '\n'; // 0.6
}
```

A probability vector need not have coefficients summing to one: a decay can
emit several different transitions. For this example the feeding coefficients
sum to `1.6`, the expected number of modeled transitions per decay.

For a larger example with several initial populations, see
[`example_vector.C`](../examples/basic/example_vector.C). Summing the
branch-resolved terms for each transition recovers its feeding coefficient.

## Ownership and numerical conventions

`DecayQuiver` allocates and owns the levels and transitions added through it.
Its destructor deletes them. Returned pointers are borrowed; do not delete them.
Paths copy transition values, but their level pointers remain borrowed.
Coincidences copy path factors and their level-order vector, with the same
borrowed level pointers. Mutating a quiver transition later does not update
transition copies already stored in paths.

`PathAlgebra` borrows its quiver. `DecayProbability` borrows both its
`PathAlgebra` and `PathProjectors`. Those dependencies must outlive the borrower.
`CAlgebra` copies the defining vector and transition definitions, but still
borrows the levels. Keep those levels alive and unchanged. Do not remove levels
while paths, vectors, or algebras referring to them are in use.

`DecayQuiver` currently has owning raw pointers and no custom copy/move policy.
Do not copy or assign populated quivers: a compiler-generated copy shares owned
pointers and can cause double deletion. Initialize a builder result directly;
the builder's named-local return currently also relies on normal NRVO behavior.
Safe general-purpose quiver value semantics are not implemented.

All algebra coefficients are `double`. They may be negative for formal linear
algebra. The APIs do not generally enforce finite, nonnegative, normalized
physical inputs. In particular, the range check on transition probability uses
comparisons against zero and one; it is not an explicit NaN check.

`DecayVector` simplifies coefficients with magnitude at most `1e-14` when it
combines an existing term or scales the vector. A newly inserted nonzero term
is not immediately subjected to this tolerance. `CAlgebra` collection removes
only exact zeros. These numerical conventions can affect tiny contributions.

## DecayLevel

Header: [`Core/DecayLevel.h`](../include/COINAlgebra/Core/DecayLevel.h).
Represents a vertex with an optional physical energy in keV. Unknown energy is
distinct from zero. Spin and algebra ordering rank are not stored.

| Public function | Behavior |
| --- | --- |
| `DecayLevel()` | Constructs a level with an empty name. |
| `explicit DecayLevel(const std::string& name)` | Stores the supplied name. |
| `DecayLevel(name, energy_keV)` | Constructs with a finite nonnegative energy. |
| `HasEnergy() const` | Reports whether energy is known. |
| `GetEnergy() const` | Returns keV; throws `std::logic_error` when unknown. |
| `SetEnergy(energy_keV)` | Validates finite nonnegative energy before replacing it. |
| `ClearEnergy()` | Returns energy to unknown. |
| `const std::string& GetName() const` | Borrows the stored name. |
| `void SetName(const std::string& name)` | Replaces the name. Does not check other levels for uniqueness. |
| `void Print() const` | Writes a vertex description to standard output. |

Level equality in the mathematical operations is pointer identity. Two separate
objects with equal names are distinct levels. Prefer `quiver.AddLevel(name)`
to create levels with managed lifetime and initial name uniqueness.

## DecayTransition

Header: [`Core/DecayTransition.h`](../include/COINAlgebra/Core/DecayTransition.h).
Represents an arrow `p` with source `s(p)` and target `t(p)`.

| Public function | Behavior |
| --- | --- |
| `DecayTransition()` | Empty name, null endpoints, probability `1.0`; not a usable path edge yet. |
| `DecayTransition(name, source, target, probability = 1.0)` | Stores endpoints and name, validates probability through `SetProbability`. Direct construction does not reject null endpoints. |
| `GetName() const` | Returns the name by const reference. |
| `GetSource() const`, `GetTarget() const` | Return borrowed `DecayLevel*` endpoints. |
| `GetProbability() const` | Returns intrinsic transition probability metadata. |
| `SetName(name)` | Changes name; no quiver-level uniqueness check. |
| `SetSource(source)`, `SetTarget(target)` | Replace endpoint pointers; no membership, null, cycle, or order check. |
| `SetProbability(probability)` | Throws `std::invalid_argument` if below zero or above one. |
| `Print() const` | Prints name, endpoints, and probability; null endpoints are printable. |

`DecayQuiver::AddTransition` provides additional name/null validation.
Neither API automatically ensures source and target belong to the same quiver.

## DecayQuiver

Header: [`Core/DecayQuiver.h`](../include/COINAlgebra/Core/DecayQuiver.h).
This is the owning graph container, not a probability vector or an algebra.

| Public function | Behavior |
| --- | --- |
| `DecayQuiver()` | Creates an empty graph. |
| `~DecayQuiver()` | Deletes owned transitions, then levels. |
| `AddLevel(name)` | Allocates a level and returns its pointer. Duplicate current name throws `std::invalid_argument`. |
| `AddLevel(name, energy_keV)` | Adds an owned level with validated physical energy. |
| `ExportJson(filename, vectors, title, conversionCoefficients, initialPopulations) const` | Exports graph, aligned energies, vectors and optional IC/population metadata. Validates supplied IC coverage and populations before opening the file. |
| `GetLevel(name) const` | Finds the first matching name; returns `nullptr` when absent. |
| `GetLevels() const` | Const reference to the vector of level pointers in insertion order. This is not necessarily descending physical order. |
| `AddTransition(name, source, target, probability = 1.0)` | Allocates an edge. Rejects null endpoints, duplicate current transition names, and out-of-range probability. Returns its pointer. |
| `GetTransition(name) const` | Matching pointer or `nullptr`. |
| `GetTransitions() const` | Const reference to stored transition pointers in insertion order. |
| `RemoveTransition(transition)` | Deletes the matching owned pointer and returns `true`; null/absent pointer returns `false`. |
| `RemoveLevel(level)` | Removes edges referring to this pointer, then deletes the matching owned level. Returns whether the level was found. Null returns `false`. Incident-edge removal precedes membership testing. |
| `IsComposable(first, second) const` | Tests `t(first) == s(second)` for non-null pointers; does not check membership. |
| `HasDirectTransition(source, target) const` | Tests for at least one stored edge between the pointers. Null arguments return `false`. |
| `GetOutgoingProbability(level) const` | Sums intrinsic probabilities of edges with this source. Null throws `std::invalid_argument`. |
| `IsNormalized(level, tolerance = 1e-12) const` | Tests whether the outgoing sum differs from one by at most the tolerance. Null returns `false`. A terminal level with sum zero is not normalized by this test. |
| `Print() const` | Prints vertices and arrows. |

Parallel edges are allowed when their names differ. Self-loops and cycles are
not rejected here; avoid them for finite decay expansions. The coincidence
algebra constructor supplies its own stricter descending-order validation.

## DecayPath

Header: [`Core/DecayPath.h`](../include/COINAlgebra/Core/DecayPath.h).
A basis path is a connected sequence `p_1,...,p_n` with
`t(p_i) = s(p_{i+1})`. A stationary path has no transitions but a real endpoint.

| Public function | Behavior |
| --- | --- |
| `DecayPath()` | Empty sentinel: null source/target, length zero. |
| `explicit DecayPath(DecayLevel* level)` | Constructs `e_level`; null throws `std::invalid_argument`. |
| `explicit DecayPath(const DecayTransition& transition)` | Copies a single transition and validates the path. |
| `explicit DecayPath(const std::vector<DecayTransition>& transitions)` | Copies the sequence in traversal order; validates endpoint/join conditions. Empty input is the empty sentinel. |
| `Empty() const` | True for the empty sentinel, false for a stationary path. |
| `IsStationary() const` | True for the length-zero path at a non-null level. |
| `Length() const` | Number of transition factors. |
| `GetTransitions() const` | Const reference to copied transition values. |
| `GetTransition(index) const` | Const reference to one factor; invalid index throws `std::out_of_range`. |
| `GetSource() const`, `GetTarget() const` | Explicit endpoint pointers; null for the sentinel. |
| `GetProbability() const` | Product of stored transition probabilities. The empty product is one for both stationary and empty paths; this does not make the sentinel a valid basis. |
| `IsComposableWith(other) const` | Both paths nonempty and this target equals the other's source. |
| `Compose(other) const` | Traverses this path, then `other`; stationary identities reduce. Incompatible paths throw `std::invalid_argument`. |
| `operator==(other) const` | Exact basis equality: stationary level pointer, or ordered transition names and endpoint pointers. Probability metadata is excluded. |
| `operator!=(other) const` | Negation of equality. |
| `HasSameSource(other) const` | Equal source pointers for nonempty paths. |
| `HasSameTarget(other) const` | Equal target pointers for nonempty paths. |
| `HasSameEndpoints(other) const` | Both endpoint comparisons; ignores intermediate route. |
| `ToString() const` | Human-readable path, stationary path, or empty marker. |

Two different routes from `u` to `v` can satisfy `HasSameEndpoints` without
being equal. The endpoint forms below deliberately combine such routes;
`DecayVector` keeps them as distinct bases.

The validator checks outer endpoints and adjacent joins. Do not use it as a
replacement for validating every transition's endpoints or graph membership.

## DecayVector

Header: [`Core/DecayVector.h`](../include/COINAlgebra/Core/DecayVector.h).
Represents `v = sum_p a_p p`, with public nested record
`Term { DecayPath path; double coefficient; }`.

| Public function | Behavior |
| --- | --- |
| `DecayVector()` | Zero vector, no terms. |
| `explicit DecayVector(path, coefficient = 1.0)` | One term via `AddTerm`. Does not infer a coefficient from intrinsic probability. |
| `Empty() const` | Whether there are no stored terms. |
| `Size() const` | Number of stored basis terms, not total path length. |
| `GetTerms() const` | Const reference to the sparse term list. |
| `AddTerm(path, coefficient)` | Rejects empty path even for coefficient zero; skips exact-zero coefficients; combines exact equal paths. |
| `operator+(other) const`, `operator-(other) const` | Return sum/difference with term collection. |
| `operator+=(other)`, `operator-=(other)` | Mutate this vector, return `DecayVector&`. Avoid aliasing the same vector as the right-hand input; these operations iterate the input while updating storage. |
| `operator*(double scalar) const` | Returns scalar multiple. |
| `operator*=(double scalar)` | Scales in place, simplifies, returns `DecayVector&`. |
| Free `operator*(double scalar, const DecayVector& vector)` | Supports `scalar * vector`. |
| `ApplyConversionMap(conversionCoefficients) const` | Multiplies each path by the product of `1/(1+alpha)`; preserves stationary terms. Required alpha entries must be finite and nonnegative. |
| `ApplyDetectionMap(efficiencies) const` | Multiplies each path by its edge efficiencies, each finite and in [0,1]. Required entries must be present. |
| `ToString() const` | Algebraic expression, or `"0"` for zero. |
| `Print() const` | Prints `ToString()` to standard output. |
| `PrintTable() const` | Prints path/coefficient table to standard output. |
| `PrintTable(std::ostream& out) const` | Same table to a supplied stream, with eight decimal places. Changes stream formatting. |

The table labels coefficients as probabilities but does not validate that
interpretation. There is no vector-vector `operator*`; use the relevant algebra.

## PathAlgebra

Header: [`Algebra/PathAlgebra.h`](../include/COINAlgebra/Algebra/PathAlgebra.h).
For basis paths the product concatenates when endpoints match, and is zero
otherwise. Extend by distributivity to vectors.

\[
1=\sum_{v\in D_0}e_v,\qquad
\operatorname{Multiply}(a,b)=\sum_{p,q:\,t(p)=s(q)}a_p b_q\,(p\text{ then }q).
\]

| Public function | Behavior |
| --- | --- |
| `PathAlgebra(const DecayQuiver& quiver)` | Borrows the quiver; no structural validation. |
| `Identity() const` | Sum of stationary paths, coefficient one per level. Empty quiver gives zero. |
| `MaxPower() const` | `numberOfLevels - 1`, or zero for no levels. A bound on nonstationary path length for an acyclic quiver, not arbitrary vectors containing stationary terms. |
| `GetQuiver() const` | Returns the borrowed quiver by const reference. |
| `Multiply(first, second) const` | Bilinear ordered concatenation; noncomposable pairs contribute zero. Uses vector coefficients, not intrinsic probabilities. |
| `Power(d, n) const` | `d^0 = Identity()`, `d^1 = d`, higher powers by multiplication. |
| `PowerExpand(d, maxPower) const` | `1 + d + ... + d^maxPower`, inclusive. |
| `SourceForm(first, second) const` | Sum `a_p b_q` over pairs with the same source. |
| `TargetForm(first, second) const` | Sum `a_p b_q` over pairs with the same target. |
| `PathForm(first, second) const` | Sum `a_p b_q` over pairs with both endpoints equal. |

The forms implement Definitions II.1-3:

\[
\langle p,q\rangle_s=\delta_{s(p),s(q)},\quad
\langle p,q\rangle_t=\delta_{t(p),t(q)},\quad
\langle p,q\rangle_P=\delta_{s(p),s(q)}\delta_{t(p),t(q)}.
\]

`PathForm` is an endpoint-equivalence pairing, not a coefficient lookup for an
exact path. Distinct parallel transitions are paired if their endpoints agree.
The algebra does not check that input vector terms belong to its quiver; callers
must preserve that context, especially for identities and finite expansions.

## PathProjectors

Header: [`Algebra/PathProjectors.h`](../include/COINAlgebra/Algebra/PathProjectors.h).
`PathProjectors()` is a stateless default constructor. Every operation returns a
new `DecayVector`. Definitions II.4-8 motivate these maps; the formulas below
specify the actual implemented action and domains.

| Public function | Implemented map on `v = sum a_p p` |
| --- | --- |
| `SourceProjector(v, source) const` | Retains only length-one paths with the given source, copying each coefficient unchanged. Null source throws `std::invalid_argument`. |
| `TargetProjector(v, target) const` | Retains only length-one paths with the given target, copying each coefficient unchanged. Null target throws `std::invalid_argument`. |
| `SourceVertexProjector(v) const` | Replaces each path by `e_s(p)` with the same coefficient; sums equal stationary paths. Acts on all lengths. Skips null endpoints. |
| `TargetVertexProjector(v) const` | Replaces each path by `e_t(p)` with the same coefficient; sums equal stationary paths. Acts on all lengths. Skips null endpoints. |
| `BranchingProjector(v) const` | Retains only stationary paths already in `v`, preserving coefficients. |

In particular,

\[
S_i(v)=\sum_{p\in P_1:s(p)=i}a_p p,\qquad
T_i(v)=\sum_{p\in P_1:t(p)=i}a_p p,
\]
\[
V_s(v)=\sum_p a_p e_{s(p)},\quad
V_t(v)=\sum_p a_p e_{t(p)},\quad
B(v)=\sum_{p\text{ stationary}}a_p p.
\]

Selection and aggregation are different. For `v = 0.2*p + 0.3*q` with a common
source `u`, `SourceProjector(v,u)` returns those two original terms;
`SourceVertexProjector(v)` returns `0.5*e_u`. It does not multiply each original
coefficient by another source-form sum. The implementation follows this linear,
coefficient-preserving interpretation even where manuscript notation could be
read as an additional bilinear weighting.

Use `b = BranchingProjector(d)` and `tau = d - b` when `d` has only stationary
and length-one components. Subtraction does not otherwise guarantee that the
remainder contains only transitions.

## DecayProbability

Header: [`Probability/DecayProbability.h`](../include/COINAlgebra/Probability/DecayProbability.h).
Provides the path-algebra probability calculations. Let
`R = 1 + tau + ... + tau^N`, with `N = PathAlgebra::MaxPower()`.

| Public function | Meaning |
| --- | --- |
| `DecayProbability(const PathAlgebra& algebra, const PathProjectors& projectors)` | Borrows both dependencies; they must remain alive. |
| `FeedingVector(decay) const` | Computes `F = V_t(b R) tau`, in API traversal order. Returns total physical feeding (gamma + IC when the input branches include both). |
| `EmissionFeedingVector(decay, alpha) const` | Physical feeding followed by the gamma fraction; conversion still feeds daughter levels. |
| `DetectionFeedingVector(decay, efficiencies) const` | Applies only the supplied efficiency to physical feeding. Does not add an IC correction. |
| `DetectionFeedingVector(decay, efficiencies, alpha) const` | Gamma emission feeding followed by conditional detector efficiency. |
| `SummingFeedingVector(decay, maps) const` | Connected-path summing with terminal-complete propagation and N^(k-1) detector scaling. Requires a DAG with normalized nonnegative outgoing physical branches. |
| `FeedingProbability(decay, path) const` | `PathForm(FeedingVector(decay), DecayVector(path,1))`. Selects by endpoints, not exact transition identity. |
| `PathConnection(decay, path_i, path_j) const` | `PathForm(V_t(e_t(path_i) R) tau, DecayVector(path_j,1))`. Includes the final transition coefficient. |
| `CoincidenceProbability(decay, path_i, path_j) const` | `FeedingProbability(decay,path_i) * PathConnection(decay,path_i,path_j)`. Intended for ordered transition queries. |

For a normal single-edge decay vector on a DAG, `FeedingVector` propagates
populations in topological order without enumerating all paths. It falls back
to the finite path expansion when that route is unsuitable. The fallback does
not make cyclic decay schemes physically valid.

`PathConnection` differs from `CAlgebra::ScalarConnection`: the former includes
one final transition, while the latter stops at its source. For a unique lower
transition `q`, the former is `c_d(p,q) * tau_q`. Parallel endpoint-equivalent
transitions are aggregated by the path form, so it is not generally a lookup
for one named arrow. Queries using longer paths likewise select endpoints; they
do not require observing that exact multi-transition sequence.

For a higher observed transition `p` followed by `q`, the product gives their
joint probability under the modeled decay assumptions. It does not reorder
queries or incorporate angular correlations, detection efficiencies, or timing.

## DecayCoin

Header: [`Core/DecayCoin.h`](../include/COINAlgebra/Core/DecayCoin.h).
Implements one basis coincidence from Definitions III.1-4, with non-overlap
(Eq. 35), canonical ordering (Eq. 36), and local identity reductions (Eq. 37).

### Ordering and non-overlap

Supply a complete physical order for your calculation, **highest first**.
`r(v)` is the position in the supplied vector. Every transition must satisfy
`r(s(p)) < r(t(p))`; a stationary factor has equal ranks. The class does not
compute energies or the manuscript's outgoing-arrow count `N(v)`.

In a fully connected decay quiver, descending `N(v)` yields the intended order.
In a sparse quiver, arrow counts need not determine physical order; the caller
must supply it. Different valid orders of otherwise unconstrained levels can
change the geometric non-overlap decision.

Factors are sorted by source rank and then target rank. Adjacent factors must
satisfy

\[
r(t(p_i))\leq r(s(p_{i+1})).
\]

Equality permits connected transitions; a strict inequality permits a gap.
After sorting, checking adjacent intervals suffices for pairwise non-overlap.
Shared source/target intervals, crossing intervals, and repeated transitions
are rejected. Duplicate transitions are not silently turned into one set entry.

A stationary factor at a transition endpoint reduces away:
`e_s(p) tensor p = p` and `p tensor e_t(p) = p`. Repeated stationary factors at
one level collapse. A stationary factor in a gap remains; one strictly inside
a transition interval is rejected.

### Public functions

| Public function | Behavior |
| --- | --- |
| `DecayCoin()` | Empty sentinel, not a stationary basis or global identity. |
| `DecayCoin(levelOrder, const std::vector<DecayTransition>& transitions)` | Copies and canonicalizes unordered transitions. |
| `DecayCoin(levelOrder, const std::vector<DecayPath>& factors)` | Accepts stationary and longer connected paths; expands longer paths into single-transition factors, then canonicalizes. |
| `Empty() const` | No factors; empty constructor input also gives this sentinel. |
| `IsStationary() const` | Exactly one stationary factor. Several separated stationary factors have degree zero but return `false`. |
| `Degree() const` | Number of nonstationary factors, not tensor factor count. |
| `GetFactors() const` | Const reference to canonical `std::vector<DecayPath>`. |
| `GetLevelOrder() const` | Const reference to copied vector of borrowed level pointers. |
| `GetSource() const` | Source of first factor, or null for empty. |
| `GetTarget() const` | Target of final factor, or null for empty; corrected Eq. (40). |
| `IsComposableWith(other) const` | Nonempty operands, identical complete orders, and this target at or above the other's source. Checks ordered geometry, not weighted reachability. |
| `Compose(other) const` | Concatenates in operand order and reduces identities. No scalar weight. Incompatible operands throw `std::invalid_argument`. |
| `operator==(other) const` | Same complete order and canonical exact factors; all empty sentinels compare equal. Probability metadata does not enter factor equality. |
| `operator!=(other) const` | Negation of equality. |
| `ToString() const` | Human-readable tensor expression or empty marker. |

Constructor errors (`std::invalid_argument`) include null/duplicate ordering
levels, endpoints outside the order, non-descending transitions, empty path
factors, and overlap. Direct `DecayCoin` construction cannot validate graph
membership because it has no quiver; use `CAlgebra::Coin` for that additional
check. Use explicitly typed vectors for empty or ambiguous brace arguments.

A basis coincidence does not contain its probability. A geometrically valid
basis may have zero coefficient in a particular fiber because no weighted
connecting path exists.

## CAlgebra

Header: [`Algebra/CAlgebra.h`](../include/COINAlgebra/Algebra/CAlgebra.h).
Implements one fixed fiber `C_d` and Definitions III.5-6. Public nested types:

```cpp
struct Term { DecayCoin coin; double coefficient; };
using Vector = std::vector<Term>;
```

`Vector{}` is zero. There is no dedicated coincidence-vector class with
`PrintTable`, `AddTerm`, or overloaded arithmetic. Iterate its terms to display
it, and use the algebra methods to construct products and collect terms.

### Scalar connection: Eq. (41)

For basis coincidences `x,y`, the implemented connection is

\[
c_d(x,y)=\sum_{\gamma:t(x)\rightsquigarrow s(y)}
             \prod_{e\in\gamma}\tau_e.
\]

The length-zero connecting path contributes one when the endpoints coincide.
This is the finite connecting-path sum associated with Eq. (41), evaluated
directly rather than by calling `PathProjectors` and `PathAlgebra::PowerExpand`.
It depends only on the boundary endpoints and the transition coefficients of
`d`; initial stationary populations are excluded.

The algorithm seeds `weight[t(x)] = 1`, then visits levels in the supplied
highest-to-lowest order and updates `weight[v] += weight[u] * tau_(u,v)` for
each outgoing edge. All incoming contributions are complete before a level is
processed. It stops at `s(y)` before traversing a final observed transition.
Parallel edges contribute separately. No path objects need to be enumerated.

For example, connecting routes with weights `0.3` and `0.5*0.4` give connection
`0.5`. Directly adjacent factors have connection `1`. A unit connection does
not prove adjacency: separated routes can also sum to one.

### Multiplication and public functions

\[
x\cdot_d y=c_d(x,y)(x\otimes y),\qquad
\left(\sum_i a_i x_i\right)\cdot_d\left(\sum_j b_j y_j\right)
=\sum_{i,j}a_i b_j c_d(x_i,y_j)(x_i\otimes y_j).
\]

| Public function | Behavior |
| --- | --- |
| `CAlgebra(quiver, levelOrder, decay)` | Copies order, transitions, and defining decay vector. Requires every quiver level exactly once, all edges descending, and defining vector terms stationary or length one and in the quiver. |
| `Coin(const std::vector<DecayTransition>& transitions) const` | Builds a canonical coincidence and checks transition membership. |
| `Coin(const std::vector<DecayPath>& factors) const` | Same factory for stationary or connected path factors. |
| `Embed(const DecayVector& vector) const` | Converts path bases into coincidence bases, preserving coefficients and collecting duplicates. Unlike the defining vector, this input may contain longer paths. |
| `ScalarConnection(const DecayCoin& first, const DecayCoin& second) const` | Endpoint connecting-path sum above. Invalid context/membership throws; empty or geometrically incompatible operands give zero. |
| `Multiply(const DecayCoin& first, const DecayCoin& second) const` | Zero vector or one weighted joined basis term. Uses the fiber's scalar connection. |
| `Multiply(const Vector& first, const Vector& second) const` | Bilinear weighted product with exact basis collection and exact-zero removal. Validates both lists even if one is empty. |
| `Power(const Vector& vector, std::size_t n) const` | Positive integer power, left-associated. `n=1` validates and collects terms; `n=0` throws `std::invalid_argument` because no global identity is provided. |

The defining vector is a snapshot: changing the original `d` later does not
change this fiber. Construct another `CAlgebra` to use another defining vector.
Membership uses transition names and endpoint pointers, not intrinsic probability.

Construction from unordered factors is distinct from ordered multiplication:
`Coin({upper,lower})` and `Coin({lower,upper})` canonicalize to the same basis,
but `Multiply(lower,upper)` returns zero when the order is reversed. Multiplying
a transition by itself also returns zero. Stationary endpoint identities still
act locally; separated stationary factors can produce additional weighted terms,
so the path-algebra global identity must not be substituted here.

For valid ordered factors, the boundary dependence explains the associativity
construction in Lemma III.1 and Proposition III.1: each grouping introduces the
same two adjacent connection factors. The numerical tests exercise stationary,
connected, gapped, and invalid products; floating-point arithmetic is approximate.

### Branch-resolved probability vector

Compute `b = B(d)`, `tau = d-b`, and then

```cpp
auto P = algebra.Multiply(algebra.Embed(b), algebra.Embed(tau));
```

A term originating at level `i` and observing transition `p` has coefficient
`b_i * c_d(e_i,p) * tau_p`. The stationary factor `e_i` stays when separated
from `p`, preserving initial branch information; it reduces when `i=s(p)`.
No extra feeding factor should be applied afterward. Summing those coefficients
for each transition gives its ordinary feeding weight.

## DetectionMaps

Header: [DetectionMaps.h](../include/COINAlgebra/Detection/DetectionMaps.h).
A copied detector response model keyed by transition name. Efficiencies are
whole-array probabilities conditional on gamma emission. Require positive N,
finite `0 <= peak <= total <= 1`, and nonnegative finite alpha. Empty alpha maps
mean no IC; nonempty maps must cover every used edge. Maps never renormalize
physical branches or initial populations.

| Public function | Behavior |
| --- | --- |
| `DetectionMaps(peak, total, detectorCount, conversion = {})` | Stores and validates response inputs. |
| `DetectorCount() const` | Number of identical isotropic detectors. |
| `FullEnergyHit(vector) const` | Path or coincidence vector: multiply observed edges by `q*peak`, with `q=1/(1+alpha)`. |
| `TotalHit(vector) const` | Path or coincidence vector: multiply by `q*total/N` for one detector. |
| `SummingOut(vector) const` | Path or coincidence vector: multiply by `1-q*total/N`. |
| `AvoidDetectors(pathVector, m) const` | Multiply each path edge by `1-m*q*total/N`; require `m<=N`. Zero means identity. |
| `SummingInExpansion(pathAlgebra, tau) const` | Connected sums: `h + h*h/N + ...`, positive powers only. |
| `SummingInExpansion(coincidenceFiber, tau) const` | Also includes disconnected hits; products use the explicitly supplied fiber's connection weights. |

Stationary factors are preserved. Summing expansions require original physical
single-edge tau, not a previously mapped hit vector. A map on a coincidence
vector changes observed factors only; construct a new CAlgebra from mapped tau
to change hidden connection weights. This distinction implements the paper's
separation between a fiber and the vectors being multiplied inside it.

### Coincidence detection and summing example

```cpp
// b contains stationary initial populations, tau physical gamma+IC edges,
// E stationary terminal levels. order lists every level from high to low.
CAlgebra physical(quiver, order, b + tau);
auto pairs = physical.Multiply(physical.Embed(b),
                              physical.Power(physical.Embed(tau), 2));
auto detectedPairs = maps.FullEnergyHit(pairs); // independent, no summing
CAlgebra out(quiver, order, maps.SummingOut(tau));
auto spectrum = out.Multiply(out.Multiply(out.Embed(b),
    maps.SummingInExpansion(out, tau)), out.Embed(E)); // Eq. 70
```

For two distinct clean peaks, use `AvoidDetectors(tau,2)` as the fiber, two
observed h factors, and multiply by `(N-1)/N`. For N=1 that observable is zero.
General gated sums of photon groups are not implemented: photon membership in a
summed tensor is not a resolved-energy gate. The Eq. (72) limitation and exact
counterexample are in the [validation baseline](mathematics/coincidence-validation.md).

The [Ba-133 macro](../tests/integration/test_133Ba_gamma_quiver.C) prints all
nonzero physical, emission and detected coincidence orders, same-detector sums,
clean pairs, and G1/G2 membership gates. Its detector responses are illustrative.
The [synthetic calculation](../examples/calculations/coincidence.C) provides
small reproducible numbers. [Independent enumeration tests](../tests/core/test_coincidence_detection.C)
check cascade/subset weights, while [path summing tests](../tests/core/test_summing.C)
check connected sums. See also [IC conventions](mathematics/internal-conversion.md)
and [detection and summing](mathematics/detection-summing.md).

## DQStudio architecture

The physical `DecayQuiver` and its energies remain independent of presentation.
`Studio::Document` owns the quiver, saved vectors, title and JSON metadata.
Metadata holds initial populations, IC, efficiency calibration, named groups and
explicit subquivers. Bands, cascades and user groups are document membership;
colors, camera, highlights, collapse and energy scaling belong to views.
Quiver, Band and Focus modes share the `QuiverView` renderer.

```text
DecayQuiver + DecayVector
          |
Studio::Document (JSON + physical analysis inputs)
          |
StudioModel          ViewState
branching/response   membership/selection/style/energy coordinates
          |                 |
          MainWindow ---- QuiverView
          analysis        shared renderer for Quiver/Band/Focus
```

Read the [view and band guide](dqstudio-bands.md),
[JSON contract](studio-json.md), and [analysis workflow](studio-analysis.md)
for persistence details and user-facing behavior. GUI components require Qt5;
the core library remains independent of Qt and ROOT.

### Document

Header: [QuiverJson.h](../gui/QuiverJson.h). Qualified name: `Studio::Document`.

| Member or operation | Purpose |
| --- | --- |
| `quiver` | Unique ownership of graph; vectors borrow its level pointers. |
| `vectors` | Saved path vectors with physical coefficients. |
| `metadata`, `title` | Analysis inputs, presentation state and document title. |
| `Studio::readJson(bytes)` | Constructs and validates a new document; a failed load leaves the open document intact. |
| `Studio::createDecayVector(quiver, metadata)` | Combines stationary initial populations and physical transition branches. |

### StudioModel

Namespace functions, not a C++ class. Header: [StudioModel.h](../gui/StudioModel.h).

| Function | Purpose |
| --- | --- |
| `branchingVector(quiver, metadata)` | Stationary initial populations; distinct from outgoing transition probabilities. |
| `transitionVector(quiver)` | Physical single-edge coefficients. |
| `levelEnergy(quiver, metadata, index)` | Retrieves the core level energy used by analysis. |
| `readEfficiencyCsv(bytes)` | Parses calibrated energy/efficiency samples, including GRIFFIN headers. |
| `efficiencyMap(quiver, metadata)` | Interpolates at absolute endpoint-energy differences; no extrapolation. |
| `conversionMap(quiver, metadata)` | Validated per-edge IC; absent metadata means alpha=0. |
| `validateStudio(quiver, metadata)` | Checks analysis metadata and required response inputs. |

The GRIFFIN CSV uses `Energy[keV], HPGe`; generic `energy_keV,efficiency` is
also accepted. Efficiencies are fractions, not percentages. Known core energies
are exported in aligned `level_energies_keV` entries; null means unknown.
Legacy energy metadata is migrated on import. Physical, branching and transition
vectors must remain distinct from response-weighted calculation results.

### GraphSelection

Header: [ViewState.h](../gui/ViewState.h). Qualified name: `Studio::GraphSelection`.

| Field | Purpose |
| --- | --- |
| `levels`, `transitions` | Boolean membership masks for selected graph objects. |
| `contextLevels`, `contextTransitions` | Context masks used by focus rendering. |

### ViewState

Namespace functions, not a C++ class. Header: [ViewState.h](../gui/ViewState.h).

| Function | Purpose |
| --- | --- |
| `normalizeViewMetadata(metadata)` | Normalizes saved view state and defaults. |
| `layoutGroups(metadata)` | Groups used for band layout. |
| `selectGraph(quiver, metadata, focus)` | Computes selection and context masks. |
| `objectStyle(metadata, groupId, objectId, transition)` | Resolves saved object styling. |
| `validateViewMetadata(quiver, metadata)` | Checks membership and view references. |
| `removeViewLevel(metadata, index)` | Remaps level references after deletion. |
| `pruneViewTransitions(metadata, quiver)` | Removes stale transition references. |
| `energyCoordinates(quiver, metadata, visible)` | Physical/compressed/uniform vertical coordinates; explicit energy modes require known energies. |

### MainWindow

Header: [MainWindow.h](../gui/MainWindow.h). Coordinates document editing,
loading/export, branching input, efficiency import, analysis panels and saved
views. View-related operations are implemented in `MainWindowViews.cpp`.
| Public function | Purpose |
| --- | --- |
| `MainWindow()` | Constructs the editor and connects document, view and analysis controls. |

The coincidence panel evaluates both allowed temporal orders for two distinct
selected transitions, preserves individual parallel-arrow shares, and applies
IC/efficiency to observed transitions. Its independent detection probability
does not include general detector-pair summing corrections.

### QuiverView

Header: [QuiverView.h](../gui/QuiverView.h). Shared graphics renderer for all
three views, with selection, pan/zoom, band movement, level scaling, styles,
collapse and highlighting. Display transforms do not change physical energies.

| Public function or signal | Purpose |
| --- | --- |
| `QuiverView(parent)` | Creates the graphics view. |
| `setQuiver(q)` | Sets the borrowed graph rendered by this view. |
| `setMetadata(metadata)` | Supplies document presentation metadata. |
| `viewState()`, `restoreView(state)` | Read or restore camera/view state. |
| `setMode(mode)`, `mode()` | Select or read the active view mode. |
| `setEnergyScale(scale)`, `configureView(state)` | Configure energy scaling and saved view settings. |
| `refresh()` | Rebuild the scene from current data. |
| `focusOnLevel(index)`, `showAllLevels()` | Focus or restore the displayed level set. |
| `levelDeleteRequested(index)`, `levelRenamed(index, newName)` | Report user edits to the controller. |
| `transitionEdited(index, probability)` | Report edited transition probability. |
| `appearanceChanged(metadata)` | Persist changes to presentation metadata. |

### TransitionEditor

Namespace function, not a C++ class. Header: [TransitionEditor.h](../gui/TransitionEditor.h).

| Function | Purpose |
| --- | --- |
| `Studio::editTransition(parent, transition, levels)` | Applies accepted source, target and probability together; returns whether edits were accepted. |

## Nuclear-data records and readers

These types preserve external records before constructing mathematical objects.
They use lower-case method names. Energies are in keV and half-lives in seconds.
Public record fields and exact constructor signatures are also listed in the
[declaration appendix](#complete-public-declarations).

### PhotonTransition

Header: [`NuclearData/PhotonEvaporationReader.h`](../include/COINAlgebra/NuclearData/PhotonEvaporationReader.h).

Fields: `daughterLevel` (ID), `energy_keV`, `relativeIntensity`, `multipolarity`,
`mixingRatio`, `conversionCoefficient`, and
`std::array<double,10> shellConversionProbabilities`. Shell order is K, L1, L2,
L3, M1, M2, M3, M4, M5, outer shells. The multipolarity integer encoding is
preserved from the input format; it is not a class-level enum.

| Public function | Behavior |
| --- | --- |
| `PhotonTransition()` | Default record with initialized values. |
| `PhotonTransition(daughterLevel, energy_keV, relativeIntensity, multipolarity, mixingRatio, conversionCoefficient, shellConversionProbabilities)` | Copies every supplied property into the record. |
| `gammaProbability() const` | `1/(1+alpha)`, where `alpha = conversionCoefficient`. |
| `conversionProbability() const` | `alpha/(1+alpha)`. |

The helpers do not validate a manually assigned conversion coefficient. They
are emission-versus-conversion fractions, not outgoing branching normalization.

### PhotonLevel

Fields: `id`, `floating` (uninterpreted floating-level flag), `energy_keV`,
`halfLife_s`, `jpi`, and `std::vector<PhotonTransition> transitions`.

| Public function | Behavior |
| --- | --- |
| `PhotonLevel()` | Defaults include ID `-1`, floating `"-"`, energy `0`, half-life `-1`, JPi `99`, and no transitions. |
| `PhotonLevel(id, floating, energy_keV, halfLife_s, jpi)` | Initializes level properties and an empty transition list. |
| `numberOfGammas() const` | Size of the transition list. |
| `isStable() const` | Tests `halfLife_s < 0`. |
| `hasKnownJPi() const` | Tests `jpi != 99`. |

### PhotonIsotope

| Public function | Behavior |
| --- | --- |
| `PhotonIsotope()` | Z and A zero, no levels. |
| `PhotonIsotope(int atomicNumber, int massNumber)` | Stores Z and A. |
| `atomicNumber() const`, `massNumber() const` | Return Z and A. |
| `levels() const`, `levels()` | Const/mutable reference to the owned `std::vector<PhotonLevel>`. |
| `numberOfLevels() const` | Vector size. |
| `level(int id) const`, `level(int id)` | Const/mutable reference by level ID, not vector index. Missing ID throws `std::out_of_range`. |
| `addLevel(const PhotonLevel& level)` | Appends a copy; does not enforce unique IDs. |

Mutating/resizing the vector can invalidate references. Lookups return the first
matching ID, so preserve uniqueness when constructing records manually.

### PhotonEvaporationReader

| Public function | Behavior |
| --- | --- |
| `explicit PhotonEvaporationReader(const std::string& dataDirectory)` | Stores directory; empty directory throws `std::invalid_argument`. |
| `read(int atomicNumber, int massNumber) const` | Parses the isotope file and returns `PhotonIsotope`. Missing files and malformed supported-format data report exceptions. |
| `filePath(int atomicNumber, int massNumber) const` | Returns `<directory>/z<Z>.a<A>`; nonpositive Z or A throws `std::invalid_argument`. Does not check file existence. |
| `dataDirectory() const` | Const reference to configured directory. |

### RadioactiveDecayChannel

Header: [`NuclearData/RadioactiveDecayReader.h`](../include/COINAlgebra/NuclearData/RadioactiveDecayReader.h).

Fields: `decayType`, `daughterEnergy_keV`, `daughterFloating`,
`branchingPercentage`, `qValue_keV`, and optional `forbiddenness`.

| Public function | Behavior |
| --- | --- |
| `RadioactiveDecayChannel()` | Initializes an empty/default channel. |
| `RadioactiveDecayChannel(decayType, daughterEnergy_keV, daughterFloating, branchingPercentage, qValue_keV, forbiddenness = "")` | Copies supplied properties. |
| `modeFraction() const` | `branchingPercentage / 100`, the fraction within its mode. |

### RadioactiveDecayMode

Fields: `decayType`, `totalBranchingFraction`, and
`std::vector<RadioactiveDecayChannel> channels`.

| Public function | Behavior |
| --- | --- |
| `RadioactiveDecayMode()` | Initializes an empty/default mode. |
| `RadioactiveDecayMode(decayType, totalBranchingFraction)` | Stores mode properties; channel list initially empty. |
| `numberOfChannels() const` | Channel count. |
| `addChannel(const RadioactiveDecayChannel& channel)` | Appends a copy. |
| `channelBranchingFraction(std::size_t index) const` | `totalBranchingFraction * channels[index].modeFraction()`. Invalid index throws `std::out_of_range`. |

The total mode fraction is already a fraction, whereas the channel field is a
percentage. Do not divide both by 100.

### RadioactiveParentState

Fields: `energy_keV`, `floating`, `halfLife_s`, and
`std::vector<RadioactiveDecayMode> decayModes`.

| Public function | Behavior |
| --- | --- |
| `RadioactiveParentState()` | Initializes a default state with half-life zero. |
| `RadioactiveParentState(energy_keV, floating, halfLife_s)` | Stores state properties and an empty mode list. |
| `numberOfDecayModes() const` | Number of modes. |
| `isStable() const` | Tests `halfLife_s <= 0`; this differs from `PhotonLevel`'s strict negative test. |
| `addDecayMode(const RadioactiveDecayMode& mode)` | Appends a copy. |

### RadioactiveIsotope

| Public function | Behavior |
| --- | --- |
| `RadioactiveIsotope()` | Z and A zero, no states. |
| `RadioactiveIsotope(int atomicNumber, int massNumber)` | Stores Z and A. |
| `atomicNumber() const`, `massNumber() const` | Return Z and A. |
| `parentStates() const`, `parentStates()` | Const/mutable reference to owned state vector. |
| `numberOfParentStates() const` | State count. |
| `parentState(double energy_keV) const`, `parentState(double energy_keV)` | Const/mutable reference to first state within strictly `1e-9` keV of the energy. Missing match throws `std::out_of_range`; floating flag is not part of lookup. |
| `addParentState(const RadioactiveParentState& state)` | Appends a copy. |

### RadioactiveDecayReader

| Public function | Behavior |
| --- | --- |
| `explicit RadioactiveDecayReader(const std::string& dataDirectory)` | Stores the directory. |
| `read(int atomicNumber, int massNumber) const` | Parses parent states/modes/channels and returns `RadioactiveIsotope`. File and format errors report exceptions. |
| `filePath(int atomicNumber, int massNumber) const` | `<directory>/z<Z>.a<A>`; nonpositive Z or A throws `std::invalid_argument`. |
| `dataDirectory() const` | Const reference to configured directory. |

Neither reader automatically finds a Geant4 installation or constructs `b` or
`tau`. Supply the appropriate directory explicitly and keep parsing separate
from the physical choice of populations and modeled transitions.

## DecayQuiverBuilder

Header: [`Builders/DecayQuiverBuilder.h`](../include/COINAlgebra/Builders/DecayQuiverBuilder.h).
Two overloads of the public static builder operation:

```cpp
static DecayQuiver BuildGammaQuiver(
    const RadioactiveIsotope& parent,
    const PhotonIsotope& daughter,
    const std::string& decayType = "BetaPlus",
    double energyTolerance_keV = 1.0);
```

The builder uses the **first parent state**, selects matching radioactive decay
modes, matches channel daughter energies to photon levels within tolerance,
and recursively includes levels reachable along the recorded gamma transitions.
It converts gamma intensities to total gamma-plus-IC weights before normalizing
positive outgoing branches at each source:

\[
P(i\to j\mid i)=I_{ij}(1+\alpha_{ij})/\sum_k I_{ik}(1+\alpha_{ik}).
\]

`"EC"` and `"ElectronCapture"` combine the recognized EC modes including K-, L-,
and M-shell capture. Individual shell strings select that mode; other strings
match exactly. The nearest energy within tolerance wins; equally close matches
use the last encountered level. Floating flags are not matched.

The result is the daughter's gamma quiver. It does **not** insert the parent
as a branch vertex or construct initial stationary population coefficients.
The resulting coefficients include conversion feeding. Apply `1/(1+alpha)`
only when calculating gamma emission; do not apply it again to the physical
branches. Build the initial-population vector separately.

Empty inputs, missing modes, unmatched energies, and unavailable daughter levels
report exceptions. Names are based on energy strings; equal formatted level
energies can cause duplicate-name rejection. Quiver copying limitations apply.

Example, using the bundled data directories from the repository root:

```cpp
#include "COINAlgebra/Builders/DecayQuiverBuilder.h"

void reader_example()
{
    RadioactiveDecayReader radioactive("RadioactiveDecay5.5");
    PhotonEvaporationReader photons("PhotonEvaporation5.5");
    auto parent = radioactive.read(12, 22);
    auto daughter = photons.read(11, 22);
    auto quiver = DecayQuiverBuilder::BuildGammaQuiver(parent, daughter);
    quiver.Print();
}
```

The builder also exposes an overload with an output conversion-coefficient map;
it extracts alpha for each named transition while using gamma-plus-IC branches.
Physical level energies are copied from reader records, not inferred from names.
The exact overloads appear in the generated header appendix.

## ROOT environment

Header: [`ROOT/COINAlgebra.h`](../include/COINAlgebra/ROOT/COINAlgebra.h).
`COINAlgebra` is the application wrapper; `CAlgebra` is the mathematical fiber.

| Public function | Behavior |
| --- | --- |
| `COINAlgebra()` | Creates wrapper with current version string `0.1.0`. |
| `~COINAlgebra()` | Destructor; does not own a `TRint`. |
| `PrintBanner() const` | Prints application banner. |
| `PrintHelp() const` | Prints available command guidance. |
| `PrintVersion() const` | Prints version. |
| `ConfigureROOT(TRint& rootApp)` | Sets the interactive prompt. |
| `InitializeEnvironment(TRint& rootApp)` | Runs `$COINALGEBRA_HOME/config/COINAlgebraLogon.C`; warns and returns if the environment/path is unavailable. |
| `GetVersion() const` | Const reference to version string. |
| Free `COINAlgebraHelp()` | Creates a wrapper and prints help. Declared also in `ROOT/Commands.h`. |
| Free `COINAlgebraVersion()` | Creates a wrapper and prints version. |

The standard startup supplies convenience commands such as `help()` and
`version()`. Use `.x examples/basic/example_vector.C` to execute the example.
The ROOT dictionary includes the new coincidence types and their term vector;
`CAlgebra` is exposed for interpretation without requesting a `+` streamer.
Dictionary visibility is not a promise of safe persistence of borrowed pointers.

Canonical headers follow module directories. Flat compatibility headers such as
`COINAlgebra/DecayPath.h` and `COINAlgebra/CAlgebra.h` forward to them. For ROOT
macros, include the foundational level/quiver headers before the path and algebra
headers, as in the worked example, to avoid recursive dictionary include parsing.

For a standalone executable after building the core library, compile with C++17,
the `include` directory, and `lib/libCOINAlgebraCore.a`. In the project CMake graph,
link to `COINAlgebra::Core`. ROOT wrapper methods require the ROOT-enabled library.

## Validation and mathematical boundaries

The code currently implements the path machinery and one fixed coincidence
fiber. Automatic `N(v)` ordering, a dedicated coincidence-vector arithmetic
class, reusable coincidence gate-projector class, fiber transport, and a full
bundle API are not provided by these public headers. Detection maps are provided;
transition-membership gate projectors are demonstrated locally in the Ba-133 macro. `PathProjectors` accepts
`DecayVector`, not `CAlgebra::Vector`.

Key distinctions when checking a calculation:

1. Basis equality is stricter than endpoint equivalence.
2. Intrinsic transition metadata and vector coefficients are separate.
3. Non-overlap is geometric; nonzero connection depends on `d`.
4. Basis construction sorts factors; multiplication preserves operand order.
5. `ScalarConnection` stops before the observed lower transition;
   `PathConnection` includes it through a final `tau` factor.
6. Acyclicity and physically meaningful inputs are assumptions for path-based
   probability results, even when a permissive container accepts other inputs.
7. Stationary branching populations are not stationary path intrinsic weights.
8. Different numerical simplification policies can affect very small terms.

Useful executable coverage:

| Files | Coverage |
| --- | --- |
| [`tests/core/test_path.C`](../tests/core/test_path.C) | Stationary paths, composition, and exact path behavior. |
| [`tests/core/test_vector.C`](../tests/core/test_vector.C) | Sparse path-vector arithmetic. |
| [`tests/core/test_quiver.C`](../tests/core/test_quiver.C) | Quiver construction and queries. |
| [`tests/algebra/test_pathalgebra.C`](../tests/algebra/test_pathalgebra.C) | Path products, powers, and forms. |
| [`tests/algebra/test_forms_projectors.C`](../tests/algebra/test_forms_projectors.C) | Endpoint forms and projector behavior. |
| [`tests/core/test_coin.C`](../tests/core/test_coin.C) | Non-overlap, identities, connection sums, vector products, powers, and associativity across 6,859 basis triples. |
| [`tests/nuclear_data/test_decay_readers.C`](../tests/nuclear_data/test_decay_readers.C) | Reader parsing behavior. |
| [`examples/basic/example_vector.C`](../examples/basic/example_vector.C) | Branch-resolved probability vector and feeding comparison. |

Run `scripts/test_coincidences.sh` for native and enabled ROOT coincidence tests,
or `ctest --test-dir build --output-on-failure` for the configured suite.

## Complete public declarations

This appendix is populated from the canonical library and DQStudio headers at
build time, so new overloads cannot remain hidden behind a stale hand-copied
inventory. Comments are omitted for compactness; access labels distinguish public
APIs from private helpers. Follow the linked source pages for the original comments.
Forwarding compatibility headers are omitted; their canonical targets are shown.

### include/COINAlgebra/Core/DecayCoin.h

Source: [DecayCoin.h](../include/COINAlgebra/Core/DecayCoin.h).

```cpp
class DecayCoin
{
public:

    DecayCoin() = default;

    DecayCoin(const std::vector<DecayLevel*>& levelOrder,
              const std::vector<DecayTransition>& transitions);

    DecayCoin(const std::vector<DecayLevel*>& levelOrder,
              const std::vector<DecayPath>& factors);

    bool Empty() const;

    bool IsStationary() const;

    std::size_t Degree() const;

    const std::vector<DecayPath>& GetFactors() const;

    const std::vector<DecayLevel*>& GetLevelOrder() const;

    DecayLevel* GetSource() const;
    DecayLevel* GetTarget() const;

    bool IsComposableWith(const DecayCoin& other) const;

    DecayCoin Compose(const DecayCoin& other) const;

    bool operator==(const DecayCoin& other) const;
    bool operator!=(const DecayCoin& other) const;

    std::string ToString() const;
private:

    std::vector<DecayLevel*> fLevelOrder;

    std::vector<DecayPath> fFactors;

    std::size_t Rank(DecayLevel* level) const;
};
```

### include/COINAlgebra/Core/DecayLevel.h

Source: [DecayLevel.h](../include/COINAlgebra/Core/DecayLevel.h).

```cpp
class DecayLevel
{
public:

    DecayLevel();

    explicit DecayLevel(
        const std::string& name
    );

    DecayLevel(const std::string& name, double energy_keV);

    bool HasEnergy() const;
    double GetEnergy() const;
    void SetEnergy(double energy_keV);
    void ClearEnergy();

    const std::string& GetName() const;

    void SetName(
        const std::string& name
    );

    void Print() const;

private:

    std::string fName;
    double fEnergy = 0.0;
    bool fHasEnergy = false;
};
```

### include/COINAlgebra/Core/DecayPath.h

Source: [DecayPath.h](../include/COINAlgebra/Core/DecayPath.h).

```cpp
class DecayLevel;

class DecayPath
{
public:

    DecayPath();

    explicit DecayPath(
        DecayLevel* level
    );

    explicit DecayPath(
        const DecayTransition& transition
    );

    explicit DecayPath(
        const std::vector<DecayTransition>& transitions
    );

    bool Empty() const;

    bool IsStationary() const;

    std::size_t Length() const;

    const std::vector<DecayTransition>&
    GetTransitions() const;

    const DecayTransition&
    GetTransition(
        std::size_t index
    ) const;

    DecayLevel* GetSource() const;

    DecayLevel* GetTarget() const;

    double GetProbability() const;

    bool IsComposableWith(
        const DecayPath& other
    ) const;

    DecayPath Compose(
        const DecayPath& other
    ) const;

    bool operator==(
        const DecayPath& other
    ) const;

    bool operator!=(
        const DecayPath& other
    ) const;

    bool HasSameSource(
        const DecayPath& other
    ) const;

    bool HasSameTarget(
        const DecayPath& other
    ) const;

    bool HasSameEndpoints(
        const DecayPath& other
    ) const;

    std::string ToString() const;

private:

    std::vector<DecayTransition> fTransitions;

    DecayLevel* fSource;

    DecayLevel* fTarget;

    void Validate() const;
};
```

### include/COINAlgebra/Core/DecayQuiver.h

Source: [DecayQuiver.h](../include/COINAlgebra/Core/DecayQuiver.h).

```cpp
class DecayQuiver
{
public:

    DecayQuiver();

    ~DecayQuiver();

    DecayLevel* AddLevel(
        const std::string& name
    );

    DecayLevel* AddLevel(const std::string& name, double energy_keV);

    DecayLevel* GetLevel(
        const std::string& name
    ) const;

    const std::vector<DecayLevel*>& GetLevels() const;

    DecayTransition* AddTransition(
        const std::string& name,
        DecayLevel* source,
        DecayLevel* target,
        double probability = 1.0
    );

    DecayTransition* GetTransition(
        const std::string& name
    ) const;

    const std::vector<DecayTransition*>& GetTransitions() const;

    bool RemoveLevel(const DecayLevel* level);

    bool RemoveTransition(const DecayTransition* transition);

    bool IsComposable(
        const DecayTransition* first,
        const DecayTransition* second
    ) const;

    bool HasDirectTransition(
        const DecayLevel* source,
        const DecayLevel* target
    ) const;

    double GetOutgoingProbability(
        const DecayLevel* level
    ) const;

    bool IsNormalized(
        const DecayLevel* level,
        double tolerance = 1.0e-12
    ) const;

    void Print() const;

    void ExportJson(
        const std::string& filename,
        const std::vector<DecayVector>& vectors = {},
        const std::string& title = "",
        const std::unordered_map<std::string, double>& conversionCoefficients = {},
        const std::vector<double>& initialPopulations = {}
    ) const;

private:

    std::vector<DecayLevel*> fLevels;

    std::vector<DecayTransition*> fTransitions;
};
```

### include/COINAlgebra/Core/DecayTransition.h

Source: [DecayTransition.h](../include/COINAlgebra/Core/DecayTransition.h).

```cpp
class DecayLevel;

class DecayTransition
{
public:

    DecayTransition();

    DecayTransition(
        const std::string& name,
        DecayLevel* source,
        DecayLevel* target,
        double probability = 1.0
    );

    const std::string& GetName() const;

    DecayLevel* GetSource() const;

    DecayLevel* GetTarget() const;

    double GetProbability() const;

    void SetName(
        const std::string& name
    );

    void SetSource(
        DecayLevel* source
    );

    void SetTarget(
        DecayLevel* target
    );

    void SetProbability(
        double probability
    );

    void Print() const;

private:

    std::string fName;

    DecayLevel* fSource;

    DecayLevel* fTarget;

    double fProbability;
};
```

### include/COINAlgebra/Core/DecayVector.h

Source: [DecayVector.h](../include/COINAlgebra/Core/DecayVector.h).

```cpp
class DecayVector
{
public:

    struct Term
    {
        DecayPath path;
        double coefficient;
    };

    DecayVector();

    explicit DecayVector(
        const DecayPath& path,
        double coefficient = 1.0
    );

    bool Empty() const;

    std::size_t Size() const;

    const std::vector<Term>& GetTerms() const;

    void AddTerm(
        const DecayPath& path,
        double coefficient
    );

    DecayVector operator+(
        const DecayVector& other
    ) const;

    DecayVector operator-(
        const DecayVector& other
    ) const;

    DecayVector& operator+=(
        const DecayVector& other
    );

    DecayVector& operator-=(
        const DecayVector& other
    );

    DecayVector operator*(
        double scalar
    ) const;

    DecayVector& operator*=(
        double scalar
    );

    friend DecayVector operator*(
        double scalar,
        const DecayVector& vector
    );

    DecayVector ApplyConversionMap(
        const std::unordered_map<std::string, double>& conversionCoefficients
    ) const;

    DecayVector ApplyDetectionMap(
        const std::unordered_map<std::string, double>& efficiencies
    ) const;

    std::string ToString() const;

    void Print() const;
    void PrintTable() const;
    void PrintTable(std::ostream& out) const;

private:

    std::vector<Term> fTerms;

    static bool PathsEqual(
        const DecayPath& first,
        const DecayPath& second
    );

    void Simplify();

};
```

### include/COINAlgebra/Algebra/CAlgebra.h

Source: [CAlgebra.h](../include/COINAlgebra/Algebra/CAlgebra.h).

```cpp
class DecayQuiver;

class CAlgebra
{
public:

    struct Term { DecayCoin coin; double coefficient; };
    using Vector = std::vector<Term>;

    CAlgebra(const DecayQuiver& quiver,
             const std::vector<DecayLevel*>& levelOrder,
             const DecayVector& decay);

    DecayCoin Coin(const std::vector<DecayTransition>& transitions) const;

    DecayCoin Coin(const std::vector<DecayPath>& factors) const;

    Vector Embed(const DecayVector& vector) const;

    double ScalarConnection(const DecayCoin& first, const DecayCoin& second) const;

    Vector Multiply(const DecayCoin& first, const DecayCoin& second) const;

    Vector Multiply(const Vector& first, const Vector& second) const;

    Vector Power(const Vector& vector, std::size_t n) const;
private:

    std::vector<DecayLevel*> fLevelOrder;

    std::vector<DecayTransition> fTransitions;

    DecayVector fDecay;

    void Validate(const DecayCoin& coin) const;

    static void AddTerm(Vector& vector, const DecayCoin& coin, double coefficient);
};
```

### include/COINAlgebra/Algebra/PathAlgebra.h

Source: [PathAlgebra.h](../include/COINAlgebra/Algebra/PathAlgebra.h).

```cpp
class DecayQuiver;

class PathAlgebra
{
public:

    PathAlgebra(
        const DecayQuiver& quiver
    );

    DecayVector Identity() const;

    std::size_t MaxPower() const;

    const DecayQuiver& GetQuiver() const;

    DecayVector Multiply(
        const DecayVector& first,
        const DecayVector& second
    ) const;

    DecayVector Power(
        const DecayVector& d,
        std::size_t n
    ) const;

    DecayVector PowerExpand(
        const DecayVector& d,
        std::size_t maxPower
    ) const;

    double SourceForm(
        const DecayVector& first,
        const DecayVector& second
    ) const;

    double TargetForm(
        const DecayVector& first,
        const DecayVector& second
    ) const;

    double PathForm(
        const DecayVector& first,
        const DecayVector& second
    ) const;

private:

    const DecayQuiver* fQuiver;
};
```

### include/COINAlgebra/Algebra/PathProjectors.h

Source: [PathProjectors.h](../include/COINAlgebra/Algebra/PathProjectors.h).

```cpp
class DecayLevel;

class PathProjectors
{
public:

    PathProjectors();

    DecayVector SourceProjector(
        const DecayVector& vector,
        DecayLevel* source
    ) const;

    DecayVector TargetProjector(
        const DecayVector& vector,
        DecayLevel* target
    ) const;

    DecayVector SourceVertexProjector(
        const DecayVector& vector
    ) const;

    DecayVector TargetVertexProjector(
        const DecayVector& vector
    ) const;

    DecayVector BranchingProjector(
        const DecayVector& vector
    ) const;
};
```

### include/COINAlgebra/Probability/DecayProbability.h

Source: [DecayProbability.h](../include/COINAlgebra/Probability/DecayProbability.h).

```cpp
class PathAlgebra;
class PathProjectors;
class DetectionMaps;

class DecayProbability
{
public:

    DecayProbability(
        const PathAlgebra& algebra,
        const PathProjectors& projectors
    );

    DecayVector FeedingVector(
        const DecayVector& decay
    ) const;

    DecayVector DetectionFeedingVector(
        const DecayVector& decay,
        const std::unordered_map<std::string, double>& efficiencies
    ) const;

    DecayVector EmissionFeedingVector(
        const DecayVector& decay,
        const std::unordered_map<std::string, double>& conversionCoefficients
    ) const;

    DecayVector DetectionFeedingVector(
        const DecayVector& decay,
        const std::unordered_map<std::string, double>& efficiencies,
        const std::unordered_map<std::string, double>& conversionCoefficients
    ) const;

    DecayVector SummingFeedingVector(
        const DecayVector& decay, const DetectionMaps& maps
    ) const;

    double FeedingProbability(
        const DecayVector& decay,
        const DecayPath& path
    ) const;

    double PathConnection(
         const DecayVector& decay,
         const DecayPath& path_i,
         const DecayPath& path_j
    ) const;

    double CoincidenceProbability(
         const DecayVector& decay,
         const DecayPath& path_i,
         const DecayPath& path_j
    ) const;

private:

    const PathAlgebra* fAlgebra;
    const PathProjectors* fProjectors;
};
```

### include/COINAlgebra/Detection/DetectionMaps.h

Source: [DetectionMaps.h](../include/COINAlgebra/Detection/DetectionMaps.h).

```cpp
class PathAlgebra;

class DetectionMaps
{
public:
    using EfficiencyMap = std::unordered_map<std::string, double>;
    DetectionMaps(const EfficiencyMap& peakEfficiencies,
                  const EfficiencyMap& totalEfficiencies,
                  std::size_t detectorCount,
                  const EfficiencyMap& conversionCoefficients = {});

    std::size_t DetectorCount() const;

    DecayVector FullEnergyHit(const DecayVector& vector) const;
    DecayVector TotalHit(const DecayVector& vector) const;
    DecayVector SummingOut(const DecayVector& vector) const;

    DecayVector AvoidDetectors(const DecayVector& vector, std::size_t m) const;

    CAlgebra::Vector FullEnergyHit(const CAlgebra::Vector& vector) const;
    CAlgebra::Vector TotalHit(const CAlgebra::Vector& vector) const;
    CAlgebra::Vector SummingOut(const CAlgebra::Vector& vector) const;

    CAlgebra::Vector SummingInExpansion(const CAlgebra& fiber,
                                       const DecayVector& transition) const;

    DecayVector SummingInExpansion(const PathAlgebra& algebra,
                                  const DecayVector& transition) const;
private:
    EfficiencyMap fPeak, fTotal, fConversion;
    std::size_t fDetectorCount;
    DecayVector Apply(const DecayVector& vector, int kind, std::size_t avoided = 1) const;
    CAlgebra::Vector Apply(const CAlgebra::Vector& vector, int kind) const;
};
```

### include/COINAlgebra/Builders/DecayQuiverBuilder.h

Source: [DecayQuiverBuilder.h](../include/COINAlgebra/Builders/DecayQuiverBuilder.h).

```cpp
class DecayQuiverBuilder
{
public:

    static DecayQuiver BuildGammaQuiver(
        const RadioactiveIsotope& parent,
        const PhotonIsotope& daughter,
        const std::string& decayType = "BetaPlus",
        double energyTolerance_keV = 1.0
    );

    static DecayQuiver BuildGammaQuiver(
        const RadioactiveIsotope& parent,
        const PhotonIsotope& daughter,
        std::unordered_map<std::string, double>& conversionCoefficients,
        const std::string& decayType = "BetaPlus",
        double energyTolerance_keV = 1.0
    );

private:

    static const PhotonLevel* FindLevelByEnergy(
        const PhotonIsotope& daughter,
        double energy_keV,
        double tolerance_keV
    );

    static void CollectReachableGammaLevels(
        const PhotonIsotope& daughter,
        int levelID,
        std::set<int>& reachableLevels
    );

    static bool IsElectronCaptureMode(
        const std::string& decayType
    );

    static bool ModeMatchesDecayType(
        const std::string& mode,
        const std::string& decayType
    );
};
```

### include/COINAlgebra/NuclearData/PhotonEvaporationReader.h

Source: [PhotonEvaporationReader.h](../include/COINAlgebra/NuclearData/PhotonEvaporationReader.h).

```cpp
struct PhotonTransition
{

    int daughterLevel;

    double energy_keV;

    double relativeIntensity;

    int multipolarity;

    double mixingRatio;

    double conversionCoefficient;

    std::array<double, 10> shellConversionProbabilities{};

    PhotonTransition();

    PhotonTransition(
        int daughterLevel,
        double energy_keV,
        double relativeIntensity,
        int multipolarity,
        double mixingRatio,
        double conversionCoefficient,
        const std::array<double, 10>& shellConversionProbabilities
    );

    double gammaProbability() const;

    double conversionProbability() const;
};

struct PhotonLevel
{

    int id;

    std::string floating;

    double energy_keV;

    double halfLife_s;

    double jpi;

    std::vector<PhotonTransition> transitions;

    PhotonLevel();

    PhotonLevel(
        int id,
        const std::string& floating,
        double energy_keV,
        double halfLife_s,
        double jpi
    );

    std::size_t numberOfGammas() const;

    bool isStable() const;

    bool hasKnownJPi() const;
};

class PhotonIsotope
{
public:

    PhotonIsotope();

    PhotonIsotope(
        int atomicNumber,
        int massNumber
    );

    int atomicNumber() const;

    int massNumber() const;

    const std::vector<PhotonLevel>& levels() const;

    std::vector<PhotonLevel>& levels();

    std::size_t numberOfLevels() const;

    const PhotonLevel& level(int id) const;

    PhotonLevel& level(int id);

    void addLevel(const PhotonLevel& level);

private:

    int fAtomicNumber;
    int fMassNumber;

    std::vector<PhotonLevel> fLevels;
};

class PhotonEvaporationReader
{
public:

    explicit PhotonEvaporationReader(
        const std::string& dataDirectory
    );

    PhotonIsotope read(
        int atomicNumber,
        int massNumber
    ) const;

    std::string filePath(
        int atomicNumber,
        int massNumber
    ) const;

    const std::string& dataDirectory() const;

private:

    std::string fDataDirectory;

    PhotonLevel parseLevel(
        const std::string& line
    ) const;

    PhotonTransition parseTransition(
        const std::string& line
    ) const;
};
```

### include/COINAlgebra/NuclearData/RadioactiveDecayReader.h

Source: [RadioactiveDecayReader.h](../include/COINAlgebra/NuclearData/RadioactiveDecayReader.h).

```cpp
struct RadioactiveDecayChannel
{
    std::string decayType;

    double daughterEnergy_keV;
    std::string daughterFloating;

    double branchingPercentage;

    double qValue_keV;

    std::string forbiddenness;

    RadioactiveDecayChannel();

    RadioactiveDecayChannel(
        const std::string& decayType,
        double daughterEnergy_keV,
        const std::string& daughterFloating,
        double branchingPercentage,
        double qValue_keV,
        const std::string& forbiddenness = ""
    );

    double modeFraction() const;
};

struct RadioactiveDecayMode
{
    std::string decayType;

    double totalBranchingFraction;

    std::vector<RadioactiveDecayChannel> channels;

    RadioactiveDecayMode();

    RadioactiveDecayMode(
        const std::string& decayType,
        double totalBranchingFraction
    );

    std::size_t numberOfChannels() const;

    void addChannel(
        const RadioactiveDecayChannel& channel
    );

    double channelBranchingFraction(
        std::size_t index
    ) const;
};

struct RadioactiveParentState
{
    double energy_keV;
    std::string floating;
    double halfLife_s;

    std::vector<RadioactiveDecayMode> decayModes;

    RadioactiveParentState();

    RadioactiveParentState(
        double energy_keV,
        const std::string& floating,
        double halfLife_s
    );

    std::size_t numberOfDecayModes() const;

    bool isStable() const;

    void addDecayMode(
        const RadioactiveDecayMode& mode
    );
};

class RadioactiveIsotope
{
public:

    RadioactiveIsotope();

    RadioactiveIsotope(
        int atomicNumber,
        int massNumber
    );

    int atomicNumber() const;

    int massNumber() const;

    const std::vector<RadioactiveParentState>&
    parentStates() const;

    std::vector<RadioactiveParentState>&
    parentStates();

    std::size_t numberOfParentStates() const;

    const RadioactiveParentState&
    parentState(
        double energy_keV
    ) const;

    RadioactiveParentState&
    parentState(
        double energy_keV
    );

    void addParentState(
        const RadioactiveParentState& state
    );

private:

    int fAtomicNumber;
    int fMassNumber;

    std::vector<RadioactiveParentState>
        fParentStates;
};

class RadioactiveDecayReader
{
public:

    explicit RadioactiveDecayReader(
        const std::string& dataDirectory
    );

    RadioactiveIsotope read(
        int atomicNumber,
        int massNumber
    ) const;

    std::string filePath(
        int atomicNumber,
        int massNumber
    ) const;

    const std::string&
    dataDirectory() const;

private:

    std::string fDataDirectory;

    RadioactiveParentState parseParentState(
        const std::string& line
    ) const;

    RadioactiveDecayMode parseDecayMode(
        const std::string& line
    ) const;

    RadioactiveDecayChannel parseDecayChannel(
        const std::string& line
    ) const;
};
```

### include/COINAlgebra/ROOT/COINAlgebra.h

Source: [COINAlgebra.h](../include/COINAlgebra/ROOT/COINAlgebra.h).

```cpp
class TRint;

class COINAlgebra
{
public:

    COINAlgebra();
    ~COINAlgebra();

    void PrintBanner() const;
    void PrintHelp() const;
    void PrintVersion() const;

    void ConfigureROOT(TRint& rootApp);

    void InitializeEnvironment(TRint& rootApp);

    const std::string& GetVersion() const;

private:

    std::string fVersion;
};

void COINAlgebraHelp();
void COINAlgebraVersion();
```

### include/COINAlgebra/ROOT/Commands.h

Source: [Commands.h](../include/COINAlgebra/ROOT/Commands.h).

```cpp
void COINAlgebraHelp();

void COINAlgebraVersion();
```

### gui/MainWindow.h

Source: [MainWindow.h](../gui/MainWindow.h).

```cpp
class QLineEdit;
class QPushButton;
class QComboBox;
class QListWidget;
class QListWidgetItem;
class QuiverView;
class QLabel;

class DecayQuiver;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow();

private slots:
    void addLevel();
    void addTransition();
    void onLevelRenamed(int index, const QString& newName);
    void onTransitionEdited(int index, double probability);
    void importQuiver();
    void exportQuiver();
    void editLevelItem(QListWidgetItem* item);
    void editTransitionItem(QListWidgetItem* item);
    void showPathVectorBuilder();
    void createDecayVector();
    void calculateFeeding();
    void calculateCoincidence();
    void showDecayTable();
    void showFeedingTable();

private:
    void removeLevel(int index);
    void editPopulations();
    void editGroups();
    void editSubquivers();
    void editViewSettings();
    void manageViews();
    void importEfficiencies();
    void showDetectionTable();
    void showEmissionTable();
    void showEfficiencyTable();
    void updateUI();
    void showVectorTable(const DecayVector& vector, const QString& title);
    DecayVector currentDecay() const;

    QuiverView* fView;
    QLineEdit* fQuiverTitle;

    QLineEdit* fLevelName;
    QLineEdit* fLevelEnergy;
    QPushButton* fAddLevel;
    QPushButton* fExportButton;

    QComboBox* fSourceBox;
    QComboBox* fTargetBox;
    QLineEdit* fProbability;
    QPushButton* fAddTransition;

    QListWidget* fLevelsList;
    QListWidget* fTransitionsList;

    std::unique_ptr<DecayQuiver> fQuiver;

    std::vector<DecayVector> fVectors;
    QJsonObject fMetadata;
    QLineEdit* fFeedingTransition;
    QLabel* fAnalysisResult;
    QComboBox* fCoincidenceA;
    QComboBox* fCoincidenceB;
    QPushButton* fCalculateCoincidence;
    QLabel* fCoincidenceResult;
    void invalidateCoincidence();
};
```

### gui/QuiverView.h

Source: [QuiverView.h](../gui/QuiverView.h).

```cpp
class DecayQuiver;
class DecayLevel;
class DecayTransition;

class QuiverView : public QGraphicsView
{
    Q_OBJECT
public:
    explicit QuiverView(QWidget* parent = nullptr);

    void setQuiver(DecayQuiver* q);
    void setMetadata(const QJsonObject& metadata);
    QJsonObject viewState() const;
    void restoreView(const QJsonObject& state);
    void setMode(int mode);
    void setEnergyScale(double scale);
    void configureView(const QJsonObject& state);
    int mode() const { return fMode; }

    void refresh();
    void focusOnLevel(int index);
    void showAllLevels();

private:
    QJsonObject fMetadata;
    int fMode=0;
    double fEnergyScale=1;
    std::vector<double> fLeft, fRight;
    std::vector<bool> fVisible, fEdgeVisible;
    std::vector<int> fCollapsed;
    std::vector<QPointF> fGroupCenters;
    int fDraggingGroup=-1;
    void setObjectStyle(int index, bool transition);
    void persistView();
    void wheelEvent(QWheelEvent* event) override;
    DecayQuiver* fQuiver = nullptr;
    const DecayLevel* fTopLevel = nullptr;
    const DecayTransition* fHighlightedTransition = nullptr;
    const DecayLevel* fHighlightedLevel = nullptr;
    int fVisibleLevelCount = 0;
    std::vector<QPointF> fPositions;
    double fNodeRadius = 28.0;
    double fLineStartX = 0.0;
    double fLineEndX = 0.0;
    double fLineHalfHeight = 6.0;

    std::vector<double> fTransitionOffsets;

    std::vector<char> fTransitionPinned;

    int fDraggingLevel = -1;
    int fDraggingTransition = -1;
    QPointF fLastMousePos;
signals:
    void levelDeleteRequested(int index);
    void appearanceChanged(const QJsonObject& metadata);
    void levelRenamed(int index, const QString& newName);
    void transitionEdited(int index, double probability);
protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
protected:
    void resizeEvent(QResizeEvent* event) override;
};
```

### gui/QuiverJson.h

Source: [QuiverJson.h](../gui/QuiverJson.h).

```cpp
namespace Studio {

struct Document {
    std::unique_ptr<DecayQuiver> quiver;
    std::vector<DecayVector> vectors;
    QJsonObject metadata;
    QString title;
};

Document readJson(const QByteArray& bytes);
DecayVector createDecayVector(const DecayQuiver& quiver, const QJsonObject& metadata);
}
```

### gui/StudioModel.h

Source: [StudioModel.h](../gui/StudioModel.h).

```cpp
namespace Studio {

DecayVector branchingVector(const DecayQuiver&, const QJsonObject&);
DecayVector transitionVector(const DecayQuiver&);
double levelEnergy(const DecayQuiver&, const QJsonObject&, int);
QJsonArray readEfficiencyCsv(const QByteArray&);
std::unordered_map<std::string,double> efficiencyMap(const DecayQuiver&, const QJsonObject&);
std::unordered_map<std::string,double> conversionMap(const DecayQuiver&, const QJsonObject&);
void validateStudio(const DecayQuiver&, const QJsonObject&);
}
```

### gui/ViewState.h

Source: [ViewState.h](../gui/ViewState.h).

```cpp
class DecayQuiver;

namespace Studio {

QJsonObject normalizeViewMetadata(QJsonObject metadata);
QJsonArray layoutGroups(const QJsonObject& metadata);
struct GraphSelection {
    std::vector<bool> levels;
    std::vector<bool> transitions;
    std::vector<bool> contextLevels;
    std::vector<bool> contextTransitions;
};
GraphSelection selectGraph(const DecayQuiver&, const QJsonObject& metadata, bool focus);
QJsonObject objectStyle(const QJsonObject& metadata, const QString& groupId,
                        const QString& objectId, bool transition);
void validateViewMetadata(const DecayQuiver&, const QJsonObject&);
void removeViewLevel(QJsonObject& metadata, int index);
void pruneViewTransitions(QJsonObject& metadata, const DecayQuiver&);

std::vector<double> energyCoordinates(const DecayQuiver&, const QJsonObject&,
                                      std::vector<bool>& visible);
}
```

### gui/TransitionEditor.h

Source: [TransitionEditor.h](../gui/TransitionEditor.h).

```cpp
class QWidget;
class DecayLevel;
class DecayTransition;

namespace Studio {

bool editTransition(QWidget* parent, DecayTransition& transition,
                    const std::vector<DecayLevel*>& levels);
}
```
