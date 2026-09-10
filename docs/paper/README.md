# Mathematical source

The current manuscript is `APS_Coincidence_Algebra-14.pdf`.

Track confirmed typos and unresolved paper–code differences in
[the discrepancy register](ERRATA.md). A computational audit of the implemented products and detection maps is
recorded in [the validation baseline](../mathematics/coincidence-validation.md).
This does not certify every bundle statement or the general gated formula.

`DecayCoin` implements Definitions III.1-4: unordered input is canonicalized
from highest to lowest, overlapping intervals are rejected, gaps and shared
endpoints are allowed, and stationary endpoint factors reduce by Eq. (37).
Transition identity follows `DecayPath` (name and endpoint pointers).

`CAlgebra` implements the scalar connection and bilinear fiber product in
Definitions III.5-6. Its `Vector` is a list of coefficient/basis terms. The
connection sums paths using coefficients in the supplied decay vector, excluding
stationary populations. A geometrically valid coincidence can have zero weight.
Multiplication remains ordered, and there is no global identity or zeroth power.

The API requires an explicit highest-to-lowest order containing all quiver
levels. This generalizes the manuscript's fully connected ordering to sparse
quivers without inferring physical order from names or outgoing edge counts.
All quiver transitions must descend in this order. Levels must remain alive and
unchanged. The decay vector and transition definitions are copied at construction.

Eq. (40) prints the source of the final factor as the target. The implementation
uses its target, consistent with the existing path embedding and Eq. (41).
Regression coverage is in `tests/core/test_coin.C` (native and ROOT).

For interpreted ROOT macros, include `COINAlgebra/Core/DecayQuiver.h` before
the algebra headers, following the dependency-first order in the ROOT tests.
This avoids recursive dictionary parsing of partially included headers.

Example:

```cpp
CAlgebra algebra(quiver, {highest, middle, lowest}, decay);
auto first = algebra.Coin(std::vector<DecayTransition>{*upperTransition});
auto second = algebra.Coin(std::vector<DecayTransition>{*lowerTransition});
auto product = algebra.Multiply(first, second); // coefficient-weighted terms
```

Detection maps and summing calculations are implemented in `DetectionMaps` and
`DecayProbability`; see [the definitions and manuscript clarifications](../mathematics/detection-summing.md)
for the path-algebra implementation of Eqs. (28)-(33), IC handling, and detector
setup inputs. Eq. (32) is the target; the documentation records the current
normalization and terminal-boundary adjustments to the printed equations.
