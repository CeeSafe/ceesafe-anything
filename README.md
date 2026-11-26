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

Twenty-two `nats-server` subsystems (`subject_match`, `wire_math`, `seqset`, `ldap_dn`, `proxyproto`, `conf_lex`, `proto_parser`, `cron`, `hdr_lookup`, `subject_transform`, `mqtt_topic`, `ws_frame`, `nats_archive`, `thw`, `gsl`, `stree`, `semver`, `msg_trace`, `raft_codec`, `js_codec`, `consumer_state`, `stream_state`) were ported to pure CeeSafe (`0` `unsafe` in `port.cee`, **1,118/1,118** differential cases passing against Go, all 7 `gate` checks passing on every unit, and **6 live cgo cutovers** — `subject_match`, `wire_math`, `semver`, `ws_frame`, `hdr_lookup`, and `msg_trace` — passing `nats-server`'s own `go test -tags ceesafe_cutover ./server` suite).

Head-to-head in-process throughput on Apple Silicon (pure Go vs CeeSafe compiled via `ceesafec build --emit c` + `cc -O2`, timed in-process with `time.Now()` / `Clock.monotonicNanos()`, identical inputs and verified checksums; **21 of 21 workloads faster in CeeSafe**, aggregate `1681.97 ms` Go -> `538.81 ms` CeeSafe = **3.12x**):

| Subsystem | Source Unit | Workload | Go (ms) | CeeSafe -O2 (ms) | Speedup | Checksum |
|---|---|---|---:|---:|---:|---|
| `semver` | `server/util.go` | 200k SemVer 2.0.0 parses + compares | 146.94 | 7.83 | **18.77x** | match |
| `wire_math` | `server/util.go` | 5M `parseSize` + `mulSaturate` | 42.89 | 3.76 | **11.39x** | match |
| `raft_codec` | `server/raft.go` | 300k Raft `AppendEntry`/Resp/`PeerState` decodes | 29.74 | 2.96 | **10.04x** | match |
| `consumer_state` | `server/filestore.go` | 100k JetStream `ConsumerState` v2 decodes | 42.05 | 4.84 | **8.69x** | match |
| `thw` | `server/thw/thw.go` | 500x (1000 THW adds + 500 removes) | 35.96 | 4.44 | **8.10x** | match |
| `subject_transform` | `server/subject_transform.go` | 50k multi-fn subject transforms | 136.84 | 20.07 | **6.82x** | match |
| `stream_state` | `server/store.go` | 100k JetStream `StreamReplicatedState` v2 decodes | 30.84 | 5.30 | **5.82x** | match |
| `cron` | `server/cron.go` | 100k 6-field UTC cron evaluations | 90.74 | 16.01 | **5.67x** | match |
| `stree` | `server/stree/stree.go` | 300k `SubjectTree` wildcard matches | 56.32 | 10.87 | **5.18x** | match |
| `msg_trace` | `server/msgtrace.go` | 200k `NATS/1.0` trace header scans | 48.14 | 10.93 | **4.40x** | match |
| `ws_frame` | `server/websocket.go` | 100k x 4KB WebSocket XOR masking (400MB) | 193.53 | 46.57 | **4.16x** | match |
| `nats_archive` | `server/archive/archive.go` | 50k `NATSARC1` binary archive decodes | 55.88 | 13.66 | **4.09x** | match |
| `gsl` | `server/gsl/gsl.go` | 600k `GenericSublist` wildcard matches | 97.42 | 29.28 | **3.33x** | match |
| `subject_match` | `server/sublist.go` | 1M `IsValidSubject` + 2M `SubjectsCollide` | 241.18 | 74.32 | **3.25x** | match |
| `conf_lex` | `conf/lex.go` | 25k full config lexes | 77.50 | 27.23 | **2.85x** | match |
| `proto_parser` | `server/parser.go` | 50k pipelined wire batches (250k frames) | 26.09 | 11.58 | **2.25x** | match |
| `js_codec` | `server/jetstream_cluster.go` | 300k JetStream `StreamMsg`/Delivered/Ack decodes | 6.28 | 3.11 | **2.02x** | match |
| `ldap_dn` | `internal/ldap/dn.go` | 100k `ParseDN` + `Equal` | 81.30 | 53.47 | **1.52x** | match |
| `seqset` | `server/avl/seqset.go` | 5000x (4096 inserts + delete + `SpanEnd`) | 57.43 | 44.68 | **1.29x** | match |
| `hdr_lookup` | `server/client.go` | 1.5M `NATS/1.0` header lookups | 52.21 | 41.75 | **1.25x** | match |
| `mqtt_topic` | `server/mqtt.go` | 500k MQTT <-> NATS topic conversions | 132.69 | 106.16 | **1.25x** | match |
| **Total** | **21 workloads** | **In-process monotonic clock** | **1681.97** | **538.81** | **3.12x** | **21/21 match** |

## Agent Skills

CeeSafe porting instructions for coding agents live in `skills/ceesafe-anything/SKILL.md` and `skills/ceesafe-anything/whole-app.md`.
