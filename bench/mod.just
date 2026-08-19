# SPDX-FileCopyrightText: 2026 Adam Fidel
# SPDX-License-Identifier: MIT

import '../just/common.just'

# Recipe bodies and the podman shim's paths are all written from the repo root
set working-directory := '..'

bench_bin := build_dir / "bench/arcana-bench"
micro_bin := build_dir / "bench/arcana-bench-micro"

# The library the podman shim mounts the host's decks at
library := "/decks"

repeat := "1000"

default:
    @just --list bench

[script]
list: build
    {{bench_bin}} --list

[script]
build: configure
    cmake --build --preset {{preset}} --target arcana-bench arcana-bench-micro

[doc('Per-call cost of each workload, in-process (Catch2). Pass a deck name to filter.')]
[group('bench')]
[script]
micro deck="" *args: build
    ARCANA_BENCH_LIBRARY_DIR={{library}} {{micro_bin}} '[.bench]' \
        {{ if deck == "" { "" } else { "-c " + quote(deck) } }} {{args}}

[doc('One workload under hyperfine, e.g. `just bench run load my-tarot`.')]
[group('bench')]
[script]
run workload deck="" *args: build
    target="{{ if deck == "" { "--library " + library } else { "--deck " + library / deck } }}"
    hyperfine --warmup 3 --style basic {{args}} \
        -n '{{workload}}' \
        "{{bench_bin}} {{workload}} $target --repeat {{repeat}} --quiet"

[doc('Compare one workload across every deck in the library.')]
[group('bench')]
[script]
sweep workload *args: build
    decks=$(ls -1 {{library}})
    hyperfine --warmup 3 --style basic {{args}} \
        -L deck "$(echo $decks | tr ' ' ',')" \
        -n '{{workload}} {deck}' \
        "{{bench_bin}} {{workload}} --deck {{library}}/{deck} --repeat {{repeat}} --quiet"

[doc('Run every workload once.')]
[group('bench')]
[script]
table deck="": build
    subject="{{ if deck == "" { "" } else { deck } }}"
    [ -n "$subject" ] || subject=$(ls -1 {{library}} | head -1)
    echo "library: {{library}}   deck: $subject"
    for w in $({{bench_bin}} --list); do
        if {{bench_bin}} "$w" --library {{library}} --repeat 1 --quiet >/dev/null 2>&1; then
            {{bench_bin}} "$w" --library {{library}} --repeat 20
        else
            {{bench_bin}} "$w" --deck {{library}}/"$subject" --repeat {{repeat}}
        fi
    done

[doc('Export a hyperfine JSON baseline to compare a later build against.')]
[group('bench')]
[script]
baseline out="bench-baseline.json": build
    decks=$(ls -1 {{library}} | tr '\n' ',' | sed 's/,$//')
    hyperfine --warmup 3 --style basic --export-json {{out}} \
        -L deck "$decks" \
        -n 'load {deck}' \
        "{{bench_bin}} load --deck {{library}}/{deck} --repeat {{repeat}} --quiet"
    echo "wrote {{out}}"
