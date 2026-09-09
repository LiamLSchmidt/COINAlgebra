# Internal conversion, gamma emission, and detection

The total internal conversion coefficient is alpha = I_conversion / I_gamma.
A transition emits a gamma with probability 1 / (1 + alpha), conditional on
that transition occurring. Both channels reach the same daughter level.

The PhotonEvaporation intensity column is a relative **gamma** intensity.
Therefore the physical branch from level i to j is

    t_ij = I_ij (1 + alpha_ij) / sum_k I_ik (1 + alpha_ik).

`DecayQuiverBuilder::BuildGammaQuiver` uses these total branches, including
when called through the original overload. This corrects the earlier
normalization using gamma intensities alone. See the bundled
`PhotonEvaporation5.5/README*`, field 6, and
[Geant4's reader](https://github.com/Geant4/geant4/blob/master/source/processes/hadronic/models/de_excitation/management/src/G4LevelReader.cc).

For a decay vector d = b + t, `FeedingVector(d)` computes

    F = V_target(b (1 + t + t^2 + ...)) t.

The expansion uses total physical branches. Do not apply gamma-emission or
detector factors inside this expansion: earlier transitions can convert or
go undetected and still feed the observed transition. For a single edge g,

    emission(g) = F(g) / (1 + alpha_g)
    detection(g) = F(g) * efficiency_g / (1 + alpha_g).

Example using reader data:

```cpp
std::unordered_map<std::string, double> alpha;
auto quiver = DecayQuiverBuilder::BuildGammaQuiver(parent, daughter, alpha, "EC");
// Construct decay from the initial level populations and quiver edge probabilities.
PathAlgebra algebra(quiver);
PathProjectors projectors;
DecayProbability pb(algebra, projectors);
auto feeding = pb.FeedingVector(decay);
auto emission = pb.EmissionFeedingVector(decay, alpha);
auto detection = pb.DetectionFeedingVector(decay, efficiencies, alpha);
// Equivalent when feeding is already available:
auto detected = feeding.ApplyConversionMap(alpha).ApplyDetectionMap(efficiencies);
```

The builder replaces the output alpha map on success; keys are generated
transition names. Keep this map alongside the quiver: the current quiver JSON
export does not store it. The reader already retains total and shell-specific
conversion data; these calculations use the total coefficient.

`ApplyConversionMap` leaves stationary terms unchanged and multiplies the
1 / (1 + alpha) factors for every edge of longer paths. Required map entries
must be present, finite, and nonnegative. Zero means no conversion. The map
does not renormalize the resulting vector. Detection efficiencies must lie
in [0, 1]. The two-argument `DetectionFeedingVector(decay, efficiencies)`
continues to apply only the supplied efficiency map; use the three-argument
overload for gamma detection including conversion.

`FeedingProbability`, `PathConnection`, and `CoincidenceProbability` retain
their physical transition meanings. For two distinct observed single edges,
gamma emission/detection factors belong on those two observed edges; any
unobserved connecting transitions still use total branches. Atomic relaxation
X rays and Auger electrons are not calculated by the gamma-emission map.

For summing-corrected observations, see [detection and summing](detection-summing.md).
There, unobserved propagation uses the no-hit map with IC inside its complement.
