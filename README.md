# ceesafe-anything

Incremental, behavior-preserving migration of existing codebases to **CeeSafe** — written in pure CeeSafe (`ceesafe.toml` + `src/*.cee`).

## Build & Test

```sh
ceesafec fmt --check
ceesafec check --deny-warnings
ceesafec test
ceesafec build -o out/ceesafe-anything
```

## Commands

- `ceesafe-anything doctor` — verify `ceesafec`, `cc`, and optional source-language runtimes (`python3`, `node`, `go`)
- `ceesafe-anything setup [claude|codex|cursor|gemini|opencode]` — print MCP server configuration for your coding agent
- `ceesafe-anything analyze <root> [--json]` — map repository source files, LOC, exported functions, and porting risks
- `ceesafe-anything bridge <root> <unit> [out]` — scaffold a pure CeeSafe C-ABI library package (`ceesafe.toml`, `src/port.cee`, `src/bridge.cee`, `include/<name>.h`, `ceesafe-anything.json`) with automatic signature extraction for C, Python, TypeScript/JavaScript, and Go
- `ceesafe-anything refresh <pkg>` — regenerate `src/bridge.cee`, `include/<name>.h`, and `ceesafe-anything.json` after signature changes without touching `src/port.cee`
- `ceesafe-anything record <pkg> [inputs.jsonl]` — record original unit behavior into `cases.jsonl` (C via isolated `ctypes`, Python via `importlib`, TS/JS via Node ESM, Go via `go run`)
- `ceesafe-anything build <pkg>` — emit C via `ceesafec build --emit c` and link a C-ABI shared library with `cc`
- `ceesafe-anything diff <pkg>` — replay `cases.jsonl` against the built CeeSafe shared library (honoring optional `known-diffs.jsonl`) and write `.ceesafe-anything/diff.json`
- `ceesafe-anything bench <pkg>` — benchmark original vs CeeSafe shared library over `cases.jsonl` and write `.ceesafe-anything/bench.{json,txt}`
- `ceesafe-anything shadow <pkg>` — generate a drop-in C shadow wrapper (`shadow/<pkg>_shadow.c` + `shadow/flags.txt`) comparing original and CeeSafe side by side
- `ceesafe-anything cutover <pkg> [--apply]` — generate swap-in shim (`cutover/<pkg>_cutover.{c,py,mjs,go}` + `cutover/link.txt`) to route callers to the CeeSafe library (pass `--apply` to back up and replace the original unit)
- `ceesafe-anything gate <pkg>` — verify `ceesafec fmt --check`, `ceesafec check --deny-warnings`, `ceesafec test`, zero unimplemented stubs, `// SAFETY:` comments on every `unsafe`, clean shared-library build, and 100% `diff` match
- `ceesafe-anything report [dir]` — emit a Markdown migration status table across one or more ported packages
- `ceesafe-anything rules [root]` — print CeeSafe porting rules tailored to the languages detected in `<root>` (plus `ceesafe/RULES.md` if present)
- `ceesafe-anything profile <root> [-- <cmd...>]` — time a workload (wall-clock + peak RSS) and rank candidate units to port first
- `ceesafe-anything lift <dir> [out]` — assemble ported unit packages into a standalone pure CeeSafe executable app (`ceesafe.toml`, `src/<pkg>_port.cee`, `src/main.cee`, `LIFT_REPORT.md`)
- `ceesafe-anything app-record <dir> -- <cmd...>` — record whole-app CLI `status`/`stdout`/`stderr` into `app-cases.jsonl`
- `ceesafe-anything app-replay <dir> -- <cmd...>` — replay `app-cases.jsonl` against a lifted CeeSafe binary
- `ceesafe-anything lookup <lang> <dep>` — look up the CeeSafe library equivalent (`lib/std`, `CeeSafeLibs`, `CeeSafe.Web`, `CeeSafe-index`)
- `ceesafe-anything mcp` — run the stdio JSON-RPC 2.0 MCP server

## Supported C-ABI & FFI Types

`bridge`, `record`, `diff`, `bench`, `shadow`, and `cutover` automatically map:

- **Scalars**: `i32`, `u32`, `i64`, `u64`, `usize`, `f64`, `bool`, `void`
- **Byte slices**: `const uint8_t *ptr, size_t len` -> `Slice<u8>` via `Slice.fromRaw`
- **C strings**: `const char *s` -> `Slice<u8>` via `extern "C" fn strlen` + `Slice.fromRaw`
- **Mutable buffers**: `uint8_t *out, size_t len` -> `SliceMut<u8>` via `SliceMut.fromRaw`
- **C structs & enums**: `typedef struct` -> generated `src/types.cee` (`FfiCodec`, `fromBytes`) and `typedef enum` -> CeeSafe `enum` + `fromI32`

## Examples

- `examples/c_checksum/checksum.c` — single-function byte-slice checksum
- `examples/c_math/math_ops.c` — multi-function scalar and byte-slice math/hashing
- `examples/c_text/text_ops.c` — `const char *` (`cstr`), mutable `SliceMut<u8>` (`buffer`), and `Slice<u8>` (`bytes`) text utilities
- `examples/c_geom/geom.c` — C `typedef struct` pointer and `typedef enum` FFI
- `examples/py_stats/stats.py` — Python unit for `bridge` + `record`
- `examples/ts_slug/slug.mjs` — Node ESM unit for `bridge` + `record`
- `examples/go_math/math_ops.go` — Go unit for `bridge` + `record`

## Live Migration Benchmark (`nats-server` Go vs CeeSafe)

Twenty `nats-server` subsystems (`subject_match`, `wire_math`, `seqset`, `ldap_dn`, `proxyproto`, `conf_lex`, `proto_parser`, `cron`, `hdr_lookup`, `subject_transform`, `mqtt_topic`, `ws_frame`, `nats_archive`, `thw`, `gsl`, `stree`, `semver`, `msg_trace`, `raft_codec`, `js_codec`) were ported to pure CeeSafe (`0` `unsafe` in `port.cee`, **1,077/1,077** differential cases passing against Go, all 7 `gate` checks passing on every unit, and **6 live cgo cutovers** — `subject_match`, `wire_math`, `semver`, `ws_frame`, `hdr_lookup`, and `msg_trace` — passing `nats-server`'s own `go test -tags ceesafe_cutover ./server` suite).

Head-to-head in-process throughput on Apple Silicon (pure Go vs CeeSafe compiled via `ceesafec build --emit c` + `cc -O2`, timed in-process with `time.Now()` / `Clock.monotonicNanos()`, identical inputs and verified checksums; **19 of 19 workloads faster in CeeSafe**, aggregate `1966.67 ms` Go -> `554.58 ms` CeeSafe = **3.55x**):

| Subsystem | Source Unit | Workload | Go (ms) | CeeSafe -O2 (ms) | Speedup | Checksum |
|---|---|---|---:|---:|---:|---|
| `semver` | `server/util.go` | 200k SemVer 2.0.0 parses + compares | 149.69 | 8.46 | **17.69x** | match |
| `wire_math` | `server/util.go` | 5M `parseSize` + `mulSaturate` | 47.74 | 4.04 | **11.83x** | match |
| `raft_codec` | `server/raft.go` | 300k Raft `AppendEntry`/Resp/`PeerState` decodes | 32.20 | 3.04 | **10.59x** | match |
| `thw` | `server/thw/thw.go` | 500x (1000 THW adds + 500 removes) | 37.95 | 4.54 | **8.37x** | match |
| `subject_transform` | `server/subject_transform.go` | 50k multi-fn subject transforms | 162.10 | 20.13 | **8.05x** | match |
| `cron` | `server/cron.go` | 100k 6-field UTC cron evaluations | 100.63 | 15.14 | **6.65x** | match |
| `stree` | `server/stree/stree.go` | 300k `SubjectTree` wildcard matches | 61.82 | 11.40 | **5.42x** | match |
| `nats_archive` | `server/archive/archive.go` | 50k `NATSARC1` binary archive decodes | 69.73 | 14.37 | **4.85x** | match |
| `ldap_dn` | `internal/ldap/dn.go` | 100k `ParseDN` + `Equal` | 263.43 | 60.73 | **4.34x** | match |
| `msg_trace` | `server/msgtrace.go` | 200k `NATS/1.0` trace header scans | 47.75 | 11.35 | **4.21x** | match |
| `ws_frame` | `server/websocket.go` | 100k x 4KB WebSocket XOR masking (400MB) | 205.67 | 52.50 | **3.92x** | match |
| `gsl` | `server/gsl/gsl.go` | 600k `GenericSublist` wildcard matches | 103.61 | 29.81 | **3.48x** | match |
| `subject_match` | `server/sublist.go` | 1M `IsValidSubject` + 2M `SubjectsCollide` | 250.84 | 78.61 | **3.19x** | match |
| `conf_lex` | `conf/lex.go` | 25k full config lexes | 78.63 | 28.17 | **2.79x** | match |
| `seqset` | `server/avl/seqset.go` | 5000x (4096 inserts + delete + `SpanEnd`) | 121.39 | 47.99 | **2.53x** | match |
| `proto_parser` | `server/parser.go` | 50k pipelined wire batches (250k frames) | 30.28 | 11.96 | **2.53x** | match |
| `js_codec` | `server/jetstream_cluster.go` | 300k JetStream `StreamMsg`/Delivered/Ack decodes | 6.75 | 3.18 | **2.12x** | match |
| `hdr_lookup` | `server/client.go` | 1.5M `NATS/1.0` header lookups | 55.50 | 41.59 | **1.33x** | match |
| `mqtt_topic` | `server/mqtt.go` | 500k MQTT <-> NATS topic conversions | 140.97 | 107.58 | **1.31x** | match |
| **Total** | **19 workloads** | **In-process monotonic clock** | **1966.67** | **554.58** | **3.55x** | **19/19 match** |

## Agent Skills

CeeSafe porting instructions for coding agents live in `skills/ceesafe-anything/SKILL.md` and `skills/ceesafe-anything/whole-app.md`.
