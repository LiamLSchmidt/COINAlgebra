# DQStudio groups, subquivers and saved views

The physical `DecayQuiver` remains the authority for levels and transitions.
Studio keeps graph membership separate from view settings; every view references
that same quiver. Branching populations, transition probabilities and detector
calibration are document data and do not change when switching views.

## Try it

Import `examples/dqstudio-bands-demo.json`. It is a **synthetic illustration**,
not evaluated nuclear data. Open **Saved views…** and restore the band overview,
selected branch with context, or collapsed-band view.

- **Quiver View** retains the ordinary level scheme.
- **Band View** uses shorter levels in columns. Drag a band heading horizontally
  to move all its levels. In **Groups…**, edit the band's centre and level width.
  Right-click a heading or summary box to collapse/expand it.
- **Focus View** displays a group, an explicit subquiver, or a level with selected
  context. Choose the target in **Focus / energy / style…**.

## Groups and explicit subquivers

**Groups…** manages semantic collections of levels: bands, cascades and user
categories. A group's default focus includes all transitions between its levels.
Its colour and line style apply to member levels and intra-group transitions;
individual styles override those defaults. For overlapping groups, the first
membership determines layout and inherited style. Unassigned transitions retain
the global style.

**Subquivers…** manages explicit selections of vertices **and arrows**. Selecting
an arrow also selects its endpoints. Other arrows between the same endpoints
remain excluded. The “Select all arrows…” button explicitly builds an induced
selection when wanted. Saving validates that every selected arrow has both
endpoints selected.

In **Focus / energy / style…**, context choices include incoming, outgoing,
both directions, all ancestors, or all descendants. Incoming/outgoing/both use
the chosen neighbour depth. Added context is faded; the original selection stays
prominent. An explicitly excluded internal arrow stays excluded even when context
is enabled. Focus is a visual restriction: analysis still uses the full quiver.

Collapsed groups become summary boxes. External connections are retained and
aggregated by their displayed endpoints; hover an aggregate line to see its
original transition names. Labels count transitions, **not probabilities**.
Internal transitions remain in the physical quiver and reappear on expansion.
Hidden groups remove their displayed levels and incident arrows from the view.

## Energy, style and camera

Energy spacing is independent of wheel zoom:

- **auto:** physical spacing when selected levels have known energies; otherwise
  insertion order, retaining manual vertical placement for uncalibrated levels.
- **physical:** spacing proportional to energy.
- **compressed:** `log(1 + E/E₀)`, with editable positive `E₀` in keV.
- **uniform:** even spacing in energy order when energies are known, otherwise
  insertion order.

An optional energy window filters levels and scales the selected interval.
Explicit physical/compressed modes and energy windows require known energies;
the settings dialog reports missing data instead of applying an invalid change.
Enter energy in keV when adding a level, or use **Initial populations / energies…**
to edit it later. Energies live in the core `DecayLevel` objects. Older
`level_…keV` names and energy metadata are migrated once during import; renaming
a level subsequently leaves its physical energy unchanged. An empty window displays a message.
`Y ×` expands vertical spacing; wheel zoom changes the camera scale.
Drag empty canvas to pan; **Fit** recovers the displayed scheme.

Style precedence is **global → group → object override**. Global colour and
transition label visibility are in the view settings dialog. Groups supply
colour, stroke width and solid/dashed/dotted lines. Right-click a level or
transition and choose **Style…** for independent overrides; “Inherit”/blank
fields remove the corresponding overrides.

## Saved views

**Saved views…** saves the current configuration under a name. Saving an existing
name replaces that view. Restore/delete operations apply to the selected entry.
Views contain layout mode, focus/context, energy transform/window, group positions
and widths, visibility/collapse, inherited and individual styles, camera centre
and scale, object highlights, and manual level/transition offsets. Camera restore
is subject to normal viewport/scrollbar pixel rounding. Calibrated energy spacing
takes precedence over manual vertical positions.

Group/subquiver definitions are shared across saved views. Editing membership
therefore affects every view referencing that definition. Removing a definition
clears references to it from all views. Removing levels remaps index references;
removing transitions or changing their endpoints prunes invalid explicit arrow
memberships and saved styles/offsets. Earlier Studio group metadata is migrated
on import. Existing source JSON without view data remains supported.

## Populations and efficiencies

**Initial populations / energies…** edits initial level fractions, which must sum
to one. They are stationary terms of the branching vector. The transition vector
uses the quiver's editable transition coefficients, and their sum is the decay
vector. Separate tables are available in the right panel. Populated levels must
have their population redistributed before deletion.

**Import Efficiency CSV…** accepts the GRIFFIN file in `data/GRIFFIN_Eff.csv`
with its `Energy[keV], HPGe` header, as well as the generic format below. GRIFFIN
values are used directly as efficiency fractions, not percentages.

For each transition, Studio evaluates the curve at `|Esource − Etarget|` in keV.
Exact tabulated energies use the corresponding sample; intermediate energies
are linearly interpolated. **Show Transition Efficiencies** lists the transition
name, derived energy and assigned efficiency. The map is rebuilt from the current
level energies for every calculation, so editing an energy updates detection.
The supplied GRIFFIN table covers 10–2000 keV; out-of-range transitions are reported
by name and energy.

Generic CSV format:

```csv
energy_keV,efficiency
0,0.20
1000,0.10
3000,0.05
```

Supply at least two strictly increasing energies and fractions in `[0,1]`.
Blank lines and `#` comments are allowed. Linear interpolation is used with no
extrapolation. Missing energies or incomplete transition-energy coverage reject
the import without replacing the prior curve.

**Show Gamma Emission Probabilities** applies `1/(1+α)` to physical feeding
using `metadata.conversion_coefficients`, keyed by transition name.
**Show Detection Probabilities** then multiplies by the efficiency at the
transition energy. Physical transition branches must include gamma + IC; they
are not renormalized or efficiency-weighted during propagation. No summing
correction is added by these controls. If no IC map is supplied, α=0 is assumed
and the table title says so. A supplied map must contain every transition with
a finite nonnegative coefficient. **Show Transition Efficiencies** also displays α.
Saved views share these physical inputs.

The `examples/133Ba_gamma_quiver.json` example includes all nine coefficients
from `PhotonEvaporation5.5/z55.a133`, and explicitly sets initial populations to
0.145 at 383.8491 keV and 0.855 at 437.0113 keV. Its integration-test generator
preserves these inputs on every export.

## Storage contract

`metadata.groups` stores stable group IDs, names, semantic kinds, and level
indices. `metadata.subquivers` stores stable IDs, names, level indices, and
explicit transition names. `metadata.view` is the active view state;
`metadata.saved_views` contains `{name, state}` entries.

Each state stores `group_layout` keyed by stable group ID, `focus` keyed by
selection type/ID (or a level index), `energy`, global/per-object styles, camera,
highlights and manual offsets. Visual data never enters core `DecayLevel` or
`DecayTransition` classes. Membership uses document indices/unique transition
names; Studio remaps or prunes them on deletion. No public core subquiver API or
algebraic restriction operator is introduced by these visualization changes.

Physical populations and calibration remain in `metadata.branching` and
`metadata.efficiency_samples`. Core level energies are exported in the top-level
`level_energies_keV` array aligned with `levels`; `null` means unknown, while
`0` is a known ground-state energy. Both native `DecayQuiver::ExportJson` and
Studio use this format. Old files without the array continue to load; legacy
energy metadata is migrated and removed. An explicit energy array takes
precedence over legacy metadata and display names. JSON `analysis_vectors` contains named branching,
transition and combined decay snapshots. Calculations rebuild from authoritative
physical inputs rather than trusting snapshots. Custom path vectors rebind to
current transitions after edits; invalid paths are discarded.


## Core energy API

```cpp
DecayQuiver quiver;
auto* ground = quiver.AddLevel("ground", 0.0);       // keV
auto* excited = quiver.AddLevel("excited", 1332.5);
auto* unknown = quiver.AddLevel("unmeasured");

excited->SetEnergy(1332.492);
const double gammaEnergy = excited->GetEnergy() - ground->GetEnergy();
if (unknown->HasEnergy()) { /* safe to call GetEnergy() */ }
unknown->ClearEnergy();
```

`DecayLevel(name, energy_keV)` is also available. Energies must be finite and
nonnegative. `GetEnergy()` throws `std::logic_error` when unset; invalid setters
throw `std::invalid_argument` without changing the previous energy. Unknown is
never silently treated as zero. Names, path identity and probabilities retain
their existing semantics. Nuclear-data builders assign the reader's full numeric
energy, rather than recovering it from rounded level names.


## Two-transition coincidence

The right panel's **Two-transition coincidence** section selects two distinct
transitions A and B. **Calculate Coincidence Probability** evaluates
`DecayProbability::CoincidenceProbability` in both orders and adds the disjoint
results for the acyclic decay graph. Reversing the selections gives the same
joint probability. Mutually exclusive branches give zero; selecting the same
transition twice disables calculation. All calculations use the full quiver,
regardless of the displayed focus or hidden groups.

The panel shows:

- **Physical:** both selected transitions occur, counting gamma + IC.
- **Gamma emission:** physical coincidence divided by `(1+αA)(1+αB)`.
- **Detected:** gamma coincidence multiplied by `ε(EA) ε(EB)`, when a CSV is
  loaded. This assumes independent detection efficiencies and does not include
  angular correlations, detector-pair geometry or summing corrections.

When IC data is absent, α=0 is explicitly indicated. Parallel arrows are resolved
to their individual branching fractions because the core path form groups paths
with the same endpoints. Selections survive ordinary edits where possible;
results are cleared when inputs change, preventing stale values from being shown.
