---
name: dyalog-script
description: Execute Dyalog APL code via the `dyalogscript` interpreter. Use when running, testing, or evaluating APL in this repo (loading `mu.apln`, calling `mu.demo`, evaluating expressions). Adapted from xpqz/dev-environment with MicroUI-APL specifics.
---

# Dyalog-Script

`dyalogscript` runs APL from the command line without a full IDE session. On macOS it lives at `/usr/local/bin/dyalogscript` (installed via `brew install --cask dyalog`).

## Quick evaluation

One-liner:

```bash
echo "⎕←(+⌿÷≢)⍳100" | dyalogscript /dev/stdin
```

Multi-line via heredoc:

```bash
dyalogscript /dev/stdin <<'EAPL'
⎕←(+⌿÷≢)⍳100
⎕←'hello world'
EAPL
```

File-based for longer scripts:

```bash
dyalogscript path/to/script.apls
```

Give script files unique names, ensure a trailing newline, and remove ephemeral scripts after evaluation.

## Output and statement separation

1. Bare expressions produce no output. Use `⎕←expr` to print to stdout.
2. Use `⋄` to separate multiple statements on a single line when piping via `echo`.

## MicroUI-APL specifics

### Load the namespace

`mu.apln` is an exported namespace, not a workspace, so `)load mu.apln` does NOT work (it returns "is not a ws."). Use `⎕FIX`:

```apl
mu←⎕FIX'file://mu.apln'
```

The `file://` URI is resolved against the cwd of the `dyalogscript` process. Either run `dyalogscript` from the repo root (`/Users/.../MicroUI-APL`) so that `mu.apln`, `atlas.dcf`, `logo.dcf`, and `./fenster.dylib` all resolve, **or** pass an absolute path:

```apl
mu←⎕FIX'file:///abs/path/to/MicroUI-APL/mu.apln'
```

The same choice applies to `mu.fenster_dll` (relative `./fenster.dylib` vs. absolute path).

### Wire up the renderer

For anything that draws (notably `mu.demo`), point `mu.fenster_dll` at the prebuilt shared library. On macOS:

```apl
mu.fenster_dll←'./fenster.dylib'
```

If `fenster.dylib` does not exist in the repo root, build it first (see AGENTS.md for the macOS command).

### Run the demo

```bash
dyalogscript /dev/stdin <<'EAPL'
mu←⎕FIX'file://mu.apln'
mu.fenster_dll←'./fenster.dylib'
mu.demo
EAPL
```

`mu.demo` opens a 1024x768 native window and runs an event loop until the window is closed. dyalogscript will block until that happens, so run in a background job (or another terminal) when you need to keep iterating.

### Headless evaluation against the namespace

For introspection or pure-APL tests that do not require the renderer, skip `fenster_dll` entirely:

```bash
dyalogscript /dev/stdin <<'EAPL'
mu←⎕FIX'file://mu.apln'
⎕←'fns: ',≢mu.⎕NL 3
⎕←10↑mu.⎕NL 3
EAPL
```

## Examples

User: Evaluate `(+⌿÷≢)⍳100`

```bash
$ echo "⎕←(+⌿÷≢)⍳100" | dyalogscript /dev/stdin
50.5
```

User: How many functions does mu expose?

```bash
$ dyalogscript /dev/stdin <<'EAPL'
mu←⎕FIX'file://mu.apln'
⎕←≢mu.⎕NL 3
EAPL
90
```
