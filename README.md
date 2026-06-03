# petcpp — CLI Snippet Manager (C++ port of pet)

A zero-dependency C++ port of [pet](https://github.com/knqyf263/pet) — manage your command-line snippets with interactive search and parameter substitution.

## Why petcpp?

The original [pet](https://github.com/knqyf263/pet) requires the Go toolchain plus dozens of modules. petcpp compiles with a single `make` using only C++17 and standard Linux headers.

## Quick Start

```bash
make
./petcpp search
./petcpp new "git log --oneline -n {{count}}"
```

## Features

- Interactive fuzzy search of snippets
- Parameter placeholders with auto-prompting (e.g., `{{name}}`)
- Add, edit, delete, and list snippets
- TOML-based snippet storage
- Exec mode to run snippets directly
- Sync snippets via Gist (optional)

## Build

```bash
make
```
Requires: GCC 10+ or Clang 12+, GNU Make
