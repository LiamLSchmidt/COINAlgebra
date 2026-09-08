# Common source gamma quivers

Open any `*_gamma_quiver.json` file with **Import Quiver** in DecayQuiverStudio.
The files use the same `levels`, `transitions`, and `vectors` schema as Studio
exports. No vectors are preselected; construct paths and vectors in Studio.

| Source | Daughter nuclei included | Levels | Gamma transitions |
| --- | --- | ---: | ---: |
| 22Na | 22Ne | 2 | 1 |
| 54Mn | 54Cr, 54Fe | 3 | 1 |
| 56Co | 56Fe | 16 | 47 |
| 57Co | 57Fe | 5 | 10 |
| 60Co | 60Ni | 4 | 6 |
| 65Zn | 65Cu | 3 | 3 |
| 88Y | 88Sr | 5 | 7 |
| 137Cs | 137Ba | 3 | 2 |
| 152Eu | 152Sm, 152Gd | 42 | 181 |
| 154Eu | 154Sm, 154Gd | 29 | 137 |

## Construction and interpretation

Generated from the repository's `RadioactiveDecay5.5` and
`PhotonEvaporation5.5` datasets using their C++ readers and
`DecayQuiverBuilder::BuildGammaQuiver`, following the 22Mg and 133Ba integration
examples. Only the parent ground state is used. All populated beta-minus,
beta-plus, and K/L/M-shell electron-capture modes with positive total branching
fraction are included. Each mode is passed to the builder with its default
1 keV energy-matching tolerance. All matches succeeded.

The output is the union of the resulting gamma quivers. Shared transitions
from different feeding modes are included once, with their unchanged builder
probabilities. Daughter isotope prefixes prevent collisions between different
nuclei. The weak 54Mn beta-minus branch populates the 54Fe ground state, which
appears as an isolated level. Daughter ground states populated directly remain
in the files even when they produce no gamma cascade.

Probabilities are **conditional outgoing gamma branch probabilities**, computed
by the existing builder as positive relative intensity divided by its outgoing
sum. They are not absolute photon intensities per parent decay. Beta/EC feeding
is not represented by edges or an initial population vector. Metadata retains
the reader's mode fractions and raw channel percentages for provenance; these
are not applied to the gamma probabilities and are not imported as Studio
analysis state. Studio may omit this extra metadata on re-export.

These files inherit the builder's treatment of internal conversion and level
matching: conversion coefficients are not used to calculate photon emission
probabilities, and reachability follows the photon dataset's transitions.
They include no atomic X-rays, Auger electrons, positron annihilation photons,
timing cuts, detector response, or subsequent radioactive daughter decays.
In particular, the 137Cs file includes the 137Ba isomer's de-excitation without
a timing cut. They describe the bundled data, not a new evaluation of it.

## Regenerate and validate

From the repository root, with a C++17 compiler and Qt5Core development files:

```sh
g++ -std=c++17 -fPIC -Iinclude -Igui \
  tools/generate_source_quivers.cpp gui/QuiverJson.cpp \
  src/Core/*.cxx src/Builders/DecayQuiverBuilder.cxx src/NuclearData/*.cxx \
  $(pkg-config --cflags --libs Qt5Core) -o /tmp/generate_source_quivers
/tmp/generate_source_quivers
```

The generator checks consistency when merging shared transitions, imports
every generated document through `Studio::readJson`, and verifies outgoing
normalization to 1e-12 before writing it. It fails on unmatched feeding levels
or empty gamma quivers. Output filenames and ordering are deterministic.

## Studio analysis update

The new right-hand analysis panel now preserves metadata on import/export and
uses source feeding or the recorded upper level to create decay vectors. This
supersedes the earlier statements above about metadata not being used or being
omitted on export. See [Studio analysis](../../docs/studio-analysis.md) for the
coefficient convention, feeding calculations and table controls. Gamma-edge
probabilities in these JSON files have not changed.
