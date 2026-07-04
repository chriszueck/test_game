# 🍄 ShroomVault

*A foddian first-person pole-vaulting ascent.* Climb a giant castle using nothing
but a pole vault. No checkpoints. No jump button. The meadow is always waiting.

Written in C++ with [raylib](https://www.raylib.com) — no engine, no assets:
every texture, model, and sound is generated in code at startup.

## Play

Run **`ShroomVault.exe`**.

| Input | Action |
|---|---|
| WASD + mouse | run & look |
| **hold LMB / SPACE** | swing the pole down from vertical (this is your charge) |
| **release** | plant the pole & launch — longer hold = bigger vault |
| release in the **bright green** | **PERFECT** (+15% power). Hold past it and you **FOUL** (stumble) |
| **sprint into the plant** | run speed converts into extra height — sprint-vaults go much bigger |
| R (hold 1s) | rage-reset to the meadow |
| ESC | pause · M mute · F11 fullscreen · Q (paused) save & quit |

**The one rule of the vault:** you must be on solid ground when you release.
Mid-air releases whiff — unless you release a moment *before* landing: a charged
release just above the ground is buffered and fires the instant you touch down.

**Red-capped mushrooms** are trampolines — you can't stand on them, but they
return ~90% of your fall. Aim for them when things go wrong. Tan mushrooms,
pipes, blocks, ledges, and clouds are solid ground: you can vault off them.
And the star itself sits on a giant red cap: **the final move is a bounce.**

**Coins** trace every route — grab streaks of them and the chime climbs the
scale. **?-blocks** pop a bonus coin on first touch (bonk them from below,
Mario-style, or just land on them). Everything respawns when you reset.

### The routes
There are **many roads up** — flags mark where they start. Every choice can be
bailed into a red mushroom, and every fall goes exactly as far as you earned.

**Meadow → the ramparts (16 m)** — pick a lane at the flags:
- 🟩 **West Ladder** *(steady)* — tan shroom → pipe → two tall tans → wall
- 🟨 **Pipe Yard** *(precision)* — five pipes of rising height, tiny lips
- 🟥 **Watchtower Skip** *(expert)* — two sprint-PERFECTs off a lone brick tower

**Ramparts → keep roof (46 m)** — three ways across the courtyard:
- gate-tower slabs → **?-block bridge** → keep balconies
- **giant shroom stack** in the east courtyard → beams bolted to the keep
- bold **gap-bricks** straight across — long sprint-vaults, big air

**Keep roof → the ★ (~94 m)** — two ways up the spire:
- **peg spiral** round the bare mini-tower → **cloudwalk** → balcony ring
- the **broken brick stair** spiraling the spire itself
- the finish: vault onto the giant red cap sealing the spire top and let
  the bounce carry you through the star. There is nowhere to stand up there.

Falls don't kill you. They just put you back. That's worse.

Progress (position, timer, lifetime stats) autosaves to `shroomvault_save.txt`
next to the exe. Delete it for a truly fresh start.

## Build from source

Everything needed is in the repo — a portable GCC (w64devkit) lives in `tools\`,
prebuilt raylib 5.5 in `raylib\`. No installs required:

```bat
build.bat        &rem game build (no console)
build.bat dev    &rem dev build with console logging
```

Source layout (single translation unit):

```
src/main.cpp    game loop, HUD, particles, saves, --shot/--demo harnesses
src/game.h      tuning constants (vault power, gravity, bounce...) & types
src/world.h     the whole level, hand-placed
src/gfx.h       cel-shading shader, procedural textures, rendering
src/player.h    movement, collision, THE POLE VAULT
src/audio.h     all sounds synthesized at startup
```

Debug harnesses: `ShroomVault.exe --shot` captures five viewpoint screenshots
(shot1–5.png); `--demo` runs a scripted vault + foul through the real physics
and writes `demo_log.txt` telemetry.

## License notes
- raylib is © Ramon Santamaria, [zlib/libpng license](https://github.com/raysan5/raylib/blob/master/LICENSE).
- Game code: do whatever you like with it. Have fun. Send times.
