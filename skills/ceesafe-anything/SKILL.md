---
name: ceesafe-anything
description: Port TypeScript/JavaScript, Python, C, Go, Ruby, Java, Kotlin, PHP, C# and PowerShell to CeeSafe with the ceesafe-anything CLI or MCP tools, from one hot function up to a whole app rewritten as a pure CeeSafe app - profile, plan, bridge one unit at a time, record the original's behavior, port, diff, bench, shadow and gate. Use when asked to rewrite, port or migrate code or an app to CeeSafe, to plan such a migration, or to make slow code faster with CeeSafe.
---

# Migration to CeeSafe with ceesafe-anything

`ceesafe-anything` gives you ground truth; you write the CeeSafe. Port one unit at a time, even when
the goal is the whole app, and call a port done only when `gate` passes.

**Whole app to a pure CeeSafe app** (the user wants the app rewritten in CeeSafe, with the original
kept only as reference): read [whole-app.md](whole-app.md) first and follow it.

Tools: the `ceesafe-anything` CLI, or the same steps as MCP tools (`ceesafe-anything mcp`): `go`,
`profile`, `analyze`, `bridge`, `refresh`, `record`, `build`, `diff`, `bench`, `shadow`, `gate`,
`enable`, `disable`, `report`, `rules`, and for a whole app `app_record`, `app_replay`,
`app_front_door`, `app_wire`, `app_lift`, `app_drift`. Pair with `ceesafec mcp` (`ceesafe_check`,
`ceesafe_api`, `ceesafe_test`, `ceesafe_doc`) while writing `.cee` code. Run `ceesafe-anything doctor`
if `ceesafec`, `cc`, or a source runtime is missing.

## 0. Profile & 1. Plan

```sh
ceesafe-anything profile <repo> -- node bench.mjs
ceesafe-anything analyze <repo> -o plan.md
ceesafe-anything analyze <repo> --json
```

- Present `start_here` units and the wave order.
- Call out `keep` units, `blocked` units, and dependencies mapped as `none` or `unknown` in CeeSafe
  (`lib/std`, `CeeSafeLibs`, `CeeSafe.Web`, `CeeSafe-index`). Never hand-roll a missing library silently.
- **Wait for the user's approval of the plan and of the first unit.**

## 2. Port one unit

```sh
ceesafe-anything bridge <repo> <unit-id>       # CeeSafe package in <repo>/ceesafe/<name>; never overwrites
ceesafe-anything record <pkg>                  # record original behavior -> cases.jsonl
```

Write the port in `src/port.cee`; `src/bridge.cee`, `include/<lib>.h`, and language wrappers are
generated. After changing a signature in `port.cee`, run `ceesafe-anything refresh <pkg>`.

Before writing code, run `ceesafe-anything rules <repo>` and follow every rule it lists.

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
  Any unavoidable `unsafe` must have a `// SAFETY:` comment right above it.
- **Unit tests**: Add `@test` functions in `src/port.cee` and verify with `ceesafec test --sweep alloc`.

## 3. Prove it

```sh
ceesafe-anything build <pkg>
ceesafe-anything diff <pkg>                    # must be N/N
ceesafe-anything bench <pkg>                   # original vs CeeSafe, bridge included
```

A mismatch is a behavior difference: fix the port, never edit `cases.jsonl`. For approved
intentional differences, add entries to `<pkg>/known-diffs.jsonl`.

## 4. Ship safely & 5. Gate

```sh
ceesafe-anything shadow <pkg>                  # run original and CeeSafe side by side
ceesafe-anything gate <pkg>                    # fmt, check --deny-warnings, test, unsafe, ported, diff
```

`gate` verifies:
- `ceesafec fmt --check`
- `ceesafec check --deny-warnings`
- `ceesafec test` (and leak checking)
- No unexplained `unsafe` blocks
- No `panic("unimplemented: ...")` stubs left in `src/port.cee`
- Built shared library exports every symbol and matches 100% of recorded cases
