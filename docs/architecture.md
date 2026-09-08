# Architecture

This first restructuring changes file organization and build wiring. Existing
mathematical implementations, public class names and signatures are preserved.

`Core`, `Algebra`, `Probability`, `Builders` and `NuclearData` compile into
`COINAlgebraObjects`, then form the standalone `COINAlgebraCore` static library.
The ROOT shared library uses the same object files with its application wrapper,
commands and generated dictionary. Qt links the standalone library through
`COINAlgebraGuiWidgets`; the scientific code has no Qt dependency.

`COINAlgebraCore` is the existing target name for the complete standalone
scientific library, not only the sources in the `Core/` directory.

`cmake/COINAlgebraSources.cmake` is the explicit source/header inventory. Add
future implemented classes there; reserved directories do not create targets.
The ROOT dictionary uses module header paths. Flat forwarding headers preserve
existing consumers while project sources use the new paths.

The current GUI export and path/vector builder stay in the GUI unchanged.
Moving serialization into `IO` and replacing display-text reconstruction with
structured path data are separate behavior changes. The existing reader record
types also remain together with their readers for now.

Target boundaries for future work:

- Core represents decay objects without file-format, GUI or ROOT dependencies.
- Algebra implements products, forms, projectors and later coincidence types.
- Probability evaluates mathematical probability quantities and normalization.
- Detection handles efficiencies and detector maps separately from decay physics.
- NuclearData parses external formats into records; Builders constructs quivers.
- IO provides reusable serialization and export consumed by application clients.
- ROOT, Qt, CLI and Python are clients of the scientific library.

Strict path equality and `HasSameEndpoints` already exist as separate operations.
Their definitions have not been modified. A future paper-to-code audit will
resolve mathematical questions before introducing new algebraic structures.
