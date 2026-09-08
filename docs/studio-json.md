# DecayQuiver Studio JSON

Launch Studio with `DQStudio` from any directory once `gui/bin` is on PATH
(via `source setup.sh`) or the user-local launcher in `~/.local/bin` is installed.
The executable is `gui/bin/DQStudio` and the window title is DecayQuiver Studio. Its logo is embedded from
`DQStudio_logo.jpeg` through `gui/studio.qrc`, so it also loads when launched from
another working directory. The slate/teal stylesheet lives in `gui/theme.qss`.

**Import Quiver**, above **Export Quiver**, opens a JSON file picker. The file
must validate completely before the current document can be replaced. When a
quiver is already open, Studio asks whether to replace it; choose No to keep it
and export it first. Invalid files display an error and preserve the workspace.

The format matches existing Studio exports:

```json
{
  "levels": ["d2", "d1", "d0"],
  "transitions": [
    {"name": "a", "source_index": 0, "target_index": 1, "probability": 0.4},
    {"name": "b", "source_index": 1, "target_index": 2, "probability": 1.0}
  ],
  "vectors": [
    {"terms": [
      {"path_names": ["a", "b"], "coefficient": 2.0},
      {"path_names": [], "stationary_level": "d0", "coefficient": 0.5}
    ]}
  ]
}
```

- `levels` and `transitions` are required arrays. Names must be non-empty and
  unique within their respective collection.
- Transition indices are zero-based integers referring to `levels`. Individual
  probabilities must be numeric and between zero and one. Import does not
  require outgoing probabilities to sum to one, allowing unfinished schemes.
- `vectors` is optional. Each vector contains a `terms` array. Coefficients must
  be finite numbers; negative coefficients are supported.
- `path_names` lists existing transition names in composition order. Each path
  is validated for composability. Parallel transitions with distinct names are
  supported by import.
- For stationary paths, `path_names` is empty and `stationary_level` identifies
  the level by name. New exports include this field. An old empty path without
  an identified level is ambiguous and is rejected with an explanation.
- Exported `display` and `path_probability` are derived metadata. Import uses
  structured fields and recomputes path probabilities from transitions.

Export continues to write timestamped JSON in `examples/`. Layout positions
are not serialized; imported quivers start with the default layout. The reader
is currently Qt-specific (`gui/QuiverJson.*`) and does not add Qt dependencies
to the scientific library.

Tests cover file selection, replace/cancel, an export/import/export round trip,
failed-import preservation, vector ownership, long and stationary paths, and
invalid syntax, types, indices, probabilities, duplicate names and path references.
