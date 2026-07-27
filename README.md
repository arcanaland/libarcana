# libarcana

`libarcana` is the shared C++26 core for the Arcana Land tarot-deck tooling: parsing and
validating decks against the [Tarot Deck Specification](https://github.com/arcanaland/specifications),
consumed by [Cartomancer](https://github.com/arcanaland/cartomancer) (a CLI) and
[Tarot Canvas](https://github.com/arcanaland/tarot-canvas) (a PyQt6 GUI) so that deck logic
is implemented once instead of twice.

Two rules define the shape of the public API and are non-negotiable:

- **Rule 1 — the public API is POD-only.** No `toml++` types, no other third-party types,
  cross the `include/arcana/` boundary. Third-party dependencies are always linked
  `PRIVATE`.
- **Rule 2 — no Qt, anywhere.** Not Qt Core, not `QtTest`, not `AUTOMOC`. `libarcana` will
  be loaded into a PyQt6 process via a Python binding later, and a second copy of Qt in
  that process is an ODR crash waiting to happen.

See `arcanaland/cartomancer`'s `agents/RFC/RFC-001-libarcana-shared-core.md` and
`agents/ADR/ADR-002-libarcana-toolchain-floor.md` for the full reasoning.

## Building

Everything builds and tests inside a pinned Fedora container driven by
[`just`](https://github.com/casey/just). The host needs only Podman and `just` — nothing
else, not even a compiler.

```bash
just build-image && just build && just test
```

Other useful recipes: `just format`, `just check-format`, `just tidy`, `just install-check`,
`just clean`. Run `just` with no arguments for the full list.
