# COINAlgebra

**COINAlgebra** is a C++ toolkit for representing nuclear decay schemes as **decay quivers** and constructing the associated **coincidence algebra**.

The project is intended to provide a computational framework for calculating decay-path, coincidence, and detection quantities directly from a nuclear decay scheme.

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
* ROOT-compatible classes and dictionaries for interactive use.
* Interactive `COINAlgebra` environment based on ROOT's `TRint`.
* Test macros demonstrating the current functionality.

## Repository Structure

```text
COINAlgebra/
├── CMakeLists.txt
├── README.md
├── setup.sh
│
├── include/COINAlgebra/
│   ├── COINAlgebra.h
│   ├── Commands.h
│   ├── DecayLevel.h
│   ├── DecayPath.h
│   ├── DecayQuiver.h
│   ├── DecayTransition.h
│   ├── DecayVector.h
│   └── LinkDef.h
│
├── src/
│   ├── COINAlgebra.cxx
│   ├── Commands.cxx
│   ├── DecayLevel.cxx
│   ├── DecayPath.cxx
│   ├── DecayQuiver.cxx
│   ├── DecayTransition.cxx
│   ├── DecayVector.cxx
│   └── main.cxx
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
├── bin/
└── lib/
```

## Requirements

* CMake ≥ 3.16
* C++17 compiler
* CERN ROOT 6.x
* ROOT's CMake integration

The project is designed to work with a standard ROOT installation and does not require a hardcoded path inside the repository. The only installation-specific value that users may need to provide is the ROOT prefix for their local environment.

## ROOT Setup

Before building or running the project, point the repo at the ROOT install you want to use.

The simplest pattern is to set a single environment variable once in your shell:

```bash
export ROOT_PREFIX=/path/to/root
# examples:
# export ROOT_PREFIX=/opt/root
# export ROOT_PREFIX=/usr/local
# export ROOT_PREFIX=/root/miniconda3/envs/root626
```

Then load the project environment:

```bash
source setup.sh
```

The script will use `ROOT_PREFIX` if it is set, otherwise it falls back to the currently active `ROOTSYS` and then common ROOT install locations.

You can also pass the ROOT install directly to CMake:

```bash
cmake -DROOT_DIR=/path/to/root -S . -B build
```

## Building

From the repository root:

```bash
export ROOT_PREFIX=/path/to/root
source setup.sh

rm -rf build
cmake -S . -B build
cmake --build build -j4
```

This will build the library and executable directly into the repository:

```text
bin/coinalgebra
lib/libCOINAlgebra.so
```

Run the interactive environment with:

```bash
source setup.sh
./bin/coinalgebra
```

The current development environment has been tested with **ROOT 6.26.10** and **GCC 10/11-compatible toolchains**. The project is intended to work with other standard ROOT 6 installations as long as the active compiler and Cling toolchain are compatible.

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

