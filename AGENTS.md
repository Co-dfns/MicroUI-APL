# AGENTS.md

Guidance for AI agents (Claude Code, etc.) working on MicroUI-APL.

For coding conventions and internal idioms, see [`docs/PATTERNS.md`](docs/PATTERNS.md).
This file is the entry point and points to deeper references; `PATTERNS.md` is the
detail.

## What this project is

Pure-APL reimplementation of [rxi/microui](https://github.com/rxi/microui), an immediate-mode GUI library, plus an APL binding to [zserge/fenster](https://github.com/zserge/fenster) for native rendering. The whole thing is a single namespace: `mu.apln`. See `README.md` for the full API.

## Running APL code

Use the `dyalog-script` skill in `.claude/skills/dyalog-script/`. It documents how to drive `dyalogscript` for evaluating expressions, loading the namespace, and launching the demo.

Key reminders:

- `)load mu.apln` does **not** work (`.apln` is a namespace export, not a workspace). Use `mu←⎕FIX'file://mu.apln'`.
- Bare expressions print nothing. Always use `⎕←expr`.
- Run `dyalogscript` from the repo root so relative paths (`mu.apln`, `atlas.dcf`, `logo.dcf`, `./fenster.dylib`) resolve.

## Building the renderer

All three platforms in one place (this is a Mac, so macOS is primary):

```bash
# macOS
clang -dynamiclib -O2 -fdeclspec -o fenster.dylib fenster.c \
  -framework Cocoa -framework AudioToolbox

# Windows
cl /LD fenster.c user32.lib gdi32.lib

# Linux
cc -shared -o fenster.so fenster.c -lX11 -lasound
```

Notes:
- `-fdeclspec` is required on macOS clang because `fenster.c` uses `__declspec(dllexport)` (a Windows-ism). clang treats it as a no-op with a warning, which is harmless.
- Frameworks on macOS: `Cocoa` for the window, `AudioToolbox` for `fenster_audio.h`.
- Output filename varies (`fenster.dll`, `fenster.so`, `fenster.dylib`); `mu.fenster_dll` defaults to `'fenster.dll'`, so override per session: `mu.fenster_dll←'./fenster.dylib'` on Mac, `mu.fenster_dll←'./fenster.so'` on Linux.

## Running the demo

```bash
dyalogscript /dev/stdin <<'EAPL'
mu←⎕FIX'file://mu.apln'
mu.fenster_dll←'./fenster.dylib'
mu.demo
EAPL
```

Opens a 1024x768 native window. `dyalogscript` blocks until the window is closed.

## Renderer entry points

The fenster renderer exposes a three-call contract used by `mu.demo` and any
custom render loop:

- `{font} mu.begin_render (title width height)` (`mu.apln:790`) - binds the FFI,
  opens the window, **calls `mu.init` for you**, returns `(struct state)`.
- `{fb} mu.do_render (struct width height state)` (`mu.apln:823`) - one frame:
  walks the command list, blits into the framebuffer, polls input. Loop
  condition is the shy first element of its result.
- `mu.end_render struct` (`mu.apln:870`) - closes the window.

A custom renderer that does **not** call `mu.begin_render` must call `mu.init`
itself before any `mu.begin` / `mu.end` cycle. See `docs/PATTERNS.md` section 11.

## Component files

Pre-generated binary assets read at render time:

- `atlas.dcf` - slot `[0]` is the glyph image, slot `[1]` is the glyph bounding
  boxes. The default `style_font←0 1` (`mu.apln:806`) names this slot pair; if
  you swap the atlas, keep the slot ordering.
- `logo.dcf` - slot `[1]` is the demo logo image, read by `mu.demo`.

There is no in-repo script to regenerate either file. Treat them as inputs.

## Dyalog reference docs

When you need APL language reference (system functions, control words, glyph semantics):

- Prefer the cloned doc/reference/ tree if relevant material exists there (it covers Dyalog GUI/COM/grid features).
- For language-level lookups (`⎕FIX`, `⎕FX`, `⎕NA`, `⎕FTIE`, etc.) the local tree is incomplete; fall back to web docs at `https://help.dyalog.com/`.

`docsearch` from `xpqz/dev-environment` is **not** installed here.

## Don't

- Don't add backwards-compat shims for the original Windows-only build (Windows users still use the README's `cl /LD` recipe).
- Don't commit `fenster.dylib`, `fenster.so`, or `fenster.dll` - they're per-machine build artifacts (`.gitignore` already excludes all three).
- Don't introduce new file dependencies for the demo without updating both `mu.demo` and this file.
