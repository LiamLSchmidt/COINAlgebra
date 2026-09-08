# Ten complementary nuclear-structure gamma quivers

Open the `*_structure_gamma_quiver.json` files with **Import Quiver** in
DecayQuiverStudio. This set extends the source-decay and Cd/Zr collections
with ten different nuclei. All files use the bundled PhotonEvaporation5.5
data through the C++ reader, with the same level/edge construction and
conditional intensity normalization convention used by the previous quivers.

## Coverage

| Nucleus | Maximum level (keV; spin/parity) | Levels | Gamma edges | Structure or decay feature |
| --- | --- | ---: | ---: | --- |
| 12C | 7654.200; 0+ | 3 | 2 | Hoyle-state radiative cascade through the first 2+ state |
| 16O | 7116.850; 1− | 5 | 7 | E1, E2 and E3 branches, plus an E0-only excited endpoint |
| 24Mg | 6432.300; 0+ | 7 | 12 | Light deformed nucleus; rotational and interband cascades |
| 27Al | 3004.200; 9/2+ | 7 | 16 | Odd-mass sd-shell spectrum with half-integer spins |
| 48Ca | 4506.780; 3− | 5 | 5 | Doubly magic nucleus with a sparse, high-energy low-lying spectrum |
| 94Mo | 3128.660; 1+ | 34 | 67 | Mixed-symmetry/scissors-mode context; M1 and mixed M1/E2 branches |
| 150Nd | 1598.500; 10+ | 34 | 59 | Shape-transition candidate; several low-lying bands |
| 166Er | 1786.975; 6− | 30 | 90 | Deformed rare-earth spectrum with rotational and interband cascades |
| 178Hf | 2446.090; 16+ | 110 | 316 | Long-lived high-K isomer, high-spin cascades and lower isomers |
| 208Pb | 4085.520; 2+ | 12 | 27 | Closed-shell nucleus with the collective 2614.522-keV octupole state |

The maxima are actual levels in the bundled dataset. These are deliberately
different windows chosen to include the feature of interest, not a uniform
Coulomb-excitation limit or a claim that every level can be excited in a
particular experiment. All absolute-energy levels at or below the maximum
are included, not only descendants of the upper state. For example, the
178Hf file contains the full lower window, not just the isomer-fed subgraph.

Useful starting analyses:

- **12C:** follow the 7654.2 → 4438.91 → 0 cascade. The graph describes its
  gamma branch only. It does **not** imply that every Hoyle-state decay
  produces these photons: competing alpha decay and pair emission are absent.
  [A radiative-cascade measurement](https://arxiv.org/abs/2009.10915) provides
  the physical context, not the numerical branching input to these files.
- **16O:** compare cascades from the 7116.85-keV state, the 6129.89-keV 3−
  gamma decay, and the 6049.4-keV 0+ endpoint. Its E0 transition cannot be
  represented as a single-photon edge. The endpoint is not physically stable.
- **24Mg / 166Er:** compare light and heavy deformed spectra and paths that
  connect different level sequences. Band assignments are interpretive
  context, not fields inferred by this generator. An
  [experimental study of deformed-nucleus bands](https://thesis.caltech.edu/9580/)
  includes 166Er.
- **27Al:** compare branching among half-integer-spin levels. The cutoff
  includes the 3.004-MeV level studied in
  [this spin/parity experiment](https://journals.aps.org/prc/abstract/10.1103/PhysRevC.100.014307).
- **48Ca / 208Pb:** compare two closed-shell spectra and their low-lying
  gamma paths. The 208Pb octupole state is discussed in
  [this shell-model study](https://arxiv.org/abs/2203.13541).
- **94Mo:** start at the 3128.66-keV 1+ level and inspect M1/E2 branching.
  [Measurements of mixed-symmetry states](https://arxiv.org/abs/nucl-ex/9907014)
  motivate this example. A branching graph alone does not establish a
  mixed-symmetry assignment or provide absolute B(M1)/B(E2) values.
- **150Nd:** examine the ground-band cascade through 10+, and branches near
  the excited 0+ and 2+ levels. It is an X(5) candidate, not an exact
  realization of a solved symmetry; see this
  [comparison of shape-transition descriptions](https://repository.lsu.edu/physics_astronomy_pubs/6751/).
  Its ground-state double-beta decay is outside this gamma-only window.
- **178Hf:** select the 2446.09-keV 16+ level and follow its lower cascades.
  The bundled half-life is 9.78286e8 s. The high-K interpretation and
  selection-rule hindrance come from spectroscopy, not graph topology; see
  [a spontaneous-isomer-decay measurement](https://digital.library.unt.edu/ark%3A/67531/metadc1413376/).
  The graph contains no clock or K quantum-number model. Its level energies
  are those of the bundled release, which can differ from later evaluations.

## Probability and completeness conventions

Only positive-intensity, non-E0 downward gamma transitions are edges. Every
such branch from a selected level must end at an included lower level; the
generator fails if a branch would leave the window. Relative gamma intensities
are normalized separately at each emitting level, as in DecayQuiverBuilder.
These probabilities are conditional gamma branches, not total physical
branching fractions including conversion or particle emission, excitation
populations, cross sections, or measured source photon yields.

The files intentionally do not add alpha, beta, EC, neutron-emission,
double-beta, conversion-electron or E0 edges. The variety is in nuclear
structure and electromagnetic gamma cascades; these are not complete
multi-mode radioactive-decay networks. The omission of particle competition
is particularly consequential for 12C above the alpha threshold. No timing
cut, detector response, angular correlation, or unmeasured branch is inferred.

Excited gamma-terminal counts are 1 for 16O, 7 for 94Mo, 7 for 150Nd,
and 25 for 178Hf; the other files have none. These endpoints signify E0-only
decay or missing usable gamma data, **not stability**. Their names and reasons
are in `metadata.excited_gamma_terminal_levels`. All excluded E0 and
nonpositive-intensity branches are recorded in `metadata.omitted_branches`.
Floating-energy levels are excluded and recorded separately because an
absolute energy cutoff cannot be assigned to them reliably.

Names preserve isotope, level ID and excitation energy; gamma labels include
the tabulated photon energy. Metadata also preserves raw spin/parity,
half-lives, intensities, multipolarities, mixing ratios, and conversion
coefficients. These fields support inspection but Studio imports only the
standard levels/transitions/vectors and may omit extra metadata on re-export.
No initial populations or vectors are preselected. A zero tabulated half-life
is not a measurement of instantaneous decay.

## Regenerate

From the repository root with a C++17 compiler and Qt5Core development files:

```sh
g++ -std=c++17 -fPIC -Iinclude -Igui \
  tools/generate_structure_quivers.cpp gui/QuiverJson.cpp \
  src/Core/*.cxx src/NuclearData/PhotonEvaporationReader.cxx \
  $(pkg-config --cflags --libs Qt5Core) -o /tmp/generate_structure_quivers
/tmp/generate_structure_quivers
```

The generator verifies the upper level's energy/spin, descending closed-window
edges, successful import through `Studio::readJson`, level/edge counts and
outgoing normalization to 1e-12. The generated files were also independently
checked against the raw photon files and for byte-identical regeneration.

## Studio analysis update

The new right-hand analysis panel now preserves metadata on import/export and
uses source feeding or the recorded upper level to create decay vectors. This
supersedes the earlier statements above about metadata not being used or being
omitted on export. See [Studio analysis](../../docs/studio-analysis.md) for the
coefficient convention, feeding calculations and table controls. Gamma-edge
probabilities in these JSON files have not changed.
