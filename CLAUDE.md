# ceesafe-anything

Pure CeeSafe CLI and stdio MCP server for incrementally migrating existing codebases to CeeSafe.

## Development

```sh
ceesafec fmt
ceesafec check --deny-warnings
ceesafec test
ceesafec build -o out/ceesafe-anything
```

## Rules

- Keep all `unsafe` blocks strictly at the C-ABI boundary (`src/bridge.cee`) with both `/// Safety:` doc comments and `// SAFETY:` comments.
- Never use `ceesafe_*` as an `@export` symbol prefix (reserved for the CeeSafe runtime); use `port_<name>_*`.
- Handle `AllocError` and `IoError` explicitly via `Result`.
