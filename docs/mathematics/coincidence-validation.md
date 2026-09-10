# Coincidence calculations and validation baseline

Audit date: 2026-09-09. Manuscript: `APS_Coincidence_Algebra-14.pdf`.
Code baseline: `3135c8757999b13288ced0978f8907a1fa87092c` plus the changes
accompanying this document. This is a computational audit of the implemented
products and detector model, not a proof of all bundle statements or a comparison
with experiment/Geant4. The manuscript itself has not been changed.

## Supported mathematics

`CAlgebra` implements the ordered product of Eqs. (41)-(45). Its connection is

    c_d(x,y) = sum over paths t(x) -> s(y) of product of tau edge coefficients.

The zero-length connection is 1. Initial populations do not enter this scalar;
they enter separately through `Embed(b)`. Thus `b *_d tau^k` gives each ordered
k-transition coincidence once, retaining initial-level markers where the
stationary quotient permits. Sum coefficients over initial markers for a given
transition set. Do not divide by k! or count the reversed order again in this
strictly descending algebra. Summing all pair coefficients gives an expected
number of pairs per decay, not necessarily a probability bounded by one.

The existing tests check associativity including stationary and disconnected
factors. The new tests independently enumerate completed physical cascades and
all observed subsets, comparing every coefficient of Eq. (61) and Eq. (70).
They cover parallel edges, two sinks, mixed initial populations, IC, zero/perfect
peak efficiencies and N=1,2,4. Exact detector assignment enumeration separately
checks the two-detector avoidance argument below.

The path/coincidence correspondence is precise for **composable** paths: their
connection is 1 and concatenation agrees with tensor composition. Embedding is
linear, but is NOT a global algebra homomorphism: two separated paths have zero
path-algebra product and may have nonzero coincidence product. `PathForm` uses
endpoint equivalence, whereas coincidence basis equality retains transition
identity. Parallel arrows must not be inadvertently merged when selecting a
specific transition.

## Detection maps and the chosen fiber

For physical gamma-plus-IC branches x_e, gamma fraction q_e=1/(1+alpha_e),
whole-array conditional peak efficiency p_e and total efficiency t_e:

    h_e = x_e q_e p_e
    o_e(m) = x_e (1 - m q_e t_e/N)

`FullEnergyHit` supplies h. `SummingOut` supplies o(1).
`AvoidDetectors(tau,m)` supplies o(m), requiring 0 <= m <= N.
These are path-algebra diagonal maps, extended multiplicatively along paths.
Stationary paths are unchanged; branches are not renormalized.

`FullEnergyHit`, `TotalHit`, and `SummingOut` also accept `CAlgebra::Vector`.
Those overloads act only on the explicitly observed factors and preserve
stationary markers. They commute with path embedding. They do **not** replace
the base vector stored in a CAlgebra. In a fixed fiber they respect its product;
transport to a different detection-mapped fiber generally does not preserve
products, because hidden connecting transitions acquire different weights.

For independent detection without summing corrections:

```cpp
CAlgebra physical(quiver, order, b + tau);
auto pairs = physical.Multiply(physical.Embed(b),
                              physical.Power(physical.Embed(tau), 2));
auto detected = maps.FullEnergyHit(pairs);
```

For Eq. (70), including sums between disconnected transitions:

```cpp
CAlgebra out(quiver, order, maps.SummingOut(tau));
auto H = maps.SummingInExpansion(out, tau); // original physical tau
// H = h + h *_out h/N + h *_out h *_out h/N^2 + ...
auto spectrum = out.Multiply(out.Multiply(out.Embed(b), H), out.Embed(E));
```

E is the sum of stationary paths at all physical terminal levels (one ground
state in the paper). The same out fiber supplies avoidance before, between and
after observed factors. Each coefficient is the expected number of the specified
single-detector peak events per decay. Different detectors can contribute
multiple counts in one decay. This is not a normalized exclusive event vector.
The path-algebra expansion contains only connected observed paths; the
coincidence expansion additionally retains disconnected sums.

## Two distinct clean peaks

For two individually resolved transitions with no summed-in photons in either
peak, all other emissions must avoid **both** selected detectors:

```cpp
// N >= 2; for N == 1 this observable is identically zero.
CAlgebra twoOut(quiver, order, maps.AvoidDetectors(tau, 2));
auto h = twoOut.Embed(maps.FullEnergyHit(tau));
auto cleanPairs = twoOut.Multiply(
    twoOut.Multiply(twoOut.Embed(b), twoOut.Power(h, 2)), twoOut.Embed(E));
for (auto& term : cleanPairs)
    term.coefficient *= double(N - 1) / double(N);
```

The prefactor selects distinct detectors. The connection uses o(2), not o(1).
This agrees with complete-cascade enumeration in the identical independent
isotropic detector model. It is a deliberately restricted observable, not an
implementation of general Eq. (72) with summed-in groups and energy gates.
Such groups need detector/group labels: a single tensor of photons does not
record which photons summed into which peak. See CA-002 in the discrepancy
register before using Eq. (72) as a general simulation prediction.

## Reproducible sample

Run from the repository root:

```sh
COINALGEBRA_HOME="$PWD" bin/coinalgebra -l -b -q examples/calculations/coincidence.C
ctest --test-dir build --output-on-failure
```

The example uses A(600 keV)->B(400 keV) with probability 1, then B->C(100 keV)
with probability .75 or B->G(0 keV) with .25, and C->G with probability 1.
Population is entirely at A. N=4. Input responses, deliberately synthetic:

| Edge | Energy (keV) | Peak | Total | alpha |
|---|---:|---:|---:|---:|
| AB | 200 | .4 | .6 | 0 |
| BC | 300 | .3 | .5 | 1 |
| BG | 400 | .35 | .55 | 0 |
| CG | 100 | .5 | .7 | .25 |

For AB and CG, whose gap contains BC:

| Observable | Calculation | Per decay |
|---|---|---:|
| Physical coincidence | .75 | .75 |
| Independent full-energy detections | .75*.4*(.5/1.25) | .12 |
| Two clean distinct-detector peaks | .12*(3/4)*(1-2*.5/(2*4)) | .07875 |
| Same-detector AB+CG sum, BC avoids it | .12/4*(1-.5/(2*4)) | .028125 |

The executable prints all individual basis coefficients, including singles and
three-photon sums, so future simulation comparisons can use exactly specified
observables. Efficiencies alone do not define summing-out: measured total
response, detector count, and the detector grouping (crystals vs addback) must
also match the simulation. No angular correlations, interdetector scattering,
auxiliary conversion radiation, time windows or addback model is included.

## Remaining manuscript work

The Eq. (40) target discrepancy and Eq. (32) terminal boundary remain recorded;
Eq. (70)'s explicit ground-state factor supports the implemented terminal
convention. Bundle isomorphisms in Eqs. (49)-(58), arbitrary signed fibers, and
general gated group detection have not been certified by this audit. Support
alone cannot ensure positive nonzero path connections for arbitrary signed
coefficients because different paths can cancel. The physical tests use
nonnegative normalized branches. CAlgebra intentionally also allows general
scalar coefficients; it does not turn arbitrary vectors into probabilities.
