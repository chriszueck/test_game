# ShroomVault — PROGRESS

Updated: 2026-07-18

## Current state

Playable, 6 worlds (Castle, Megashroom, Sporeway, Gorge, Skyhaven, Bonewood).
Builds with
`build.bat` (release) / `build.bat dev` (console). Verified via `--demo`
(physics autopilot), `--shot*` (screenshots), and `--edcheck` (editor +
serialization self-test, writes `edcheck_log.txt`).

## Just landed (this session)

1. **Mushroom size variety** — route pads, reds, and decoratives now span
   button (r≈1.5) to colossus (r≈11): fat welcoming first pads, tight crux
   pads, size-rhythm on the Megashroom shelf spiral, varied bounce caps in
   Sporeway/Gorge, plus `addMiniShroom`/`addShroomPatch` (decor-only button
   families, no collision). Cap/stem proportion clamps widened in
   `addShroom`/`addCap` so extremes read correctly.
2. **Level editor (F4)** — `src/editor.h`. Group-based (every composite builder
   tags parts with a shared `grp` id via `GrpScope`). Pick/move/scale/dup/
   delete, prefab drop (keys 1–0), spawn/star markers, invisible-collision
   cages. F5 → `levels/levelN.txt`; `BuildWorld` loads that file instead of the
   code builder when it exists (delete file = back to code). Serializer in
   `src/level_io.h`; format is hand-editable text.
3. **Mechanic editor (F6)** — `src/tuning.h`. All physics constants in
   `game.h` demoted from `const` to live-tunable registry entries (name, range,
   step, description). Adjust live while playing; F5 → `mechanics.txt`, loaded
   at startup by `InitTuning()`.

4. **THE BONEWOOD (level 5, warp key 6)** — hardest world, new mechanic
   **WALLSPRING** (`player.h`: tap LMB mid-air within `WALL_GRACE` of a wall
   contact → kick `WALL_VY + WALL_CONV·fallspeed` up, `WALL_OUT` away; contact
   set in `moveHoriz`/`resolveCylsRadial`). Unlocked at the Springheart shrine
   (type 3, green). Six acts, each gating a mechanic: wallspring chimney ×2,
   web-only crossing, PERFECT-slam-only drum (drop 14 → apex 28.5 needed 24.7;
   good slam reaches 19.8 — fails), sail+updraft glide (unsailed range 17 m vs
   42 m gap), spore-turbo shelves (+9.5 m vs 7.6 m unboosted max). One red
   bail. Anti-cheese: all mid-void ledges are floating slabs (no kickable
   column pairs); P2↔drum faces 6.4 m apart (kick cycle nets <1 m there).

## Verification status

- Demo phase 8 PASS: wallspring bot chained 27 m up the Split Trunk through
  real physics and landed on the 34 m exit lip (`demo_log.txt` WALLSPRING line).
- Bonewood acts 2–5 are **math-verified only** (ranges/windows derived from
  live constants, mirroring proven Gorge/Skyhaven/Sporeway numbers) — not yet
  hand-driven end to end.
- `--edcheck` PASS: per-level save→load→save byte-identical roundtrip (all 6),
  editor pick/move/scale/dup/delete ops, file-override + code-fallback path,
  mechanics.txt roundtrip.
- `--demo` clean: vault, foul, web swing, perfect slam, Skyhaven updraft
  height-cap all behave as before the refactor.
- Screenshots (`--shot`..`--shote`) eyeballed for the new size spread.
- **Unverified (needs a human hand):** editor *feel* — mouse-pick accuracy at
  range, nudge step sizes, and the F6 panel ergonomics were exercised only
  programmatically, not by hand-play.

## Known constraints / gotchas

- Once a `levels/levelN.txt` exists, code changes to that level's builder in
  `world.h` stop showing up until the file is deleted or re-saved.
- Editor arrows conflict with the F6 panel, so F6 is blocked while F4 is open.
- Harness modes (`--demo`, `--shot*`) always build from code (`forceCode`), so
  player-edited levels can't break the verification suite.
- `shroomvault_save.txt` position-resume clamps to world bounds; a level file
  that moves spawn outside x±214/z −74..274 will trip the mangled-save reset.

## Next steps (open)

- Hand-playtest both editors (Chris).
- Possible: undo stack for the editor; rotation support (only decor has rotY).
