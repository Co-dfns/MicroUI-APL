# MicroUI-APL coding patterns

This file documents the *internal coding conventions* used in `mu.apln` and its C
binding. The `README.md` documents the public API; this file documents the idioms
a reader needs in order to follow, extend, or modify the existing source.

The whole library is one Dyalog APL namespace (`:Namespace mu` at `mu.apln:23`,
~1200 lines). Conventions here are stable and internally consistent; new code
should match them rather than introduce a parallel style.

## 1. Namespace and file layout

- Single namespace, single file: `mu.apln:23` opens `:Namespace mu`. No nested
  namespaces.
- `⎕IO ⎕CT←0` is set once at the top (`mu.apln:25`). All array indexing is 0-origin.
- Section banners are full-width comment boxes (look for `⍝⍝⍝⍝⍝...⍝⍝⍝`). Major
  sections begin at lines 27 (sizes), 44 (constants), 80 (field offsets), 100
  (state / `init`), 190 (utilities), 209 (core), 279 (containers), 333 (pools),
  354 (input), 366 (command list), 412 (layout), 490 (controls), 772 (ffi
  binding scaffolding), 875 (demo).
- Place new functions inside the section that matches their concern. Do not
  introduce new top-level structure.

## 2. Function-style rule: dfn vs tradfn

Both forms are used deliberately.

- **Dfn** (`name←{...}`) for pure single-expression helpers, especially when the
  body is a conditional ladder using `expr:` early-return. Examples:
  - `assert` (`mu.apln:194`)
  - `expand_rect` (`mu.apln:195`)
  - `intersect_rects` (`mu.apln:196-201`)
  - `get_id` (`mu.apln:247-250`)
  - `bring_to_front` (`mu.apln:277`)
- **Tradfn** (`∇...∇`) when you need any of: named labels, multiple locals,
  multi-statement bodies that don't compose cleanly, or optional args probed
  with `⎕NC`. Examples: `init` (`mu.apln:104`), `end` (`mu.apln:220`),
  `next_command` (`mu.apln:372`), `begin_window` (`mu.apln:697`),
  `begin_render` (`mu.apln:790`), `do_render` (`mu.apln:823`),
  `demo` (`mu.apln:879`).
- Dfn helpers without a meaningful return value end in `_←0` or `_←⍵`. This is
  the convention for "no shy result; suppress this expression". See
  `push_clip_rect` (`mu.apln:258-261`), `bring_to_front` (`mu.apln:277`).

## 3. Naming

- Functions and variables: `snake_case` (e.g. `push_clip_rect`, `get_layout`,
  `mouse_delta`).
- Constants: `SCREAMING_SNAKE_CASE` (e.g. `COMMANDLIST_SIZE`, `OPT_NOSCROLL`,
  `KEY_RETURN`).
- Empty-struct sentinels: `MT_*` (lines 71-78). `MT_` reads as "empty" - one
  prototype value per struct shape.
- Field-offset constants: lower-snake with a struct prefix - `cnt_*` for
  containers (lines 84-87), `lyt_*` for layout (lines 89-94), `pool_*` for pool
  rows (line 96), `jump_dst` for command-jump (line 98).
- Stack pointers: `<name>_idx` paired with the stack itself (`command_list` /
  `command_list_idx`, etc.).

## 4. Struct-as-flat-vector + sentinel + field-offset constants

There is no record type. Each "struct" is a flat numeric vector; field access is
by name-as-index.

- The empty value is `MT_<NAME>` (lines 71-78). `MT_CONTAINER` is built by
  concatenating `MT_RECT`, `MT_VEC`, etc. (line 78).
- The field offsets are top-level constants (lines 84-98). They are scalars or
  small index vectors.
- A pool of rows is a `SIZE × ≢MT_<NAME>` matrix, prefilled with the sentinel:
  `containers←CONTAINERPOOL_SIZE(≢MT_CONTAINER)⍴MT_CONTAINER` (`mu.apln:173`).
- To add a new field to a struct: extend `MT_<NAME>`, add a new offset constant,
  re-allocate the pool in `init`, and update any `containers[idx;...]` access
  sites.

## 5. Stack idiom

A stack is a preallocated container plus an index counter that names the next
free slot.

- Allocation in `init` (`mu.apln:152-169`):

  ```apl
  command_list      ←COMMANDLIST_SIZE⍴⊂⍬
  command_list_idx  ←0
  clip_stack        ←CLIPSTACK_SIZE 4⍴⍬
  clip_stack_idx    ←0
  layout_stack      ←LAYOUTSTACK_SIZE(≢MT_LAYOUT)⍴MT_LAYOUT
  layout_stack_idx  ←0
  ```

- Push: write at `<name>[<name>_idx;...]`, then `<name>_idx+←1`.
- Pop: `<name>_idx-←1`. The data row is left in place; a future push overwrites
  it.
- Top-of-stack: `<name>[<name>_idx-1;...]`.
- Stacks never grow. Sizes are set in the constants block (lines 31-39); a
  full stack is a logic error.

## 6. Common idioms

- **Conditional apply** `f⍣bool⊢arg` - replaces `if` for a side-effect or value
  transformation. See `bring_to_front⍣t⊢next_hover_root` (`mu.apln:230`),
  `pool_update_container⍣(...)⊢idx` (`mu.apln:296`), `cnt scrollbars⍣(~opt[OPT_NOSCROLL])⊢body`
  (`mu.apln:303`), `draw_frame⍣(~opt[OPT_NOFRAME])⊢rect color_windowbg`
  (`mu.apln:706`). Used 20+ times throughout. Prefer this over a tradfn `→` branch
  when the alternative is "do nothing".

- **`@`-at to set fields on a struct value** - `value@FIELD⊢struct` returns a
  new struct with `FIELD` overwritten. See `1@cnt_open⊢MT_CONTAINER`
  (`mu.apln:298`), `style_title_height@3⊢rect` (`mu.apln:708`),
  `1@(OPT_POPUP OPT_AUTOSIZE OPT_NORESIZE OPT_NOSCROLL OPT_NOTITLE OPT_CLOSED)⊢MT_OPT`
  (`mu.apln:750`). Multi-field assignment is just multi-element index on the
  left.

- **Sentinel-guard `expr:` line ending in dfns** - early return. The pattern is
  `cond:value` ("if cond, return value"). See `check_clip` (`mu.apln:271-275`):
  multiple guards, fall-through to default `CLIP_PART`.

- **Optional left arg in dfn** - `⍺←DEFAULT` at the top of the body. See
  `get_container_id` (`mu.apln:295` - `⍺←MT_OPT ⋄ opt←⍺`),
  `push_container_body` (`mu.apln:302`).

- **Optional left arg in tradfn** - `→LABEL⍴⍨0≠⎕NC'name' ⋄ name←DEFAULT`. The
  guard branches over the default-assign when the caller supplied the arg. See
  `begin_window` (`mu.apln:698`) and `do_render` (`mu.apln:826`).

## 7. Bitvector flags, not bitmasks

The microui C original uses bitmasks (`MU_OPT_ALIGNCENTER` is a bit position).
This port uses **boolean vectors indexed by named offsets**.

- `MT_OPT` is `13⍴0` (`mu.apln:72`); `OPT_*` constants (lines 57-60) are the
  indexes into it.
- Same pattern: `MT_RES` indexed by `RES_*`, `MT_MOUSE` indexed by `MOUSE_*`,
  `MT_KEY` indexed by `KEY_*`.
- Test a flag with `opt[OPT_NOSCROLL]`; set with `1@OPT_NOSCROLL⊢opt`; clear with
  `0@OPT_NOSCROLL⊢opt`.
- A new flag costs: extend `MT_OPT` length (line 72), add the index constant
  (lines 57-60).

## 8. Control flow with `→`

In tradfns, `→` is the only branching primitive. The repeating forms:

- `→0` - exit the function.
- `→0⍴⍨cond` - exit if `cond` is true (`mu.apln:238`, 700).
- `→LABEL⍴⍨cond` - branch to `LABEL` if `cond` is true (`mu.apln:698`, 723,
  728, 730).
- `→LABEL[idx]` - computed-goto: a vector of label expressions, indexed by a
  small integer (`mu.apln:830`: `→BAD BAD CLIP RECT TEXT ICON IMAGE[⊃cmd]`).
- `→COND_TRUE COND_FALSE[bool]` (or the `⊃` form) - two-way branch
  (`mu.apln:804`, 895, 1012).

Labels are bare identifiers followed by `:`. Convention: `SCREAMING_SNAKE` for
labels that act as section markers (`FONT_BEGIN`, `END`, `BAD`, `RESIZE_TO_CONTENT`).

## 9. C / FFI boundary

There is one `⎕NA` site, in `begin_render` at `mu.apln:793-797`:

```apl
'fenster_open'  ⎕NA 'P ',fenster_dll,'|fenster_open_apl <0C[] I4 I4 <I2[]'
'fenster_loop'  ⎕NA 'P ',fenster_dll,'|fenster_loop_apl P <PP >I4[256] >I4 >I4 >I4 >I4'
'fenster_close' ⎕NA '',fenster_dll,'|fenster_close_apl P'
'fenster_sleep' ⎕NA '',fenster_dll,'|fenster_sleep I4'
'fenster_time'  ⎕NA 'I8 ',fenster_dll,'|fenster_time'
```

- `fenster_dll` is settable per session (defaults to `'fenster.dll'`); the
  current platform decides the actual filename (`fenster.dll`, `fenster.so`,
  `fenster.dylib`).
- All wrappers Dyalog calls live in `fenster.c` (the `_apl` suffix is an
  application convention). Type codes: `P`=pocket pointer, `<` input, `>`
  output, `I4`/`I8`/`I2` integer width, `0C[]` zero-terminated string,
  `<I2[]` input uint16 buffer.
- Adding a new C export: implement and `FENSTER_API`-mark it in `fenster.c`,
  then add a corresponding `⎕NA` line in this block. Both files change together.
- The C side has a stub `DyalogGetInterpreterFunctions` (`fenster.c:28-32`)
  that always returns 0 - leave it.

## 10. Component (`.dcf`) files

`.dcf` files are Dyalog component files; each component is one slot, addressed
by integer index. The two used here are pre-generated assets.

- `atlas.dcf`:
  - Slot `0` - the glyph image (the bitmap atlas).
  - Slot `1` - the glyph bounding boxes (one per glyph).
  - Read at `mu.apln:811-814` in `begin_render`. The default `style_font←0 1`
    (`mu.apln:806`) names this slot pair.
- `logo.dcf`:
  - Slot `1` - demo logo image, read by the demo (`mu.apln:879+`).

There is no script in the repo to regenerate either file; they are committed
binary artifacts. Treat them as inputs.

## 11. Renderer pipeline

- `begin_render` (`mu.apln:790`) ties the framebuffer, binds the FFI, calls
  `init`, opens the fenster window, and returns `(struct state)`.
- `do_render` (`mu.apln:823`) walks the command list once per frame, blits
  rectangles / text / icons / images / clip changes into the framebuffer, then
  calls `fenster_loop` to flush and read input.
- `end_render` (`mu.apln:870`) closes the window.

Important: **`begin_render` calls `init` for you** (`mu.apln:799`). Code that
implements its own renderer (skipping `begin_render`) must call `init`
explicitly before any `begin`/`end` cycle, otherwise the stacks and pools are
empty/uninitialised.

## 12. Demo conventions

`mu.demo` (`mu.apln:879`) drives one full frame per loop. Convention for
collapsible per-window sections is a label pair, named by which side they jump
to:

- `*_OPEN` / `*_CLOSED` (e.g. `FRAME_RATE_OPEN` / `FRAME_RATE_CLOSED`,
  `STYLE_WIN_OPEN` / `STYLE_WIN_CLOSED`, `LOG_WIN_OPEN` / `LOG_WIN_CLOSED`,
  `TEST_WIN_OPEN` / `TEST_WIN_CLOSED`).
- `*_BEGIN` / `*_END` (e.g. `TW_INFO_BEGIN` / `TW_INFO_END`,
  `LB_BEGIN` / `LB_END`).

Pick one pair shape per window/section and follow the existing structure -
don't mix them within a single window.

## 13. Verifying changes

There is no automated test suite. The current verification surface is these
recipes; run them after editing `mu.apln` or `fenster.c`.

**Build the renderer (macOS):**

```bash
clang -dynamiclib -O2 -fdeclspec -o fenster.dylib fenster.c \
  -framework Cocoa -framework AudioToolbox
```

Should produce a Mach-O dylib with no errors. Warnings about `__declspec` on
clang are expected and harmless. Windows and Linux recipes live in `README.md`.

**Headless namespace load + function count:**

```bash
dyalogscript /dev/stdin <<'EAPL'
mu←⎕FIX'file://mu.apln'
⎕←≢mu.⎕NL 3
EAPL
```

Should print `90`. A different number means a function was added or removed -
reconcile against the API list in `README.md`.

**Smoke-eval the demo entry point:**

```bash
dyalogscript /dev/stdin <<'EAPL'
mu←⎕FIX'file://mu.apln'
mu.fenster_dll←'./fenster.dylib'
mu.demo
EAPL
```

Should open a 1024x768 native window titled "MicroUI Demo". Closing the window
exits `dyalogscript`.

**Inline `assert` invariants:** `mu.apln` calls `assert` at lines 222 (in `end`)
and 384 (in `draw_rect`). Any change that triggers either is a regression -
they encode invariants that should always hold.

## What's not documented yet

These belong to the conventions surface but are not currently captured:

- No automated test suite. Adding one (Dyalog `Tester2`, `]TEST`, or
  hand-rolled) is a separate effort.
- No CI. Depends on tests existing first.
- No regeneration pipeline for `atlas.dcf` or `logo.dcf`.
- No "how to add a new control" walkthrough. The existing controls (`button`,
  `checkbox`, `slider`, `textbox`, `header`, `begin_treenode`, `begin_window`)
  are the templates.
