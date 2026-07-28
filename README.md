# libarcana

`libarcana` is C++ library implementing Arcana Land's [Tarot Deck spec](https://github.com/arcanaland/specifications).

## Building

Builds use a Fedora container via podman invoked by `just` (>= 1.32).

```bash
just build-image && just build && just test
```


