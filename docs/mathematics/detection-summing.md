# Detection maps and coincidence summing

The source is `docs/paper/APS_Coincidence_Algebra-14.pdf`, Section II.B,
Eqs. (28)-(33). Eq. (32) is the target probability vector. The implementation uses
an isotropic array of N identical detectors. Efficiencies are probabilities
for the whole array, conditional on gamma emission; they are not per-detector
efficiencies. Supply measured or simulated values for your setup, keyed by
transition name. No total-efficiency curve is built in.

For physical total transition coefficient t and gamma fraction q=1/(1+alpha):

    h = t q peak_efficiency
    a = t q total_efficiency / N
    o = t (1 - q total_efficiency / N)

`DetectionMaps::FullEnergyHit`, `TotalHit`, and `SummingOut` implement these
three edge maps. The total efficiency includes both full-energy absorption
and partial deposition, including Compton scattering. Thus a photon can sum
an observed peak out even when its own peak efficiency is zero.

Conversion belongs inside the complement in o. Multiplying the entire
no-hit probability by q would discard converted transitions that still feed
the next level. These maps model gamma interactions; conversion electrons,
atomic relaxation, inter-detector scattering correlations, angular correlations,
and timing windows require additional detector-response modelling.

All maps preserve stationary coefficients. Longer paths receive the product
of their edge factors. They do not renormalize mapped vectors. Apply the maps
to IC-inclusive physical coefficients, not to an already emission-weighted
vector, to avoid counting conversion twice.

## Usage

```cpp
#include "COINAlgebra/Detection/DetectionMaps.h"

// peakEfficiencies and totalEfficiencies: maps supplied for your array.
// alpha: coefficient map returned by BuildGammaQuiver.
DetectionMaps maps(peakEfficiencies, totalEfficiencies, detectorCount, alpha);
auto branch = projectors.BranchingProjector(decay);
auto transition = decay - branch;
auto hit = maps.FullEnergyHit(transition);
auto avoid = maps.SummingOut(transition);
auto summedPaths = maps.SummingInExpansion(algebra, transition);

auto corrected = pb.SummingFeedingVector(decay, maps);
double peakProbability = algebra.PathForm(corrected, DecayVector(observedPath));
```

Omitting alpha (or passing an empty map) explicitly assumes no conversion.
A nonempty alpha map must contain each used edge. Required efficiency entries
must exist; all efficiencies must be finite and satisfy 0 <= peak <= total
<= 1, alpha must be finite and nonnegative, and N must be positive.

`tests/integration/test_133Ba_gamma_quiver.C` defines the detector setup inside
its no-argument macro: `totalEfficiency = 0.7` for every transition and
`detectorCount = 64. Edit these constants for your setup. Each run prints the
emission/detection tables followed by connected summing contributions and
endpoint-combined corrected peaks. The uniform 0.7 is an illustrative input,
not a measured efficiency curve.
The macro retains its existing illustrative peak-efficiency values; replace
those too when changing the detector setup.

## Connected summing

`SummingInExpansion` constructs only positive powers:

    H = h + h^2/N + h^3/N^2 + ... .

It accepts single-edge terms, and leaves each connected path as a distinct
basis term. An isolated gamma's efficiency is not divided by N: the first hit
can occur anywhere in the array; each additional summed hit must be in that
same detector. The code currently uses N^(k-1) under this whole-array
efficiency convention. Eq. (31) as printed instead has N^(k+1); this is an
author-confirmed typo; the agreed correction is recorded as
[PA-001](../paper/ERRATA.md#pa-001--detector-count-exponent).
No stationary term belongs in H, the central factor after identity subtraction
in Eq. (32).

With O=1+o+o^2+... and E the sum of stationary paths at terminal levels:

    Gamma_path = V_target(b O) H V_source(O E).

This follows the intent of Eq. (32) with a terminal restriction on the lower
factor. The unrestricted V_source(O) printed there counts every partial
suffix as well as the completed cascade. For example, an observed upper
transition followed by a lower transition would acquire 1+o_lower instead of
o_lower. Restricting to terminal endpoints prevents that overcounting; this is
an explicit boundary-condition adjustment to the printed Eq. (32), still
awaiting review in [PA-002](../paper/ERRATA.md#pa-002--lower-propagation-boundary).
Terminal levels are quiver sinks, including multiple sinks.
The observation window is assumed to include the full cascade to these sinks.

`SummingFeedingVector` implements this formula. The resulting vector retains
individual path contributions. `PathForm` collects paths with equal source
and target, giving the direct peak plus connected cascades with the same
energy difference. Sum-only paths exist even where no direct edge is present.
This does not model finite energy resolution or group unrelated paths that
happen to have equal gamma-energy sums.

## Scope

These calculations use only the path algebra. Observed photons must form a
connected path; sums of non-connected transitions and gated spectra are outside
this implementation.

`SummingFeedingVector` requires an acyclic quiver, nonnegative finite initial
populations, single-edge transition terms, and total outgoing coefficients
summing to one at every nonterminal level. It uses the decay-vector
coefficients rather than stored transition probabilities. It enumerates
higher-order contributions, so output size can grow rapidly with cascade size.

## Verification

`tests/core/test_summing.C` checks losses above and below, IC in the complement,
Compton-only losses, zero efficiencies, N=1, multiple detectors, and rejected
inputs. An independent reference enumerates complete physical cascades and all
nonempty subsets of observed photons, comparing connected-path results on a branching scheme with two initial populations
and two terminal levels.
