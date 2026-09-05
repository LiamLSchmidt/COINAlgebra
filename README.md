# COINAlgebra

**COINAlgebra** is a C++ toolkit for representing nuclear decay schemes as **decay quivers** and constructing the associated **coincidence algebra**.

The project provides a computational framework for building decay networks from nuclear-level data, composing decay paths, and evaluating feeding and coincidence probabilities. It is designed to work directly with the Geant4 radioactive-decay and photon-evaporation databases, which are used as the source of nuclear-level and transition information.

## Features

* **Decay levels** represented as vertices of a quiver.
* **Decay transitions** represented as directed arrows with associated probabilities.
* **Decay paths** constructed from composable transitions.
* **Decay vectors** formed as linear combinations of decay paths.
* **Decay quivers** supporting:

  * level and transition management,
  * path composability,
  * direct-transition checks,
  * transition-probability normalization.
* **Path algebra** for multiplication, powers, and bilinear forms on decay vectors.
* **Feeding and coincidence probabilities** computed from path and vertex projectors.
* **Geant4 data readers** for the radioactive-decay and photon-evaporation databases.
* ROOT-compatible classes and dictionaries for interactive use.
* Interactive `COINAlgebra` environment based on ROOT's `TRint`.
* Test macros demonstrating the current functionality.

## Repository Structure

```text
COINAlgebra/
├── CMakeLists.txt
├── README.md
├── setup.sh
├── rebuild.sh
├── regit.sh
│
├── include/COINAlgebra/
│   ├── COINAlgebra.h
│   ├── Commands.h
│   ├── DecayLevel.h
│   ├── DecayPath.h
│   ├── DecayProbability.h
│   ├── DecayQuiver.h
│   ├── DecayQuiverBuilder.h
│   ├── DecayTransition.h
│   ├── DecayVector.h
│   ├── LinkDef.h
│   ├── PathAlgebra.h
│   ├── PathProjectors.h
│   ├── PhotonEvaporationReader.h
│   └── RadioactiveDecayReader.h
│
├── src/
│   ├── COINAlgebra.cxx
│   ├── Commands.cxx
│   ├── DecayLevel.cxx
│   ├── DecayPath.cxx
│   ├── DecayProbability.cxx
│   ├── DecayQuiver.cxx
│   ├── DecayQuiverBuilder.cxx
│   ├── DecayTransition.cxx
│   ├── DecayVector.cxx
│   ├── main.cxx
│   ├── PathAlgebra.cxx
│   ├── PathProjectors.cxx
│   ├── PhotonEvaporationReader.cxx
│   └── RadioactiveDecayReader.cxx
│
├── examples/
│   └── example_vector.C
│
├── tests/
│   ├── test_path.C
│   ├── test_quiver.C
│   └── test_vector.C
│
├── config/
│   ├── COINAlgebraLogon.C
│   └── COINAlgebraLogon.h
│
├── data/
├── macros/
├── PhotonEvaporation5.5/
│   └── README-LevelGammaData
├── RadioactiveDecay5.5/
│   └── README_RDM
├── bin/
├── lib/
└── build/
```

## Core modules and their purpose

The project now includes a more complete set of algebraic and data-access components. The most important ones are:

* `COINAlgebra.h` / `COINAlgebra.cxx`  
  Top-level library interface and ROOT integration entry points.

* `Commands.h` / `Commands.cxx`  
  Commands exposed in the interactive ROOT shell for quick access to library functionality.

* `DecayLevel.h` / `DecayLevel.cxx`  
  Represents a nuclear level or quiver vertex, including its identity and decay-related metadata.

* `DecayTransition.h` / `DecayTransition.cxx`  
  Represents a directed decay transition between levels, including the transition label, connection, and associated probability.

* `DecayPath.h` / `DecayPath.cxx`  
  Represents a sequence of connected transitions; this is the primitive object used to build decay chains.

* `DecayVector.h` / `DecayVector.cxx`  
  Represents a linear combination of decay paths with coefficients and algebraic operations.

* `DecayQuiver.h` / `DecayQuiver.cxx`  
  Stores the graph of levels and transitions, handles path composition, and provides the quiver structure underlying the algebra.

* `DecayProbability.h` / `DecayProbability.cxx`  
  Computes feeding vectors, feeding probabilities, path connections, and coincidence probabilities with respect to a given decay vector and set of path projectors.

* `PathAlgebra.h` / `PathAlgebra.cxx`  
  Defines the path-algebra product, powers, identities, and bilinear forms needed for algebraic manipulations of decay vectors.

* `PathProjectors.h` / `PathProjectors.cxx`  
  Implements source/target and branching projections that isolate specific path subspaces and stationary contributions within a decay vector.

* `DecayQuiverBuilder.h` / `DecayQuiverBuilder.cxx`  
  Builds a gamma-decay quiver by matching radioactive-decay daughter energies to photon-evaporation levels and selecting physically relevant decay channels.

* `RadioactiveDecayReader.h` / `RadioactiveDecayReader.cxx`  
  Parses the Geant4 radioactive-decay data files (`z*.a*` files), reads parent states, decay modes, and branching channels, and converts the text tables into structured C++ objects.

* `PhotonEvaporationReader.h` / `PhotonEvaporationReader.cxx`  
  Parses the Geant4 photon-evaporation level-gamma data and stores excitation energies, half-lives, JPi information, and gamma deexcitation transitions for each isotope.

* `main.cxx`  
  Entry point for the interactive ROOT application.

* `LinkDef.h`  
  ROOT dictionary definitions for exposing COINAlgebra classes to the interpreter.

## Nuclear data sources

The library is designed around the Geant4 nuclear data files distributed with Geant4. The data files in the directories `PhotonEvaporation5.5/` and `RadioactiveDecay5.5/` are not custom local tables; they are the standard Geant4 nuclear decay datasets used for level structure, gamma branching, and radioactive decay channels.

These readers load the Geant4 files and convert them into C++ objects that the quiver and probability infrastructure can manipulate.

## Requirements

* CMake ≥ 3.16
* C++17 compiler
* CERN ROOT
* ROOT's CMake integration

The current development environment has been tested with **ROOT 6.26.10** and **GCC 11**.

## Building

From the repository root:

```bash
rm -rf build
mkdir build
cd build

cmake ..
make -j4
```

The executable and library are placed directly in the repository:

```text
bin/coinalgebra
lib/libCOINAlgebra.so
```

Run the interactive environment with:

```bash
./bin/coinalgebra
```

## Interactive Environment

COINAlgebra provides a ROOT-based interactive environment:

```text
COINAlgebra [0]>
```

ROOT commands can be used normally, for example:

```text
.L tests/test_quiver.C
test_quiver()
```

The current command interface also provides:

```text
help()
version()
```

## Example

A simple decay scheme can be constructed from levels and transitions:

```cpp
DecayLevel* d0 = quiver.AddLevel("d0");
DecayLevel* d1 = quiver.AddLevel("d1");
DecayLevel* d2 = quiver.AddLevel("d2");

quiver.AddTransition("gamma1", d2, d1, 0.6);
quiver.AddTransition("gamma2", d1, d0, 1.0);
quiver.AddTransition("gamma3", d2, d0, 0.4);
```

Paths can then be constructed from composable transitions:

```text
d2 -[gamma1]-> d1
d1 -[gamma2]-> d0

d2 -[gamma1]-> d1 -[gamma2]-> d0
```

Decay vectors allow linear combinations of such paths:

```text
v = 2 p1 + 3 p3
```

The algebraic structure will be extended to incorporate coincidence products and the associated probability calculations.

## Development Status

COINAlgebra is currently under active development.

The present implementation establishes the basic computational objects:

$$
\text{Decay Quiver}
\longrightarrow
\text{Decay Paths}
\longrightarrow
\text{Decay Vectors}.
$$

Future development will build the coincidence algebra on top of these structures, including multiplication, coincidence probabilities, detection efficiencies, and related nuclear-decay calculations.

## Documentation

Detailed mathematical and software documentation is maintained separately from this README.

The documentation covers:

* the mathematical motivation for the coincidence algebra,
* decay quivers,
* decay levels and transitions,
* paths and path composition,
* decay vectors,
* probability normalization,
* ROOT integration,
* the CMake build system,
* adding new source files,
* ROOT dictionary generation,
* testing and development workflow.

## License

License information will be added as the project develops.

