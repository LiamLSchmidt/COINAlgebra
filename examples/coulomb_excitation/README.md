# Cadmium and zirconium Coulomb-excitation gamma quivers

Use **Import Quiver** in DecayQuiverStudio to open any of the twelve JSON files
here. These describe gamma de-excitation in the named nucleus itself, using
`PhotonEvaporationReader` and the bundled `PhotonEvaporation5.5` data. There is
no radioactive-decay reader, beta feeding, or electron-capture feeding.

## Selected upper levels

Each window includes **all tabulated absolute-energy levels at or below the
selected upper level**, including that level and the ground state. It is not
restricted to descendants of a single initially populated state. This permits
analysis of cascades from multiple states populated in Coulomb excitation.

| Isotope | Upper energy (keV) | Upper spin/parity in bundled data | Levels | Gamma edges | Excited gamma terminals |
| --- | ---: | --- | ---: | ---: | ---: |
| 106Cd | 2973.330 | 2+ | 34 | 53 | 0 |
| 108Cd | 2993.110 | 2+ | 40 | 108 | 2 |
| 110Cd | 2984.460 | 2+ | 54 | 108 | 14 |
| 112Cd | 2980.850 | 2+ | 70 | 178 | 5 |
| 114Cd | 2941.270 | 2+ | 67 | 192 | 13 |
| 116Cd | 2782.600 | 2+ | 54 | 116 | 2 |
| 90Zr | 3308.800 | 2+ | 8 | 14 | 1 |
| 92Zr | 2398.360 | 4+ | 9 | 15 | 1 |
| 94Zr | 2366.120 | 2+ | 9 | 14 | 0 |
| 96Zr | 2857.373 | 4+ | 11 | 16 | 2 |
| 98Zr | 2276.900 | 4+ | 12 | 24 | 2 |
| 100Zr | 1414.620 | 4+ | 11 | 22 | 0 |

The cadmium choice is the highest tabulated 2+ or 4+ level below 3 MeV
for each isotope. This is a deliberately broad analysis window: a
[110Cd experimental report](https://indico.in2p3.fr/event/32956/contributions/143171/)
describes population of 20 states up to 3 MeV, including the 1731-keV 0+ state.
The other cadmium windows are an analogous selection, not a claim that all
their levels were observed or can be appreciably populated. The relevance of
multi-step quadrupole excitation to 106Cd is also demonstrated in
[this experimental study](https://arxiv.org/abs/2209.06298).

Zirconium windows are chosen individually around low-lying quadrupole states:
the second tabulated 2+ in 90Zr; the 2398-keV 4+ in 92Zr; the fourth tabulated
2+ in 94Zr; the 2857-keV 4+ with tabulated gamma decays in 96Zr; and the
2277- and 1415-keV 4+ states in 98Zr and 100Zr. These choices cover several
low-lying structures without carrying the neutron-rich nuclei to an arbitrary
common high-energy limit. The 94Zr choice is motivated by an
[experimental report of excitation up to the fourth 2+ state](https://inspirehep.net/files/042a5e7a74a553150b7cb4692dcdecee).
[98Zr Coulomb-excitation measurements](https://www.anl.gov/argonne-scientific-publications/pub/145213)
provide context for studying its low-lying transitions and shape coexistence.
Exact energies and spin values here come from the bundled dataset, not those
papers. Upper-level choices are editable analysis defaults, not optimized
experimental predictions: beam, target, geometry and electromagnetic matrix
elements have not been specified.

## Branches and limitations

The construction follows the level/edge structure and normalization convention
of `DecayQuiverBuilder`, replacing radioactive feeding/reachability with the
energy-window selection above. Every tabulated downward, positive-intensity,
non-E0 gamma branch from a selected level is retained. Unknown multipolarities
are retained when the dataset gives a positive gamma intensity. Level IDs in
names prevent collisions between equal or nearly equal excitation energies;
edge names also include the tabulated gamma energy in keV.

Each nonterminal level has outgoing probabilities summing to one, calculated
as relative gamma intensity divided by the outgoing sum. These are conditional
gamma branching probabilities, **not Coulomb-excitation population probabilities
or absolute photon yields**. No excitation population or preselected vector is
assigned. No gamma branch is dropped at the energy boundary: all retained
positive gamma branches were checked to end at an included, lower level.

E0 transitions are not single-photon gamma transitions and are omitted.
Zero-intensity branches are also omitted, without replacing unknown strengths.
Internal-conversion competition is not applied. In particular an E0-only
excited 0+ state can be a terminal in this gamma graph, even though it is not
physically stable. Other excited terminals have no usable outgoing branches
in the bundled data; paths ending there are incomplete physical cascades.
The table counts these levels, and `metadata.excited_gamma_terminal_levels`
identifies them individually. Do not interpret terminal status as a measured
lifetime or assume every cascade in these files reaches the ground state.

Floating levels (`+X`, `+Y`, etc.) are excluded because their energies are not
absolute excitation energies; their IDs are recorded in metadata. All other
levels below the cap are retained regardless of spin, including levels that
may not be significantly excited experimentally. There are no timing cuts,
detector efficiencies, angular correlations, conversion electrons, or atomic
relaxation products.

Metadata records the upper level, energies, raw spin/parity encoding,
half-lives, gamma intensities, multipolarity codes, mixing ratios and conversion
coefficients. Studio imports only the standard quiver fields; it does not use
this extra metadata for analysis and may omit it on re-export. A zero tabulated
half-life must not be interpreted as a measured zero lifetime.

## Reproduce

From the repository root, with a C++17 compiler and Qt5Core development files:

```sh
g++ -std=c++17 -fPIC -Iinclude -Igui \
  tools/generate_coulex_quivers.cpp gui/QuiverJson.cpp \
  src/Core/*.cxx src/NuclearData/PhotonEvaporationReader.cxx \
  $(pkg-config --cflags --libs Qt5Core) -o /tmp/generate_coulex_quivers
/tmp/generate_coulex_quivers
```

Edit the `selections` array in the generator to change the isotope-specific
upper energies. The current validation requires each cap to match an actual
2+ or 4+ level in the photon data. Every document is validated through the
actual `Studio::readJson` importer, its counts are checked, and outgoing
probability sums must agree with one within 1e-12 before it is written.

## Studio analysis update

The new right-hand analysis panel now preserves metadata on import/export and
uses source feeding or the recorded upper level to create decay vectors. This
supersedes the earlier statements above about metadata not being used or being
omitted on export. See [Studio analysis](../../docs/studio-analysis.md) for the
coefficient convention, feeding calculations and table controls. Gamma-edge
probabilities in these JSON files have not changed.
