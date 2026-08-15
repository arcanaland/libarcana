# C++ quickstart

> **TODO(adam): this page is unwritten.**
>
> Its prose was generated and has been removed under
> [AI-POLICY](https://github.com/arcanaland/arcanaland/blob/main/AI-POLICY.md) §2. The
> outline below is the plan, not the page.
>
> Every section here has a runnable counterpart in [`examples/`](../examples) that CI builds
> and runs against the reference decks. Those programs are code, not prose, and stand as the
> working reference until this page is written.

## TODO(adam): getting it into your build

Cover `find_package(arcana)` / `arcana::arcana`, the CPS and CMake config that
`ARCANA_INSTALL` stages, and the C++26 floor. `tests/consumer/` is the worked example — it
is what the `consume_cps` and `consume_script` ctest cases build.

## TODO(adam): listing the decks on the system

Worked program: [`examples/list_decks.cpp`](../examples/list_decks.cpp).

Beats to hit: `library_options` roots default to the XDG library; roots shadow like `PATH`;
`decks()` vs. `malformed_decks()` and why a broken deck is reported rather than dropped.

## TODO(adam): loading a deck

Worked program: [`examples/load_deck.cpp`](../examples/load_deck.cpp).

Beats: loading is by directory name; loads are cached and hand back a
`shared_ptr<deck const>` that outlives the library; `load_deck` for a path outside any
library lives in `<arcana/loader.hpp>`; `deck->suits` is a member, not a call.

## TODO(adam): finding a card and its assets

Worked program: [`examples/card_assets.cpp`](../examples/card_assets.cpp).

Beats: the three ways to name a card; image-kind selection by the space being drawn into;
card backs are deck-wide (`default_card_back_design()`), not per-card.

## TODO(adam): validating a deck

Worked program: [`examples/validate_deck.cpp`](../examples/validate_deck.cpp).

## TODO(adam): error handling

Everything fallible returns `std::expected<T, arcana::error>`; `error` carries an
`error_code` (`not_found`, `parse_error`, `io_error`, `invalid_argument`) and a message.
Nothing throws.

## Building this repository

```sh
just build-image
just build && just test && just check-format && just tidy && just lint-reuse
```

`ctest -L examples` runs just the example programs. `-DARCANA_BUILD_EXAMPLES=OFF` skips
building them.
