# libarcana

`libarcana` is C++ library implementing Arcana Land's [Tarot Deck spec](https://github.com/arcanaland/specifications).

## Building

Builds and tests use a Fedora container driven by `just`.

```bash
just build-image && just build && just test
```

