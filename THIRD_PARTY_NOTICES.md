# Third-party materials

The root MIT license applies to original COINAlgebra and DecayQuiver Studio
code and documentation. It does not relicense third-party software or nuclear
datasets, or grant rights in underlying data reproduced in generated examples.

## Nuclear data

`PhotonEvaporation5.5/` and `RadioactiveDecay5.5/` are Geant4 data libraries.
Geant4 credits the Evaluated Nuclear Structure Data File (ENSDF), maintained
by the National Nuclear Data Center at Brookhaven National Laboratory, as
the source for these libraries.

See the upstream [data credits and citations](https://geant4.web.cern.ch/download/data_files_citations)
and [Geant4 distribution site](https://geant4.web.cern.ch/download/).
Applicable upstream terms and attribution requirements remain in effect;
this notice does not assign these datasets an MIT license.

Generated quiver JSON files in `examples/sources/`,
`examples/coulomb_excitation/`, and `examples/nuclear_structure/` reproduce
information from these datasets. Their embedded provenance metadata and
accompanying documentation identify the data release used.

## Dependencies

Qt and ROOT are independently licensed dependencies. Their own terms apply
to their use and redistribution, including when distributing linked binaries.
COINAlgebra's MIT license does not replace those terms.
