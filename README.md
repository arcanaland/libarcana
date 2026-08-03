# libarcana

`libarcana` is C++ library implementing Arcana Land's [Tarot Deck spec](https://github.com/arcanaland/specifications).

## Building

Builds use a Fedora container via podman invoked by `just` (>= 1.32).

```bash
just build-image && just build && just test
```

## Build options

| Option | Default | Notes |
|---|---|---|
| `ARCANA_BUILD_TESTS` | on at top level | Off automatically when a subproject. |
| `ARCANA_INSTALL` | on at top level | Install lib in a staged prefix. |
| `ARCANA_FETCH_REFERENCE_DECKS` | `ON` | Downloads [reference-decks](https://github.com/arcanaland/reference-decks) for tests. |
| `ARCANA_REFERENCE_DECKS_TAG` | pinned SHA | Commit to test against. |
| `ARCANA_REFERENCE_DECKS_DIR` | empty | Local path that overrides  `ARCANA_FETCH_REFERENCE_DECKS`. |
| `ARCANA_BUILD_PYTHON` | `OFF` | Builds the Python binding. |

