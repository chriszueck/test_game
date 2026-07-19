# ShroomVault — PROGRESS

Updated: 2026-07-18 (late session)

## Current state

**THE GREAT ASCENT shipped**: the game is now ONE continuous ~670 m foddian
climb (level 6) — all six worlds stacked and connected. Standalone worlds
0–5 remain for harness/editor QA only. Builds with `build.bat` /
`build.bat dev`.

## This session

1. **Updraft bug fixed (Bonewood act 4).** Root cause: slam and sail both
   unlocked there, both on SHIFT — the same press started a SLAM (gravity 62)
   under the opening canopy, so the geyser felt dead. Now: one power owned =
   legacy feel; both owned = **SHIFT tap = slam, hold = sail** (E always
   slams). States mutually exclusive. Updrafts pull toward a target RISE RATE
   instead of adding raw accel — no column can bank unbounded exit velocity
   (killed a fresh act-5 skip found by the demo at str 34; also hardens
   Skyhaven). Geyser buffed 20→34 str, r 7→8; its 76 m ceiling unchanged.
2. **The restructure** (see BLUEPRINT.md for the full design):
   - `gBOff` build offsets in every leaf adder + `gMega` skips per builder
     (own ground slab, warp pipes, horizon dressing). `BuildAscent()` stacks:
     castle (0,0,0) → megashroom (0,80,170) on a stone **crag plateau** →
     sporeway (102,225,170) → gorge as a **floating mesa** (104,310,270) →
     skyhaven (168,385,522) → bonewood on the **bone plateau** (240,540,592).
     Star at (248, 672.6, 760).
   - **Seams**: spire→crag cloud steps; crown→shore-mound (they touch); two
     hop-isles under the mesa rim; gorge-crown slam-boing → Skyhaven P0;
     bone bridge (arcs past the great mill's blades) → grove rim. All probed
     in `--edcheck` (9-point seam table, all OK).
   - **Shrines in route order**: Weaver's Bloom 149 m (web) → Thunder 310 m
     (slam) → NEW Skysail shrine type 2 on P0 387 m (sail) → Springheart
     540 m (wallspring). Save persists all four (usail/uwall added). Old
     per-world saves keep lifetime stats, reset powers/best-time.
   - **Atmosphere**: altitude-graded sky+fog (day → pale → peach dusk →
     violet → deep twilight), sun fades out ~430 m, camera-anchored moon +
     110-star dome above ~400 m, twilight-tinted high clouds, banded wind
     (Skyhaven's easterly only in its band), 5 music tracks (canyon thunder +
     twilight grove are new) crossfaded by altitude band.
   - Warp pipes removed from the game; win panel/triggers/intro texts for
     level 6; bounds x±340, z −80..850.
3. **Performance** (was 33–61 fps, now **118–145 fps** at all 7 probe views,
   `--perf`):
   - Static opaque decor (~3.4k items) baked into merged meshes bucketed by
     (90 m band, primitive, texture) — a handful of DrawMesh calls. Alpha
     decor, landmarks (scale ≥ 40) and editor mode stay per-item.
     NOTE: raylib de-indexes par_shapes meshes (indices NULL) — bake emits
     triangle soup. Rebake triggers: BuildWorld + editor exit (gBakeDirty).
   - The silent killer: immediate-mode DrawSphere for clouds/stars
     (~14 ms/frame everywhere). Now DrawSphereEx at low segments.
   - Behind-camera rejection for solids/decor/chunks; distance gates for
     coins/updrafts/banners/pinwheels; far shrines draw orb+beacon only;
     windmill spokes vanish past 250 m. Collision loops got a ±y-band
     early-out (6 worlds of solids).

## Verification status

- `--demo` PASS end-to-end incl. new phases 9/10 driving the REAL SHIFT
  branch via gBotShift: hold=sail through the geyser (65.9→77.6 in-column,
  bough overfly 87.5, no slam), tap=slam (0.01 s conversion, PERFECT drum
  bounce apex 39.8).
- `--edcheck` PASS: serialization round-trip levels 0–6, editor ops, ascent
  seam probes ×9, ascent counts (373 solids / 3509 decor / 183 coins /
  4 shrines / 14 updrafts).
- `--shota` eyeballed: tower reads from the meadow; mesa mouth + shrine ✓;
  dusk at P0 ✓; twilight + stars + plateau at the crown ✓.
- Boots clean with fresh save and with migrated old save; 6 s live run, no
  spurious save writes; ~208 MB RAM with baked meshes.
- **Unverified (needs a human hand):** the full climb end-to-end by hand —
  especially seam FEEL (S1 cloud steps, S4 slam-boing to P0 is deliberately
  the tightest, bone bridge), difficulty pacing across zone joins, and music
  band transitions while climbing. Numbers are derived+probed, not felt.

## Known constraints / gotchas

- `levels/level6.txt` (F4 editor save) overrides the whole ascent; delete to
  return to code. Same per-zone files override standalone builds 0–5.
- gBakeDirty must be set after any decor mutation outside the editor path
  (editor exit sets it; BuildWorld sets it).
- The user's pre-restructure save was replaced with a sanitized one
  (lifetime falls/vaults kept; fake fly-test bestTime=30.6 s cleared;
  powers cleared so the shrine cinematics play in order).
- Save-resume clamps to the level-6 bounds; a hand-edited level file that
  moves spawn outside them trips the mangled-save reset.

## Next steps (open)

- Chris: hand-play the full ascent; judge seam feel + difficulty pacing.
- Art second pass candidates: mesa underside silhouette at long range,
  bone-plateau top texture variety, more connective cloud life at 200–300 m.
- Possible: per-zone split times on the HUD (foddian speedrun candy);
  undo stack + rotation for the editor.
