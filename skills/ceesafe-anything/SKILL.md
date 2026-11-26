---
name: ceesafe-anything
description: Port TypeScript/JavaScript, Python, C, and Go to CeeSafe with the ceesafe-anything CLI or MCP tools, from one hot function up to a whole app rewritten as a pure CeeSafe app - analyze, bridge one unit at a time, record the original's behavior, port, diff, bench, shadow, gate, and cutover. Use when asked to rewrite, port or migrate code or an app to CeeSafe, to plan such a migration, or to make slow code faster with CeeSafe.
---

# Migration to CeeSafe with ceesafe-anything

`ceesafe-anything` gives you ground truth; you write the CeeSafe. Port one unit at a time, even when
the goal is the whole app, and call a port done only when `gate` passes.

**Whole app to a pure CeeSafe app** (the user wants the app rewritten in CeeSafe, with the original
kept only as reference): read [whole-app.md](whole-app.md) first and follow it.

Tools: the `ceesafe-anything` CLI, or the same steps as MCP tools (`ceesafe-anything mcp`):
`analyze`, `bridge`, `record`, `build`, `diff`, `bench`, `shadow`, `cutover`, `gate`, and
`lookup_library`. Pair with `ceesafec mcp` (`ceesafe_check`, `ceesafe_api`, `ceesafe_test`,
`ceesafe_doc`) while writing `.cee` code. Run `ceesafe-anything doctor` if `ceesafec`, `cc`, or a
source runtime (`python3`, `node`, `go`) is missing.

## 1. Analyze & Plan

```sh
ceesafe-anything analyze <repo>
ceesafe-anything analyze <repo> --json
ceesafe-anything lookup <lang> <dep>
```

- Review discovered units, LOC, extracted function counts, and risk counts.
- Look up dependencies in CeeSafe (`lib/std`, `CeeSafeLibs`, `CeeSafe.Web`, `CeeSafe-index`).
  Never hand-roll a missing library silently.
- **Wait for the user's approval of the plan and of the first unit.**

## 2. Bridge & Record One Unit

```sh
ceesafe-anything bridge <repo> <unit> [out]    # scaffolds CeeSafe package; extracts C/Py/TS/Go signatures
ceesafe-anything record <pkg> [inputs.jsonl]   # records original behavior -> cases.jsonl
```

Write the port in `src/port.cee`; `src/bridge.cee`, `include/<lib>.h`, and `ceesafe-anything.json`
are generated automatically. Supported C-ABI / FFI parameter kinds:
- Scalars (`i32`, `u32`, `i64`, `u64`, `usize`, `f64`, `bool`, `void`)
- Read-only byte slices (`const uint8_t *ptr, size_t len` -> `Slice<u8>`)
- NUL-terminated C strings (`const char *s` -> `Slice<u8>` via `extern "C" fn strlen`)
- Mutable output buffers (`uint8_t *out, size_t len` -> `SliceMut<u8>` via `SliceMut.fromRaw`)

### CeeSafe Porting Rules

- **No panics across the FFI boundary**: CeeSafe has no `catch_unwind` — a panic (integer overflow,
  out-of-bounds index, or `!` on `null`/`Err`) immediately aborts the process with exit status 134.
  Use `Result<T, E>`, `checkedAdd`/`checkedSub`/`checkedMul`, and explicit bounds checks instead of `!`.
- **Explicit allocation failures**: `push`, `append`, `String.from`, `clone`, and `Box.create`
  return `Result<..., AllocError>`. Propagate with `?` (or map via `@from`) rather than `!`.
- **No macros, tuples, type aliases, or string interpolation**:
  - Group multiple return values in named `struct`s.
  - Build text with `String` (`append`, `appendNumber`, `appendHex`, `appendFixed`) or `TextWriter`.
  - Use `for` loops (or custom `Iterator<T>` structs with `fn next(borrow mut this) -> T?`) instead
    of `.map().filter().collect()`.
- **Equality & map keys**: Derive `@derive(Equal)`, `@derive(Equal, HashKey)`, or `@derive(OrderKey)`
  on structs/enums used with `==`, `HashMap`, or `OrderedMap`.
- **Cyclic graphs & shared state**: There is no `Rc<T>` or `Weak<T>`. Use `SlotMap<T>` + `Handle<T>`
  for graphs/trees, or `Arc<Mutex<T>>` when shared across threads.
- **No `unsafe` in `src/port.cee`**: Keep all raw pointer conversions isolated in generated `src/bridge.cee`.
  Any unavoidable `unsafe` must have a `// SAFETY:` comment right above it and a `/// Safety:` doc
  comment on the enclosing function.
- **Unit tests**: Add `@test` functions in `src/port.cee` and verify with `ceesafec test --sweep alloc`.

## 3. Prove It

```sh
ceesafe-anything build <pkg>                   # emits C and links out/lib<pkg>.dylib
ceesafe-anything diff <pkg>                    # must match N/N recorded cases
ceesafe-anything bench <pkg>                   # original vs CeeSafe timing table
```

A mismatch is a behavior difference: fix the port, never edit `cases.jsonl`.

## 4. Shadow, Gate & Cutover

```sh
ceesafe-anything shadow <pkg>                  # generates shadow/<pkg>_shadow.c + shadow/flags.txt
ceesafe-anything gate <pkg>                    # fmt, check --deny-warnings, test, stubs, unsafe, build, diff
ceesafe-anything cutover <pkg> [--apply]       # generates cutover shim + link flags (or swaps in with --apply)
```

`gate` verifies:
- `ceesafec fmt --check`
- `ceesafec check --deny-warnings`
- `ceesafec test`
- No `panic("unimplemented: ...")` stubs left in `src/port.cee`
- Every `unsafe` block has a `// SAFETY:` comment
- Built shared library links cleanly and matches 100% of recorded cases
