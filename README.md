# 🍄 ShroomVault — THE GREAT ASCENT

*A foddian first-person pole-vaulting ascent.* One unbroken climb, ~670 m from
the meadow to the star. No checkpoints. No jump button. Fall, and you fall
through everything you earned — unless you catch a red cap on the way down.
The meadow is always waiting.

Written in C++ with [raylib](https://www.raylib.com) — no engine, no assets:
every texture, model, sound, and all five zone music tracks are generated in
code at startup, and the sky itself changes as you climb: day at the grass,
thin pale air over the islands, peach dusk at the mesa, and star-pricked
twilight at the top of the world.

## The climb

Six realms, stacked into one connected vertical world. Each one teaches a new
power at a shrine — and every realm above demands everything you've learned:

1. **The Castle** (0–94 m) — the doorstep. Vaults, sprint-vaults, PERFECTs,
   red-cap bounces. Three flagged lanes up the ramparts, three roads across
   the courtyard, and the spire finale: bounce the sealed red cap.
2. **The Hanging Meadow & the MEGASHROOM** (80–228 m) — a colossal mushroom
   grew out of the crag behind the keep. Cloud-step off the spire, climb its
   shelf-cap spiral. The **Weaver's Bloom** wakes the **WEB SWING** right
   before the silk gaps. Falls rain back down the spiral, onto the crag, onto
   the castle roofs.
3. **The Sporeway** (228–303 m) — islands adrift beyond the crown. Bounce-run
   the floating red trampolines, swing the blooms, chain **golden-spore**
   turbo hops before the glow dies.
4. **The HANGING MESA** (310–388 m) — an entire quarried canyon floating in
   the sky, waterfall pouring off its rim into nothing. The **Thunder
   Shrine** at the mouth grants the **SLAM**. Red bounce-shafts, arches that
   bite overshoots, the two-bloom river crossing, the lying ledges of the
   Throat — then a PERFECT slam boings you off the crown column.
5. **Skyhaven** (387–536 m) — the windmill kingdom above the rim. The
   **Skysail shrine** on the first terrace unfurls the **SKYSAIL**: vault,
   glide, catch updrafts, thread the red blade-gates, twelve terraces to the
   great mill's crown.
6. **The BONEWOOD** (540–673 m) — a twilight grove of petrified giants on the
   bone plateau, over everything. The **Springheart** wakes the
   **WALLSPRING**, and every power you own becomes a gate: wallspring
   chimneys, a web-only crossing, a PERFECT-slam drum, a 42 m sail glide on a
   dead stalk's last breath, spore-turbo ember shelves — then the Hollow
   Bone, one vault, one bounce, one star.

**Falls don't kill you. They just put you back.** Sometimes 600 meters back.
Steer for the reds, the crag, the mesa floor, the bone plateau — every lucky
catch is progress kept.

## Play

Run **`ShroomVault.exe`**.

| Input | Action |
|---|---|
| WASD + mouse | run & look |
| **hold LMB / SPACE** | swing the pole down (this is your charge) |
| **release** | plant & launch — longer hold = bigger vault |
| release in the **bright green** | **PERFECT** (+15%). Past it: **FOUL** |
| **sprint into the plant** | run speed becomes height |
| **hold RMB** (or tap F) | swing from a glowing **web bloom** — *Weaver's Bloom, 149 m* |
| **SHIFT / E** mid-air | **SLAM** — dive. Perfect-slam a red: the boing returns MORE — *Thunder Shrine, 310 m* |
| **hold SHIFT** mid-air | **SKYSAIL** — glide. W dive, S flare, A/D steer — *Skysail shrine, 387 m* |
| **tap LMB** on a wall mid-air | **WALLSPRING** — kick off; fall in fast = spring out big — *Springheart, 540 m* |
| R (hold 1s) | rage-reset to the meadow |
| ESC | pause · M mute · F11 fullscreen · Q (paused) save & quit |

Once both slam and sail are yours, SHIFT does both: **tap = slam, hold =
sail** (E always slams instantly).

**The one rule of the vault:** you must be on solid ground when you release —
though a release a moment before landing is buffered and fires on touchdown.

**Red-capped mushrooms** are trampolines: you can't stand on them, but they
return ~90% of your fall — more, if you slam them at the last blink. Coins
trace every route. ?-blocks pop a bonus coin. Shrine cinematics mark each new
power, right before the first gap that needs it.

### The editors (F4 / F6)

- **F4 — LEVEL EDITOR.** Fly, LMB-pick whole objects, arrows/PGUP/PGDN move,
  +/- scale, C duplicate, X delete, 1–0 drop prefabs, drag spawn & star.
  **F5** saves to `levels/level6.txt` — from then on the game loads *your*
  ascent (delete the file for the built-in one). F9 reloads.
- **F6 — MECHANIC EDITOR.** Every physics constant live-tunable while you
  play; F5 → `mechanics.txt`, loaded on startup. Delete for factory physics.

Progress autosaves to `shroomvault_save.txt`. Powers, once earned at their
shrines, are yours across sessions. Delete the file for a truly fresh start.

## Build from source

Everything needed is in the repo — portable GCC (w64devkit) in `tools\`,
prebuilt raylib 5.5 in `raylib\`:

```bat
build.bat        &rem game build (no console)
build.bat dev    &rem dev build with console logging
```

Source layout (single translation unit):

```
src/main.cpp    game loop, HUD, particles, saves, harnesses
src/game.h      tuning constants & shared types
src/world.h     all six realms + the seams + BuildAscent (level 6 = the game)
src/gfx.h       cel shader, procedural textures, altitude sky, decor baking
src/player.h    movement, collision, THE POLE VAULT + slam/sail/web/wallspring
src/audio.h     all sounds + five zone tracks, synthesized at startup
src/editor.h    in-game level editor · src/tuning.h mechanic editor
src/level_io.h  level file save/load
```

Debug harnesses: `--shota` screenshots the ascent (shotA1–7.png); `--shot`,
`--shotb`…`--shotw` shoot the six standalone zone builds; `--demo` runs
scripted physics verification (vault, foul, web, slam, sail+updraft, tap-slam,
wallspring) into `demo_log.txt`; `--edcheck` round-trips serialization and
probes every ascent seam; `--perf` writes ms/frame at seven viewpoints.

## License notes
- raylib is © Ramon Santamaria, [zlib/libpng license](https://github.com/raysan5/raylib/blob/master/LICENSE).
- Game code: do whatever you like with it. Have fun. Send times.
