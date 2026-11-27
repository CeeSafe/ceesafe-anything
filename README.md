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

Forty `nats-server` subsystems (`subject_match`, `wire_math`, `seqset`, `ldap_dn`, `proxyproto`, `conf_lex`, `proto_parser`, `cron`, `hdr_lookup`, `subject_transform`, `mqtt_topic`, `ws_frame`, `nats_archive`, `thw`, `gsl`, `stree`, `semver`, `msg_trace`, `raft_codec`, `js_codec`, `consumer_state`, `stream_state`, `msg_ttl`, `ack_reply`, `proto_wire`, `mqtt_wire`, `sublist_match`, `js_usage_codec`, `store_policy`, `gw_reply`, `reply_guard`, `util_num`, `filestore_meta`, `comp_negotiate`, `js_batch_codec`, `route_auth_util`, `js_api_meta`, `host_path_util`, `subject_class`, `sys_hash`) were ported to pure CeeSafe (`0` `unsafe` in `port.cee`, **2,050/2,050** differential cases passing against Go, all 7 `gate` checks passing on every unit, **40/40** lifted `@test` suites passing, and **24 live cgo cutovers** — `subject_match`, `wire_math`, `semver`, `ws_frame`, `hdr_lookup`, `msg_trace`, `msg_ttl`, `ack_reply`, `proto_wire`, `mqtt_wire`, `sublist_match`, `js_usage_codec`, `store_policy`, `gw_reply`, `reply_guard`, `util_num`, `filestore_meta`, `comp_negotiate`, `js_batch_codec`, `route_auth_util`, `js_api_meta`, `host_path_util`, `subject_class`, and `sys_hash` — passing `nats-server`'s own `go test -tags ceesafe_cutover ./server` suite).

Head-to-head in-process throughput on Apple Silicon (pure Go vs CeeSafe compiled via `ceesafec build --emit c` + `cc -O2`, timed in-process with `time.Now()` / `Clock.monotonicNanos()`, identical inputs and verified checksums; **39/39 checksums matching**, aggregate `2173.28 ms` Go -> `754.42 ms` CeeSafe = **2.88x**):

| Subsystem | Source Unit | Workload | Go (ms) | CeeSafe -O2 (ms) | Speedup | Checksum |
|---|---|---|---:|---:|---:|---|
| `comp_negotiate` | `server/server.go` | 300k S2 compression negotiation, RTT & option validation | 76.57 | 0.53 | **145.5x** | match |
| `route_auth_util` | `server/auth.go` | 200k bcrypt, FNV-1a route pool, TLS version & tracked reply | 64.96 | 1.39 | **46.82x** | match |
| `semver` | `server/util.go` | 200k SemVer 2.0.0 parses + compares | 148.10 | 7.96 | **18.60x** | match |
| `store_policy` | `server/store.go` | 200k run-length len + 1M JetStream policy JSON parses | 9.02 | 0.50 | **18.10x** | match |
| `js_batch_codec` | `server/jetstream_cluster.go` | 300k control hdr + skip update + batch message decodes | 5.22 | 0.34 | **15.45x** | match |
| `wire_math` | `server/util.go` | 5M `parseSize` + `mulSaturate` | 45.67 | 3.81 | **11.99x** | match |
| `raft_codec` | `server/raft.go` | 300k Raft `AppendEntry`/Resp/`PeerState` decodes | 29.70 | 2.92 | **10.19x** | match |
| `thw` | `server/thw/thw.go` | 500x (1000 THW adds + 500 removes) | 37.08 | 4.14 | **8.96x** | match |
| `consumer_state` | `server/filestore.go` | 100k JetStream `ConsumerState` v2 decodes | 37.18 | 4.98 | **7.47x** | match |
| `subject_transform` | `server/subject_transform.go` | 50k multi-fn subject transforms | 141.58 | 19.92 | **7.11x** | match |
| `stream_state` | `server/store.go` | 100k JetStream `StreamReplicatedState` v2 decodes | 33.62 | 5.27 | **6.38x** | match |
| `cron` | `server/cron.go` | 100k 6-field UTC cron evaluations | 92.30 | 14.85 | **6.22x** | match |
| `stree` | `server/stree/stree.go` | 300k `SubjectTree` wildcard matches | 61.05 | 10.66 | **5.73x** | match |
| `filestore_meta` | `server/filestore.go` | 300k block/record sizing, TTL/schedule scan & compression JSON | 61.41 | 11.70 | **5.25x** | match |
| `nats_archive` | `server/archive/archive.go` | 50k `NATSARC1` binary archive decodes | 61.21 | 13.50 | **4.53x** | match |
| `mqtt_wire` | `server/mqtt.go` | 400k MQTT header/varint checks + 400k SparkplugB parses | 26.16 | 5.95 | **4.40x** | match |
| `msg_ttl` | `server/stream.go` | 400k JetStream `Nats-TTL` duration parses | 13.77 | 3.17 | **4.34x** | match |
| `msg_trace` | `server/msgtrace.go` | 200k `NATS/1.0` trace header scans | 46.30 | 10.74 | **4.31x** | match |
| `ws_frame` | `server/websocket.go` | 100k x 4KB WebSocket XOR masking (400MB) | 199.77 | 46.81 | **4.27x** | match |
| `gsl` | `server/gsl/gsl.go` | 600k `GenericSublist` wildcard matches | 99.47 | 27.95 | **3.56x** | match |
| `util_num` | `server/util.go` | 200k `parseUint64` + `addSaturate` + `comma` formats | 21.38 | 6.53 | **3.27x** | match |
| `subject_match` | `server/sublist.go` | 1M `IsValidSubject` + 2M `SubjectsCollide` | 244.76 | 76.56 | **3.20x** | match |
| `conf_lex` | `conf/lex.go` | 25k full config lexes | 77.66 | 27.61 | **2.81x** | match |
| `gw_reply` | `server/gateway.go` | 400k gateway routed reply + prefix checks | 1.32 | 0.53 | **2.49x** | match |
| `proto_parser` | `server/parser.go` | 50k pipelined wire batches (250k frames) | 26.27 | 11.72 | **2.24x** | match |
| `proto_wire` | `server/proto.go` | 800k protobuf wire scans + 200k varint encodes | 8.69 | 3.94 | **2.20x** | match |
| `js_codec` | `server/jetstream_cluster.go` | 300k JetStream `StreamMsg`/Delivered/Ack decodes | 6.11 | 3.08 | **1.98x** | match |
| `sublist_match` | `server/sublist.go` | 400k `matchLiteral` + 200k `subsetMatch` + 200k `tokenAt` | 18.93 | 10.99 | **1.72x** | match |
| `ack_reply` | `server/consumer.go` | 300k `$JS.ACK` v1/v2 + `Stream-Source` parses | 37.36 | 23.93 | **1.56x** | match |
| `ldap_dn` | `internal/ldap/dn.go` | 100k `ParseDN` + `Equal` | 80.76 | 54.34 | **1.49x** | match |
| `hdr_lookup` | `server/client.go` | 1.5M `NATS/1.0` header lookups | 54.14 | 41.01 | **1.32x** | match |
| `js_usage_codec` | `server/jetstream.go` | 200k 3-tier binary usage decodes + 400k name checks | 9.30 | 7.13 | **1.30x** | match |
| `seqset` | `server/avl/seqset.go` | 5000x (4096 inserts + delete + `SpanEnd`) | 58.18 | 45.23 | **1.29x** | match |
| `mqtt_topic` | `server/mqtt.go` | 500k MQTT <-> NATS topic conversions | 136.45 | 106.51 | **1.28x** | match |
| `reply_guard` | `server/client.go` | 300k reserved reply/dest + `$JS.ACK` checks | 21.25 | 17.57 | **1.21x** | match |
| `host_path_util` | `server/util.go` | 200k `parseHostPort` + `normalizeBasePath` + `isReservedAccount` | 31.23 | 25.90 | **1.21x** | match |
| `js_api_meta` | `server/jetstream_versioning.go` | 300k API level header check + retry backoff + S2 comp metadata | 9.30 | 9.18 | **1.01x** | match |
| `subject_class` | `server/sublist.go` | 400k wildcard/literal + JS pull req + internal client checks | 28.28 | 33.61 | **0.84x** | match |
| `sys_hash` | `server/events.go` | 100k SHA-256 base-62 system hash (scalar software vs ARMv8 crypto) + tier check | 11.77 | 51.96 | **0.23x** | match |
| **Total** | **39 workloads** | **In-process monotonic clock** | **2173.28** | **754.42** | **2.88x** | **39/39 match** |

## Agent Skills

CeeSafe porting instructions for coding agents live in `skills/ceesafe-anything/SKILL.md` and `skills/ceesafe-anything/whole-app.md`.
