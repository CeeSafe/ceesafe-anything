# Whole app to a pure CeeSafe app

The goal: a standalone CeeSafe app that does what the original does, with the original kept only as
reference. The app stays runnable at every step: a CeeSafe **front door** owns the entry point early
and hands whatever is not ported yet to the original, units move to CeeSafe one at a time through
steps 1 to 5 of [SKILL.md](SKILL.md), and the **lift** at the end removes the fallback.

Everything you write for the port lives in `<repo>/ceesafe/`: `MAP.md`, `DECISIONS.md`,
`GLOSSARY.md`, `HANDOFF.md`, `RULES.md`, `research/`, `app-cases/`, the unit packages, and the `app/`
package.

## Workflow Summary

1. **Grill the user & plan**: Run `ceesafe-anything analyze <repo> --whole-app` to map entry points,
   dependencies (`lib/std`, `CeeSafeLibs`, `CeeSafe.Web`, `CeeSafe-index`), and port waves. Record
   decisions in `ceesafe/DECISIONS.md` and domain terms in `ceesafe/GLOSSARY.md`.
2. **Record the whole app**:
   - CLI: `ceesafe-anything app record <repo> -- <original command>`
   - HTTP server: `ceesafe-anything app record <repo> --http -- <original server command>`
   - Verify baseline: `ceesafe-anything app replay <repo>` must pass 100%.
3. **Front door**: Build the CeeSafe front door (`CeeSafe.Web` `web_server` + `web_proxy` for HTTP
   servers, using `spawnIsolated` / `trySpawnIsolated` so a panicking handler cannot crash the whole
   server; or a CeeSafe CLI wrapper for CLIs) and verify 100% replay.
4. **Port every unit**: Claim each unit in `ceesafe/MAP.md`, follow `SKILL.md` steps 2–5, and pass
   `ceesafe-anything gate <pkg>` (including `ceesafec fmt --check`, `ceesafec check --deny-warnings`,
   and `ceesafec test --sweep alloc`) before wiring into the front door.
5. **Lift & cut over**: Once fallback rate is 0% and every unit passes `gate`, remove the fallback
   paths, run `ceesafe-anything app replay <repo> --build <binary>`, and keep `app drift` in CI.
