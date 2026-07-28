# libarcana

`libarcana` is C++ library implementing Arcana Land's [Tarot Deck spec](https://github.com/arcanaland/specifications).

## Building

Builds and tests use a Fedora container via podman driven by `just` (requires >= 1.32).

```bash
just build-image && just build && just test
```


