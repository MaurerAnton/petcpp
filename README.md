# petcpp — CLI Snippet Manager (C++ port of pet)

A zero-dependency C++ port of [pet](https://github.com/knqyf263/pet) — manage command-line snippets with interactive search, template variables, and optional Gist sync.

## Why petcpp?

The original [pet](https://github.com/knqyf263/pet) requires Go plus dozens of modules. petcpp compiles with a single `make` using only C++17.

## Quick Start

```bash
make
./petcpp configure             # Initialize config
./petcpp new                   # Create a snippet interactively
./petcpp search "git log"      # Search snippets
./petcpp exec 1                # Execute snippet #1
```

## Features

- **new** — Create snippet with description, command, and tag
- **list** — List all snippets (`-1` for compact, `-c` for color)
- **search** — Fuzzy search by description/command
- **edit** — Edit snippet by index
- **exec** — Execute snippet (with `{{variable}}` template substitution)
- **configure** — Set up config file (Gist ID, editor)
- **sync** — Push/pull snippets to/from GitHub Gist
- Tag filtering (`-t TAG`)

## Snippet Format

Snippets are stored in TOML:
```toml
[[snippets]]
description = "Git log with count"
command = "git log --oneline -n {{count}}"
tag = ["git"]
```

## Build

```bash
make
```
Requires: GCC 10+ or Clang 12+, GNU Make
