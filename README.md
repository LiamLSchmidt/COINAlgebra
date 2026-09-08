# COINAlgebra

COINAlgebra is a C++17 toolkit for representing nuclear decay schemes as decay
quivers and constructing path, probability, and coincidence-algebra machinery.
It includes a ROOT interactive environment and **DecayQuiver Studio**, a Qt
quiver-builder GUI.

## License

Original project code and documentation are licensed under the [MIT License](LICENSE).
Bundled nuclear datasets and third-party dependencies retain their upstream
terms; see [Third-party notices](THIRD_PARTY_NOTICES.md).

## Build

Requirements: CMake 3.16+, a C++17 compiler, ROOT for the optional interactive
build, and Qt5 Widgets for the optional GUI. The standalone library requires
neither ROOT nor Qt.

```bash
cmake -S . -B build -DBUILD_WITH_ROOT=ON -DBUILD_GUI=ON
cmake --build build -j4
source setup.sh
./bin/coinalgebra
```

`BUILD_WITH_ROOT` and `BUILD_GUI` default to `ON`. Missing ROOT is an error when
requested; missing Qt5 Widgets skips the GUI with a configure-time message.
Use `-DBUILD_WITH_ROOT=OFF` for a standalone library and optional GUI build.
Use `-DBUILD_GUI=OFF` to skip Qt entirely. `BUILD_TESTING` defaults to `ON`.

Development outputs retain their existing locations:

- `lib/libCOINAlgebraCore.a`: standalone mathematical/data library.
- `lib/libCOINAlgebra.so` and dictionary files: optional ROOT library.
- `bin/coinalgebra`: optional ROOT interpreter.
- `gui/bin/DQStudio`: DecayQuiver Studio. After sourcing `setup.sh`, run
  `DQStudio` from any directory. A user-local launcher in `~/.local/bin` also
  makes the command available without sourcing the project setup.

Different build directories share these development outputs; build them
sequentially. The GUI exports timestamped JSON files into `examples/`.
Use **Import Quiver** (above **Export Quiver**) to open an existing JSON quiver.
Imports are validated before replacing the workspace and include saved vectors.
Levels are placed from bottom to top in insertion order. Right-click a
transition and choose **Edit properties** to edit its source, target and
probability together. The logo and slate/teal theme are embedded in the executable. See
[the Studio JSON format](docs/studio-json.md) for supported fields.

In the ROOT environment:

```cpp
help();
version();
.x tests/core/test_quiver.C
.x tests/integration/test_reader_gamma_quiver.C(12,22,11,22)
```

## Layout

Public headers under `include/COINAlgebra/` mirror implementations under `src/`:

| Module | Responsibility |
| --- | --- |
| `Core/` | Decay levels, transitions, paths, quivers, vectors |
| `Algebra/` | Path algebra and projectors |
| `Probability/` | Decay probability calculations |
| `Detection/` | Reserved for efficiencies and detector response |
| `Builders/` | Construct quivers from nuclear records |
| `NuclearData/` | Nuclear records and Geant4 readers |
| `IO/` | Reserved for common import/export functionality |
| `ROOT/` | ROOT application wrapper, commands, dictionary configuration |

The existing records remain with their reader headers. This structural refactor
does not redesign mathematical definitions or record types.

```text
apps/root/             ROOT executable entry point
cmake/                 Shared source/header list
config/                ROOT startup macros
include/COINAlgebra/    Modular headers and legacy forwarding headers
src/                   Module implementations
gui/                   Qt client of COINAlgebraCore
examples/              Basic macro, sample JSON, future module examples
tests/                 Core, algebra, reader, integration and GUI tests
validation/            Future matrix/Geant4 comparisons, datasets and results
data/                  Future curated examples and external-data configuration
external/              Future third-party dependency support
tools/                 Future CLI tools
bindings/python/       Future Python bindings and tests
docs/                  Architecture, mathematics, tutorials, API and paper
scripts/               Build and test entry points
.github/workflows/     Reserved for future CI workflows
```

Empty extension locations contain `.gitkeep`; they are not implemented features.
The existing `PhotonEvaporation5.5/` and `RadioactiveDecay5.5/` datasets remain in
place because current readers/tests use them. External-data migration is a later
roadmap item.

New code can include `COINAlgebra/Core/DecayQuiver.h`,
`COINAlgebra/Algebra/PathAlgebra.h`, etc. Legacy flat includes such as
`COINAlgebra/DecayQuiver.h` still forward to the corresponding module. The legacy
`COINAlgebra/COINAlgebra.h` continues to expose the ROOT application wrapper.
CMake consumers in this build can link `COINAlgebra::Core`.

## Tests

```bash
ctest --test-dir build --output-on-failure
```

The algebra macros use quiver-bound constructors and test vertex projectors
that aggregate coefficients across all source or target vertices.

CTest also runs the existing core assertions as compiled C++ tests and, when Qt
is available, an offscreen GUI smoke test covering levels, a transition, scene
creation, a single-transition path/vector, and JSON export. It does not replace
manual testing of dragging, editing or multi-transition vectors.

See [docs/architecture.md](docs/architecture.md) for module boundaries and
[docs/studio-json.md](docs/studio-json.md) for the Studio file format.
