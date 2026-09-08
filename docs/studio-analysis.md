# Studio analysis panel

The right sidebar contains **Paths & Vectors**, **Create Decay Vector**,
**Print Decay Vector Table**, a transition-name input, **Calculate Feeding
Probability**, and **Print Feeding Table**. Clicking a transition in the left
list fills the input; an exact transition name can also be typed or pasted.

Create Decay Vector stores a snapshot in the workspace's vector collection
and includes it in Export Quiver. Its terms are:

- Each single-transition path with coefficient equal to its current gamma
  branch probability.
- For imported source files, stationary daughter paths with coefficients
  obtained by summing `raw_branching_percentage / 100` over feeding channels.
  The bundled channel intensities are absolute parent percentages (for
  example, 22Na's beta-plus channels sum to 90.382%, not 100%). Therefore the
  mode fraction is not applied a second time. Daughter energies are matched
  within the recorded tolerance, with the daughter isotope prefix respected.
- For imported energy-window schemes, the `upper_level` stationary path with
  coefficient one. For a manually created quiver without metadata, the last
  added level is used as the top level.

Zoom is display-only. These operations always use the full underlying quiver.
The table and feeding buttons build from the current quiver on each click;
they do not use an older saved vector snapshot. After editing a scheme, use
Create Decay Vector again if a new snapshot is wanted for export.

Calculate Feeding Probability calls `DecayProbability::FeedingProbability`.
Print Feeding Table calls `FeedingVector`, then `DecayVector::PrintTable`.
The table dialogs provide a read-only, selectable monospace table and **Save
Table…** for a text file; the same table is printed to standard output. The
existing PrintTable column is labelled Probability: in the decay-vector table
it holds term coefficients, and in the feeding-vector table it holds propagated
coefficients. Calculations retain the existing PathForm semantics, including
its aggregation of paths with identical endpoints.

Stationary-plus-single-edge vectors on acyclic quivers use a topological
population-flow implementation inside DecayProbability, mathematically
equivalent to its finite path expansion. General vector calculations retain
the existing expansion fallback. Studio rejects cyclic schemes for feeding
calculations. No detector efficiency, internal-conversion or competing-particle
correction is introduced by the panel.

Import/export now preserves metadata. If an imported source feeding level or
the recorded upper level is renamed/removed, the panel reports a matching
error rather than inventing an initial population; reimport the original file
to restore the association. Older exports that omitted metadata cannot recover
their original beta/EC populations: use the original files in examples/sources.
