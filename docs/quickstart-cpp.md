# C++ quickstart

`libarcana` is a C++26 static library. There are no binary releases and no packaged builds
yet: you build it from source and either install it or vendor it into your build.

Everything quoted below is a runnable program in [`examples/`](../examples), built and run in
CI against the reference decks:

```bash
just build
./build/RelWithDebInfo/examples/list_decks ~/decks
./build/RelWithDebInfo/examples/card_assets ~/decks rider-waite-smith
```

## Getting it into your build

The project is `arcana` everywhere — `find_package(arcana)`, the target `arcana::arcana`,
`namespace arcana`, `#include <arcana/…>`. Only the repository, the Conan package and
`libarcana.a` carry the longer name.

Vendored, via `FetchContent` or `add_subdirectory` — tests and install rules turn themselves
off when `arcana` is not the top-level project:

```cmake
include(FetchContent)
FetchContent_Declare(arcana
    GIT_REPOSITORY https://github.com/arcanaland/libarcana.git
    GIT_TAG v0.1.0)
FetchContent_MakeAvailable(arcana)

target_link_libraries(your_app PRIVATE arcana::arcana)
```

Or against an installed copy. The install ships both a CMake package config and a CPS
description, and `find_package` will take whichever your CMake prefers:

```cmake
find_package(arcana 0.1.0 REQUIRED)
target_link_libraries(your_app PRIVATE arcana::arcana)
```

The only dependency is header-only [toml++](https://github.com/marzer/tomlplusplus), which
the build resolves with `find_package` and fetches at `v3.4.0` if that fails.

You need a compiler that can build C++26 — GCC from `fedora:44` is what the build container
uses, and CMake 4.3 or newer. `<arcana/version.hpp>` gives you `arcana::library_version()`
plus `ARCANA_VERSION_AT_LEAST(major, minor, patch)` for conditional compilation. Expect ABI
breaks before 1.0.

## Listing the decks on the system

[`examples/list_decks.cpp`](../examples/list_decks.cpp)

```cpp
#include <arcana/library.hpp>
#include <arcana/paths.hpp>

// An empty roots list means the XDG library, $XDG_DATA_HOME/tarot/decks.
arcana::library_options options;
options.roots.emplace_back("/home/you/decks");   // searched in order, like PATH

arcana::deck_library library{options};           // the scan happens here

for (arcana::deck_summary const& deck : library.decks())
    std::println("{} {} ({} cards) {}", deck.directory_name, deck.name, deck.card_count,
                 deck.path.string());

for (arcana::malformed_deck const& broken : library.malformed_decks())
    std::println("malformed {}: {}", broken.directory_name, broken.problem.message);
```

Constructing the library reads every `deck.toml` and nothing else — no image is touched
until you load. `arcana::paths::deck_library_path()` gives the default root without
constructing anything.

`decks()`, `malformed_decks()`, `roots()` and `languages()` hand back **`std::span`s into the
library**, not copies. They stay valid until `refresh()`, and never outlive the library —
copy out what you intend to keep. `find(directory_name)` returns an owned
`std::optional<deck_summary>` instead.

## Loading a deck

[`examples/load_deck.cpp`](../examples/load_deck.cpp)

```cpp
std::expected<std::shared_ptr<arcana::deck const>, arcana::error> loaded =
    library.load("rider-waite-smith");           // by directory name, not by [deck].id

if (!loaded)
{
    std::println("{}: {}", static_cast<int>(loaded.error().code), loaded.error().message);
    return 1;
}

std::shared_ptr<arcana::deck const> deck = *loaded;

std::println("{} {} (schema {})", deck->metadata.name, deck->metadata.version,
             deck->metadata.schema_version);

for (arcana::suit_info const& s : deck->suits())
    std::println("{} {} {}", s.key, s.display_name, deck->cards_in_suit(s.key).size());
```

Everything fallible returns `std::expected<T, arcana::error>`; `error` is an `error_code`
(`not_found`, `parse_error`, `io_error`, `invalid_argument`) and a message. Nothing throws.

`load` caches by directory name, so the second call hands back the same `shared_ptr`. The
deck owns everything it hands out and outlives the library it came from.
`load_external(path)` loads a directory outside the library, and the free
`arcana::load_deck(path, languages)` skips the library entirely.

Set `options.languages` to a preference chain (`{"de", "en"}`) to pick which name files are
read; it falls back to English.

## Finding a card and its assets

[`examples/card_assets.cpp`](../examples/card_assets.cpp)

```cpp
std::optional<arcana::card> fool = deck->find_card(arcana::card_id::standard_major(0));
auto ace = deck->find_card(arcana::card_id::standard_minor(arcana::suit::cups,
                                                           arcana::rank::ace));

// Parsing is fallible, so it comes back as std::expected.
std::expected<arcana::card_id, arcana::error> id =
    arcana::card_id::parse("minor_arcana.swords.king");

if (!fool)
    std::println("excluded: {}",
                 deck->exclusion_reason("major_arcana.00").value_or("not declared"));

for (arcana::card_image const& image : fool->images)
    std::println("{} {}", image.source_dir, image.path.string());

auto raster = fool->best_raster_for_height(600);   // smallest at or above 600px
auto ansi = fool->best_ansi_for_lines(40);         // largest at or below 40 lines
auto svg = fool->scalable_image();                 // nullopt when the deck ships no SVG
```

Paths are absolute and already resolved against the deck root. `find_card` returns
`std::nullopt` for a card this deck excludes or never declared, which is an answer rather
than an error — `exclusion_reason` says which of the two it was.

Card backs are deck-wide rather than per-card:

```cpp
if (std::optional<arcana::card_back_variant> back = deck->default_card_back_variant())
    std::println("{} {}", back->id, back->image.string());
```

The rest of the deck surface: `cards_of_kind`, `cards_in_suit`, `random_card(seed)`
(deterministic), `display_suit_name` / `display_rank_name` for what this deck calls things,
`variants`, `custom_suits`, `major_arcana_remap`, and `source_toml()` for the manifest
re-serialized.

## Validating a deck

[`examples/validate_deck.cpp`](../examples/validate_deck.cpp)

```cpp
#include <arcana/validation.hpp>

for (arcana::diagnostic const& finding : arcana::validate(*deck))
    std::println("{} {} {}", static_cast<int>(finding.level), finding.code, finding.message);
```

A `diagnostic` carries a `severity` (`pedantic`, `info`, `warning`, `error`), the `code` of
the rule that produced it, an interpolated `message`, and whichever of card / path /
`deck.toml` key locates it. Only `error` means the deck is non-conforming; every severity
still loads.

`arcana::rules()` is the whole catalogue — each `rule` carries its code, default severity,
area, the spec sections behind it, and the `schema_version` majors it applies to. The
catalogue is written from the specification and is deliberately ahead of the checks, so its
size is not a count of what `validate` judges. Rule codes are API and are never renamed.

## Building this repository

Builds run in a Fedora container driven by `just`:

```bash
just build-image
just build && just test && just check-format && just tidy && just lint-reuse
```

`ctest -L examples` runs just the example programs. `-DARCANA_BUILD_EXAMPLES=OFF` skips
building them; see the root `README.md` for the other build options.
