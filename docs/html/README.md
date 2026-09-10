# HTML documentation

Open `dist/index.html` directly, or serve `dist/` with a static HTTP server.
The site includes the full mathematical guide, searchable class/member indexes,
source-file views with line anchors, equations, code-copy buttons, a printable
guide, and responsive navigation. Rendering requires no external CDN or runtime
server; KaTeX assets are included locally.

The blue banner, tabbed indexes, and tree navigation are inspired by the
[GRSISort Doxygen documentation](https://griffincollaboration.github.io/GRSISort/index.html).
This is a custom static renderer, not Doxygen-generated output. The member index
links to the guide's documented function/overload groups; exact declarations
remain available in the declaration appendix.

## Rebuild

From this directory, with Node.js and pnpm installed:

```sh
pnpm install --frozen-lockfile
pnpm build
pnpm dev
```

The preview server uses port 8765. The generator reads `../class-reference.md`, canonical library/DQStudio headers,
and linked source files. The `<!-- canonical-headers -->` marker expands into
a fresh header appendix at build time; do not hand-copy declarations. Edit that Markdown guide rather than generated
HTML. Rebuild after changing the Markdown or linked headers/examples/tests.
The script checks equation rendering, unique IDs, and internal anchor targets.
The class/member indexes include DetectionMaps and DQStudio types and namespace
modules. The downloaded Markdown includes the expanded header appendix.

- `build.mjs`: Markdown conversion, indexes, local source pages, asset packaging.
- `style.css`: original styling inspired by the reference site's structure.
- `app.js`: navigation, search, filtering, copying, and printing.
- `dist/`: generated portable static website.
- `.openai/hosting.json`: private Sites publication identity and output directory.

The output includes only the manuscript and source files explicitly linked by
the guide. The private preview does not change the repository's visibility.

## GitHub Pages

Public documentation: https://liamlschmidt.github.io/COINAlgebra/

The active workflow `.github/workflows/documentation-pages.yml` runs on `main`
and publishes `documentation-site.tar.gz` from the `documentation-pages` branch.
This isolates the documentation snapshot from uncommitted library changes and
respects the `github-pages` environment's main-branch deployment protection.
Pages is configured to use **GitHub Actions**.

To refresh the snapshot, rebuild this HTML and create an archive from the
repository root:

```sh
tar -czf /tmp/documentation-site.tar.gz -C docs/html/dist .
```

Replace the archive on the `documentation-pages` branch, then run **Publish
documentation** manually from the repository's Actions tab, choosing `main`.
Only the generated site is extracted and uploaded. Sites credentials and
`.openai/hosting.json` are not part of the Pages artifact.

After the documentation and linked source files are integrated into `main`,
`documentation-pages-source.yml` in this directory can replace the active
workflow to rebuild automatically from the Markdown and headers. It is a
migration template, not an active second deployment workflow.

The private Sites preview is independent of this public Pages publication.
