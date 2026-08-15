# Python quickstart

```sh
pip install arcana-tarot
```

> **TODO(adam): this page is unwritten.**
>
> Its prose was generated and has been removed under
> [AI-POLICY](https://github.com/arcanaland/arcanaland/blob/main/AI-POLICY.md) §2. The
> outline below is the plan, not the page.
>
> Every section here has a runnable counterpart in
> [`examples/python/`](../examples/python) that `just python test` runs. Those scripts are
> code, not prose, and stand as the working reference until this page is written.

The distribution is **`arcana-tarot`**; the module you import is **`arcana_tarot`**.

```python
import arcana_tarot as arcana
```

## TODO(adam): listing the decks on the system

Worked program: [`examples/python/list_decks.py`](../examples/python/list_decks.py).

Beats: no roots means the XDG library; roots are searched in order like `PATH`; a root takes
a `str` or a `Path` and nothing expands `~` for you; `decks()` vs. `malformed_decks()`.

## TODO(adam): loading a deck

Worked program: [`examples/python/load_deck.py`](../examples/python/load_deck.py).

Beats: load by directory name, not `[deck].identifier`; loads are cached; everything that
can fail raises `deck_error`, which carries a `code`; `deck.suits` is an attribute, not a
call.

## TODO(adam): finding a card and its assets

Worked program: [`examples/python/card_assets.py`](../examples/python/card_assets.py).

## What is not bound

Deck validation, `arcana::paths` beyond `deck_library_path()`, card backs, deck variants and
custom-suit declarations have no Python surface yet. The C++ API has all of them.

## Building the binding from this repo

```bash
just python test     # build the nanobind module and run the pytest suite
just python wheel    # pip install . into a throwaway venv and import it
```
