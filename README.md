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

- `ceesafe-anything doctor` — verify `ceesafec`, `cc`, and optional source-language runtimes (`python3`, `node`, `go`, `rustc`)
- `ceesafe-anything setup [claude|codex|cursor|gemini|opencode]` — print MCP server configuration for your coding agent
- `ceesafe-anything analyze <root> [--json]` — map repository source files, LOC, exported functions, and porting risks
- `ceesafe-anything bridge <root> <unit> [out]` — scaffold a pure CeeSafe C-ABI library package (`ceesafe.toml`, `src/port.cee`, `src/bridge.cee`, `include/<name>.h`, `ceesafe-anything.json`) with automatic signature extraction for C, Python, TypeScript/JavaScript, Go, and Rust
- `ceesafe-anything refresh <pkg>` — regenerate `src/bridge.cee`, `include/<name>.h`, and `ceesafe-anything.json` after signature changes without touching `src/port.cee`
- `ceesafe-anything record <pkg> [inputs.jsonl]` — record original unit behavior into `cases.jsonl` (C via isolated `ctypes`, Python via `importlib`, TS/JS via Node ESM, Go via `go run`, Rust via `rustc`)
- `ceesafe-anything build <pkg>` — emit C via `ceesafec build --emit c` and link a C-ABI shared library with `cc`
- `ceesafe-anything diff <pkg>` — replay `cases.jsonl` against the built CeeSafe shared library (honoring optional `known-diffs.jsonl`) and write `.ceesafe-anything/diff.json`
- `ceesafe-anything bench <pkg>` — benchmark original vs CeeSafe shared library over `cases.jsonl` and write `.ceesafe-anything/bench.{json,txt}`
- `ceesafe-anything shadow <pkg>` — generate a drop-in C shadow wrapper (`shadow/<pkg>_shadow.c` + `shadow/flags.txt`) comparing original and CeeSafe side by side
- `ceesafe-anything cutover <pkg> [--apply]` — generate swap-in shim (`cutover/<pkg>_cutover.{c,py,mjs,go,rs}` + `cutover/link.txt`) to route callers to the CeeSafe library (pass `--apply` to back up and replace the original unit)
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
- `examples/rust_math/math_ops.rs` — Rust unit for `bridge` + `record`; generated cutover shims support scalar arguments and byte slices.

## Live Migration Benchmark (`nats-server` Go vs CeeSafe)

One hundred ten `nats-server` subsystems were ported to pure CeeSafe (`0` `unsafe` in `port.cee`, **3,937/3,937** differential cases passing against Go, all 7 `gate` checks passing on every unit, **110/110** lifted `@test` suites passing, and **94 live cgo cutovers** — `subject_match`, `wire_math`, `semver`, `ws_frame`, `hdr_lookup`, `msg_trace`, `msg_ttl`, `ack_reply`, `proto_wire`, `mqtt_wire`, `sublist_match`, `js_usage_codec`, `store_policy`, `gw_reply`, `reply_guard`, `util_num`, `filestore_meta`, `comp_negotiate`, `js_batch_codec`, `route_auth_util`, `js_api_meta`, `host_path_util`, `subject_class`, `sys_hash`, `js_enum_json`, `ws_handshake`, `ws_ext_header`, `fs_hdr_leaf`, `mqtt_hdr_util`, `monitor_js_util`, `js_res_util`, `cfg_enum_util`, `hdr_edit`, `client_arg_util`, `raft_snap_util`, `tls_curve_util`, `js_policy_str`, `closed_state_util`, `dns_route_match`, `js_enum_str`, `js_marshal_json`, `proto_arg_util`, `jwt_time_path`, `client_kind_util`, `mqtt_pkt_util`, `ws_ev_util`, `mqtt_pubrel_util`, `ev_lat_ws`, `js_rep_codec`, `mqtt_pub_hdr`, `raft_vote_codec`, `batch_flow_json`, `raft_peer_resp`, `fast_batch_parse`, `raft_entry_codec`, `js_stream_msg_enc`, `stream_hdr_util`, `core_flag_rate`, `mqtt_retained_codec`, `consumer_ack_fmt`, `js_err_snap`, `store_rle_util`, `route_sub_proto`, `leaf_sub_key`, `store_cstate_enc`, `client_hdr_gen`, `mqtt_sparkb_ts`, `client_hdr_ops`, `ws_close_comp`, `mqtt_retained_dec`, `leaf_sub_proto`, `route_msg_parse`, `source_hdr_util`, `trace_redact`, `leaf_msg_parse`, `client_pub_parse`, `client_msg_hdr`, `gw_sub_proto`, `version_parse`, `mapping_fn_parse`, `gw_msg_hdr`, `sub_arg_parse`, `redact_cli_args`, `resolver_parse`, `creds_parse`, `group_hex_val`, `mapping_arg_split`, `conn_state_util`, `js_group_fmt`, `ws_mask_ops`, `mqtt_pub_wire`, `ws_unmask_ops`, `js_bytes_fmt`, and `js_api_subj` — passing `nats-server`'s own `go test -tags ceesafe_cutover ./server` suite). One additional subsystem (`proxyproto`) has a CeeSafe benchmark below pending its Go baseline comparison.

Head-to-head in-process throughput on Apple Silicon (pure Go vs CeeSafe compiled via `ceesafec build --emit c` + `cc -O2`, timed in-process with `time.Now()` / `Clock.monotonicNanos()`, identical inputs and verified checksums; **107/107 checksums matching**, aggregate `6070.32 ms` Go -> `1240.77 ms` CeeSafe = **4.89x** across the 107 measured workloads):


| Subsystem | Source Unit | Workload | Go (ms) | CeeSafe -O2 (ms) | Speedup | Checksum |
|---|---|---|---:|---:|---:|---|
| `cfg_enum_util` | `server/{monitor,opts,server}.go` | 50k HealthZ/write-policy/TLS/cert-match & pinned-cert SHA-256 hex checks | 840.53 | 5.07 | **165.65x** | match |
| `js_group_fmt` | `server/{jetstream_cluster,client}.go` | 200k `formatGroupName`, `subjectQueueString` & `adjustPingInterval` | 30.25 | 0.20 | **154.44x** | match |
| `comp_negotiate` | `server/server.go` | 300k S2 compression negotiation, RTT & option validation | 76.57 | 0.53 | **145.5x** | match |
| `closed_state_util` | `server/{monitor,auth}.go` | 200k `ClosedState.String` (37 variants) & connection-type normalization | 29.15 | 0.38 | **77.22x** | match |
| `raft_peer_resp` | `server/raft.go` | 200k `appendEntryResponse.encode`, `peerStateBufSize` & `encode/decodePeerState` | 26.56 | 0.40 | **66.52x** | match |
| `js_marshal_json` | `server/{store,filestore,consumer,stream}.go` | 200k `Ack`/`Replay`/`Deliver`/`Compression`/`Action`/`Priority`/`Persist` JSON marshalers | 37.78 | 0.61 | **61.55x** | match |
| `js_enum_str` | `server/{consumer,stream,store}.go` | 200k `ConsumerAction`/`PriorityPolicy`/`PersistMode` stringers & JSON marshalers | 24.03 | 0.44 | **54.07x** | match |
| `client_kind_util` | `server/{client,monitor,util}.go` | 200k client kind/type stringers, `HealthZErrorType.MarshalJSON` & `secondsToDuration` | 26.38 | 0.54 | **48.72x** | match |
| `route_auth_util` | `server/auth.go` | 200k bcrypt, FNV-1a route pool, TLS version & tracked reply | 64.96 | 1.39 | **46.82x** | match |
| `raft_snap_util` | `server/raft.go` | 100k Raft `snap.<term>.<index>` parses, `RaftState` & `EntryType` stringers | 68.71 | 2.24 | **30.64x** | match |
| `js_bytes_fmt` | `server/{jetstream,accounts,ocsp}.go` | 200k `friendlyBytes`, `ServiceRespType.String` & `ocspStatusString` | 27.98 | 0.92 | **30.31x** | match |
| `resolver_parse` | `server/opts.go` | 200k `MEM`/`URL(...)` string account-resolver config parses | 152.67 | 5.78 | **26.44x** | match |
| `redact_cli_args` | `server/opts.go` | 200k `RedactArgs` sensitive CLI flag classifications | 56.19 | 2.17 | **25.87x** | match |
| `mapping_fn_parse` | `server/{subject_transform,sublist}.go` | 200k mapping-function token, account-fn & 10-regex validation checks | 131.98 | 5.29 | **24.97x** | match |
| `mapping_arg_split` | `server/subject_transform.go` | 200k `getMappingFunctionArgsByKind` comma-arg splits across 13 mapping fns | 101.89 | 4.14 | **24.60x** | match |
| `creds_parse` | `server/leafnode.go` | 100k NATS `.creds` armored JWT & NKey Seed block parses | 328.69 | 13.42 | **24.49x** | match |
| `version_parse` | `server/util.go` | 200k `versionComponents` & `versionAtLeastCheckError` SemVer parses | 140.61 | 7.26 | **19.36x** | match |
| `js_err_snap` | `server/{jetstream_errors,raft}.go` | 200k `ApiError.Error` & `raft.encodeSnapshot` binary framing | 36.37 | 1.89 | **19.26x** | match |
| `ws_mask_ops` | `server/websocket.go` | 200k `wsMaskBuf` & `wsMaskBufs` 4-byte unrolled XOR masking | 50.86 | 2.66 | **19.15x** | match |
| `semver` | `server/util.go` | 200k SemVer 2.0.0 parses + compares | 148.10 | 7.96 | **18.60x** | match |
| `store_policy` | `server/store.go` | 200k run-length len + 1M JetStream policy JSON parses | 9.02 | 0.50 | **18.10x** | match |
| `group_hex_val` | `server/{consumer,opts,auth}.go` | 200k priority group name, pinned-cert hex64 & mustache template scans | 204.56 | 12.02 | **17.01x** | match |
| `monitor_js_util` | `server/{monitor,jetstream_api}.go` | 200k `SortOpt` + `myUptime` + JS JSON/empty req checks | 70.69 | 4.46 | **15.86x** | match |
| `trace_redact` | `server/client.go` | 100k `removeSecretsFromTrace` CONNECT secret redactions | 444.07 | 28.48 | **15.59x** | match |
| `js_batch_codec` | `server/jetstream_cluster.go` | 300k control hdr + skip update + batch message decodes | 5.22 | 0.34 | **15.45x** | match |
| `batch_flow_json` | `server/stream.go` | 200k `BatchFlowAck`/`BatchFlowGap` JSON marshalers & `ExternalStream.Domain` | 95.14 | 7.00 | **13.60x** | match |
| `client_hdr_gen` | `server/client.go` | 200k `genHeader` client header generation | 30.41 | 2.26 | **13.47x** | match |
| `mqtt_retained_codec` | `server/mqtt.go` | 200k `mqttEncodeRetainedMessage` & `mqttNewDeliverableMessage` | 25.63 | 1.91 | **13.41x** | match |
| `wire_math` | `server/util.go` | 5M `parseSize` + `mulSaturate` | 45.67 | 3.81 | **11.99x** | match |
| `raft_entry_codec` | `server/raft.go` | 200k `appendEntry.encode` & `decodeAppendEntry` wire frames | 43.90 | 3.93 | **11.18x** | match |
| `js_res_util` | `server/{jetstream,opts}.go` | 300k JetStream reservation, tier naming & storage-size parses | 42.93 | 4.11 | **10.44x** | match |
| `raft_codec` | `server/raft.go` | 300k Raft `AppendEntry`/Resp/`PeerState` decodes | 29.70 | 2.92 | **10.19x** | match |
| `thw` | `server/thw/thw.go` | 500x (1000 THW adds + 500 removes) | 37.08 | 4.14 | **8.96x** | match |
| `consumer_state` | `server/filestore.go` | 100k JetStream `ConsumerState` v2 decodes | 37.18 | 4.98 | **7.47x** | match |
| `route_sub_proto` | `server/route.go` | 200k `addRouteSubOrUnsubProtoToBuf` & `parseUnsubProto` | 38.91 | 5.34 | **7.29x** | match |
| `subject_transform` | `server/subject_transform.go` | 50k multi-fn subject transforms | 141.58 | 19.92 | **7.11x** | match |
| `js_rep_codec` | `server/jetstream_cluster.go` | 200k `decodeAckUpdate`, `decodeDeliveredUpdate` & `decodeStreamMsg` | 8.07 | 1.14 | **7.09x** | match |
| `leaf_sub_key` | `server/leafnode.go` | 200k `keyFromSub` & `keyFromSubWithOrigin` | 10.29 | 1.53 | **6.73x** | match |
| `dns_route_match` | `server/{auth,route,client,gateway}.go` | 200k RFC 6125 DNS AltName wildcard match, routed sub key & enum stringers | 41.18 | 6.24 | **6.61x** | match |
| `ev_lat_ws` | `server/{events,websocket}.go` | 200k `remoteLatencySubjectForResponse` & `wsGetHostAndPort` | 20.84 | 3.25 | **6.42x** | match |
| `stream_state` | `server/store.go` | 100k JetStream `StreamReplicatedState` v2 decodes | 33.62 | 5.27 | **6.38x** | match |
| `cron` | `server/cron.go` | 100k 6-field UTC cron evaluations | 92.30 | 14.85 | **6.22x** | match |
| `client_arg_util` | `server/{client,util,accounts,memstore}.go` | 200k `splitSubjectQueue`, `comma`, `clampInt64ToInt32` & `memStoreMsgSizeRaw` | 48.67 | 7.96 | **6.12x** | match |
| `raft_vote_codec` | `server/raft.go` | 200k Raft `voteRequest`/`voteResponse` & `decodeAppendEntryResponse` codecs | 8.46 | 1.41 | **5.99x** | match |
| `consumer_ack_fmt` | `server/{consumer,sdm}.go` | 200k `ConsumerConfig.replicas`, `consumer.ackReply` & `isSubjectDeleteMarker` | 44.31 | 7.53 | **5.89x** | match |
| `stree` | `server/stree/stree.go` | 300k `SubjectTree` wildcard matches | 61.05 | 10.66 | **5.73x** | match |
| `filestore_meta` | `server/filestore.go` | 300k block/record sizing, TTL/schedule scan & compression JSON | 61.41 | 11.70 | **5.25x** | match |
| `conn_state_util` | `server/{monitor,client,sublist}.go` | 200k `parseConnState`, `normalizeConnType`, `numTokens` & `tokensCanMatch` | 9.51 | 1.85 | **5.13x** | match |
| `store_cstate_enc` | `server/store.go` | 200k `encodeConsumerState` binary wire framing | 5.19 | 1.06 | **4.92x** | match |
| `jwt_time_path` | `server/{jwt,opts}.go` | 200k JWT time-range validation, `normalizeBasePath` & `parseTLSVersion` | 22.52 | 4.59 | **4.90x** | match |
| `gw_sub_proto` | `server/gateway.go` | 200k `gwBuildSubProto`, `gwFormatAccProto` & `parseGatewayRSubArgs` | 19.79 | 4.07 | **4.86x** | match |
| `ws_close_comp` | `server/{websocket,msgtrace}.go` | 200k `wsCreateCloseMessage` & `getCompressionType` | 10.94 | 2.30 | **4.75x** | match |
| `mqtt_sparkb_ts` | `server/mqtt.go` | 200k `sparkbReplaceDeathTimestamp` protobuf field-1 varint updates | 11.84 | 2.54 | **4.66x** | match |
| `mqtt_pub_wire` | `server/mqtt.go` | 200k `WritePublishHeader` & `readVarInt` MQTT wire codecs | 4.23 | 0.91 | **4.66x** | match |
| `nats_archive` | `server/archive/archive.go` | 50k `NATSARC1` binary archive decodes | 61.21 | 13.50 | **4.53x** | match |
| `mqtt_retained_dec` | `server/mqtt.go` | 200k `mqttSliceHeaders` & header-path `mqttDecodeRetainedMessage` | 38.97 | 8.68 | **4.49x** | match |
| `mqtt_wire` | `server/mqtt.go` | 400k MQTT header/varint checks + 400k SparkplugB parses | 26.16 | 5.95 | **4.40x** | match |
| `msg_ttl` | `server/stream.go` | 400k JetStream `Nats-TTL` duration parses | 13.77 | 3.17 | **4.34x** | match |
| `msg_trace` | `server/msgtrace.go` | 200k `NATS/1.0` trace header scans | 46.30 | 10.74 | **4.31x** | match |
| `ws_frame` | `server/websocket.go` | 100k x 4KB WebSocket XOR masking (400MB) | 199.77 | 46.81 | **4.27x** | match |
| `mqtt_pubrel_util` | `server/mqtt.go` | 200k MQTT PUBREL NATS header parse/build & retained pending durable | 17.58 | 4.16 | **4.23x** | match |
| `mqtt_hdr_util` | `server/mqtt.go` | 250k MQTT flag encode/decode, reserved sub & strict UTF-8 checks | 23.46 | 5.74 | **4.09x** | match |
| `tls_curve_util` | `server/{opts,sublist,client,monitor}.go` | 200k TLS curve lookup, `tokenAt`, `isJSReservedReply` & `HealthZErrorType.String` | 9.38 | 2.59 | **3.62x** | match |
| `gsl` | `server/gsl/gsl.go` | 600k `GenericSublist` wildcard matches | 99.47 | 27.95 | **3.56x** | match |
| `proto_arg_util` | `server/{client,leafnode,gateway}.go` | 200k `splitArg`, `proxyDialAddress`, `getGWRoutedSubjectOrSelf` & `isJSAPIRequest` | 13.41 | 4.02 | **3.33x** | match |
| `util_num` | `server/util.go` | 200k `parseUint64` + `addSaturate` + `comma` formats | 21.38 | 6.53 | **3.27x** | match |
| `subject_match` | `server/sublist.go` | 1M `IsValidSubject` + 2M `SubjectsCollide` | 244.76 | 76.56 | **3.20x** | match |
| `ws_ev_util` | `server/{websocket,events}.go` | 200k RFC 6455 `wsFillFrameHeader` & `getAcceptEncoding` | 11.28 | 3.75 | **3.01x** | match |
| `ws_unmask_ops` | `server/websocket.go` | 100k x 1KB `wsReadInfo.unmask`, close status & header-key checks | 18.19 | 6.32 | **2.88x** | match |
| `conf_lex` | `conf/lex.go` | 25k full config lexes | 77.66 | 27.61 | **2.81x** | match |
| `mqtt_pub_hdr` | `server/mqtt.go` | 200k `mqttMakePublishHeader` & `mqttParsePublishNATSHeader` | 25.92 | 9.30 | **2.79x** | match |
| `js_policy_str` | `server/{store,filestore,consumer}.go` | 200k JetStream policy, storage, cipher & compression stringers | 1.80 | 0.66 | **2.71x** | match |
| `gw_reply` | `server/gateway.go` | 400k gateway routed reply + prefix checks | 1.32 | 0.53 | **2.49x** | match |
| `stream_hdr_util` | `server/stream.go` | 200k `streamAndSeq`, `getExpectedLastSeq`, `getBatchSequence` & `getRollup` | 58.06 | 23.72 | **2.45x** | match |
| `store_rle_util` | `server/store.go` | 200k `IsEncodedStreamState`, `appendRunLength` & `DeleteRange.State` | 1.20 | 0.49 | **2.44x** | match |
| `leaf_sub_proto` | `server/leafnode.go` | 200k `writeLeafSub`, `processLeafSub` arg parse & perm-check exemptions | 8.02 | 3.33 | **2.41x** | match |
| `hdr_edit` | `server/client.go` | 100k `removeHeaderIfPresent`, `removeHeaderIfPrefixPresent` & `setHeader` | 37.52 | 16.26 | **2.31x** | match |
| `proto_parser` | `server/parser.go` | 50k pipelined wire batches (250k frames) | 26.27 | 11.72 | **2.24x** | match |
| `js_stream_msg_enc` | `server/jetstream_cluster.go` | 200k `encodeStreamMsgAllowCompressAndBatch` wire framing | 8.45 | 3.81 | **2.22x** | match |
| `client_msg_hdr` | `server/client.go` | 200k `msgHeader` & `msgHeaderForRouteOrLeaf` wire header builds | 3.94 | 1.78 | **2.22x** | match |
| `proto_wire` | `server/proto.go` | 800k protobuf wire scans + 200k varint encodes | 8.69 | 3.94 | **2.20x** | match |
| `core_flag_rate` | `server/{feature_flags,rate_counter,ring}.go` | 200k feature-flag lookup/status, rate-counter step & ring buffer math | 3.36 | 1.53 | **2.19x** | match |
| `fast_batch_parse` | `server/stream.go` | 200k `getFastBatch` suffix parser, `streamAndSeqFromAckReply` & `consumerFromAckReply` | 30.76 | 14.58 | **2.11x** | match |
| `js_codec` | `server/jetstream_cluster.go` | 300k JetStream `StreamMsg`/Delivered/Ack decodes | 6.11 | 3.08 | **1.98x** | match |
| `mqtt_pkt_util` | `server/mqtt.go` | 200k MQTT delete JSON, QoS2 msg ID, flags & level-up checks | 9.26 | 5.20 | **1.78x** | match |
| `sublist_match` | `server/sublist.go` | 400k `matchLiteral` + 200k `subsetMatch` + 200k `tokenAt` | 18.93 | 10.99 | **1.72x** | match |
| `js_api_subj` | `server/jetstream_api.go` | 200k `parseConsumerCreateSubject`, path-sep & stream/consumer token scans | 29.49 | 17.98 | **1.64x** | match |
| `ack_reply` | `server/consumer.go` | 300k `$JS.ACK` v1/v2 + `Stream-Source` parses | 37.36 | 23.93 | **1.56x** | match |
| `ldap_dn` | `internal/ldap/dn.go` | 100k `ParseDN` + `Equal` | 80.76 | 54.34 | **1.49x** | match |
| `client_hdr_ops` | `server/client.go` | 200k `removeHeaderStatusIfPresent` & `sliceHeader` | 6.49 | 4.64 | **1.40x** | match |
| `gw_msg_hdr` | `server/gateway.go` | 200k gateway `RMSG`/`HMSG` wire header builds | 4.80 | 3.45 | **1.39x** | match |
| `hdr_lookup` | `server/client.go` | 1.5M `NATS/1.0` header lookups | 54.14 | 41.01 | **1.32x** | match |
| `js_usage_codec` | `server/jetstream.go` | 200k 3-tier binary usage decodes + 400k name checks | 9.30 | 7.13 | **1.30x** | match |
| `seqset` | `server/avl/seqset.go` | 5000x (4096 inserts + delete + `SpanEnd`) | 58.18 | 45.23 | **1.29x** | match |
| `mqtt_topic` | `server/mqtt.go` | 500k MQTT <-> NATS topic conversions | 136.45 | 106.51 | **1.28x** | match |
| `reply_guard` | `server/client.go` | 300k reserved reply/dest + `$JS.ACK` checks | 21.25 | 17.57 | **1.21x** | match |
| `host_path_util` | `server/util.go` | 200k `parseHostPort` + `normalizeBasePath` + `isReservedAccount` | 31.23 | 25.90 | **1.21x** | match |
| `client_pub_parse` | `server/client.go` | 200k `processPub` & `processHeaderPub` wire arg parses | 5.98 | 5.04 | **1.19x** | match |
| `route_msg_parse` | `server/route.go` | 200k `processRoutedMsgArgs`, `processRoutedHeaderMsgArgs` & `processRoutedOriginClusterMsgArgs` | 6.51 | 5.53 | **1.18x** | match |
| `leaf_msg_parse` | `server/leafnode.go` | 200k `processLeafMsgArgs` & `processLeafHeaderMsgArgs` wire arg parses | 5.53 | 4.80 | **1.15x** | match |
| `source_hdr_util` | `server/stream.go` | 200k `composeIName`, `composeCName`, `genSourceHeader` & `isControlMsg` | 75.45 | 71.08 | **1.06x** | match |
| `sub_arg_parse` | `server/{client,leafnode}.go` | 200k `parseSub`, `processUnsub` & `processLeafSub` wire arg parses | 4.83 | 4.55 | **1.06x** | match |
| `js_api_meta` | `server/jetstream_versioning.go` | 300k API level header check + retry backoff + S2 comp metadata | 9.30 | 9.18 | **1.01x** | match |
| `subject_class` | `server/sublist.go` | 400k wildcard/literal + JS pull req + internal client checks | 28.28 | 33.61 | **0.84x** | match |
| `ws_handshake` | `server/websocket.go` | 100k RFC 6455 SHA-1 + Base64 accept key, max msg size & close frame | 61.60 | 78.92 | **0.78x** | match |
| `js_enum_json` | `server/consumer.go` | 1.2M JetStream `ConsumerAction`/`PriorityPolicy`/`PersistModeType` JSON parses | 3.11 | 5.13 | **0.61x** | match |
| `sys_hash` | `server/events.go` | 100k SHA-256 base-62 system hash (scalar software vs ARMv8 crypto) + tier check | 11.77 | 51.96 | **0.23x** | match |
| `proxyproto` | `server/proxyproto.go` | 50k PROXY protocol v1/v2 hex parses (addresses & ports) | *pending* | 11.73 | — | — |
| **Total** | **107 measured (+1 pending)** | **In-process monotonic clock** | **6070.32** | **1240.77** | **4.89x** | **107/107 match** |

## Agent Skills

CeeSafe porting instructions for coding agents live in `skills/ceesafe-anything/SKILL.md` and `skills/ceesafe-anything/whole-app.md`.
