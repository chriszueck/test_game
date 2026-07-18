# 🍄 ShroomVault

*A foddian first-person pole-vaulting ascent.* Climb a giant castle using nothing
but a pole vault. No checkpoints. No jump button. The meadow is always waiting.

Written in C++ with [raylib](https://www.raylib.com) — no engine, no assets:
every texture, model, sound, and the three zone music tracks (meadow tune,
castle march, sky drift — they crossfade as you climb) are generated in code
at startup.

## Play

Run **`ShroomVault.exe`**.

| Input | Action |
|---|---|
| WASD + mouse | run & look |
| **hold LMB / SPACE** | swing the pole down from vertical (this is your charge) |
| **release** | plant the pole & launch — longer hold = bigger vault |
| release in the **bright green** | **PERFECT** (+15% power). Hold past it and you **FOUL** (stumble) |
| **sprint into the plant** | run speed converts into extra height — sprint-vaults go much bigger |
| **hold RMB** (or tap F) | near a glowing **web bloom**: swing from it. Let go to fly with your momentum — *unlocked at the Weaver's Bloom shrine (world 2)* |
| **SHIFT** (or E) mid-air | **SLAM** — tuck and drop. Slam a red cap at the last blink: **PERFECT SLAM**, the bounce returns *more* than you brought — *unlocked at the Thunder Shrine (world 4)* |
| R (hold 1s) | rage-reset to the meadow |
| ESC | pause · M mute · F11 fullscreen · Q (paused) save & quit |

### The editors (F4 / F6)

- **F4 — LEVEL EDITOR.** Fly (WASD/SPACE/CTRL, SHIFT boost), **LMB** picks whatever
  the crosshair hits — a whole mushroom, pipe, or platform selects as one object.
  **Arrows** move it, **PGUP/PGDN** raise/lower (**ALT** = fine steps), **+/-**
  scale, **C** duplicate, **X** delete. Number keys **1–0** drop new prefabs at
  the crosshair (shroom, red shroom, pipe, slab, ?-block, coin, web bloom, spore,
  updraft, sky platform). The green wire sphere is the spawn, the gold one is the
  star — both draggable. Blue cages show invisible collision. **F5** saves to
  `levels/levelN.txt`, and from then on the game loads *your* file for that world
  (delete the file to get the built-in level back). **F9** reloads the last save.
- **F6 — MECHANIC EDITOR.** Every physics constant — gravity, vault power, bounce
  restitution, slam windows, web, spores, the skysail — live-tunable *while you
  play*. **UP/DOWN** pick, **LEFT/RIGHT** adjust (ALT = fine), **BKSP** resets one
  (CTRL+BKSP all), **F5** saves to `mechanics.txt` which loads on every startup.
  Delete the file for factory physics.

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

**Web blooms** (glowing blue-white buds) dot a few tricky stretches. Swing
under one to convert a fall into speed, pump with W, and release at the top
of the arc. They keep your momentum — a swing is only as good as the vault
that started it.

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

### Level 5: SKYHAVEN
A kingdom of colossal windmills in the open sky, built around the **SKYSAIL**.
Vault off a terrace, then **hold SHIFT to unfurl a hang-glider**: you glide on
the wind (a stiff easterly with a nagging crosswind that pushes every flight),
**W** tucks into a dive for speed, **S** flares to brake, **A/D** steer. Sail
into a swirling **updraft** and it lifts you — but each column only reaches the
next terrace, so miss it and you sink into the blue. The loop: vault → sail →
catch the lift → line up the next terrace → land. **Twelve** terraces climb
150 m, each smaller and farther than the last, and five spinning **blade-gates**
(the red mills) straddle the route — a blade sweeping through you rips the sail
away and bats you down, so time your pass through the gap between blades. Only
two bail platforms on the whole ascent. The star crowns the great mill — ride
its gust up to claim it. Reach it via the flagged warp pipe (the pipes now
cycle all five worlds).

### Level 4: THE GORGE
The longest and hardest climb: a quarried canyon of strata, pines, hanging
vines, crystals, a river and a waterfall — built entirely around the SLAM.
Giant red caps rise from the floor as bounce shafts; slam late to grow each
bounce, then land your apex inside the exit window: the stone arches over
every shaft bite overshoots. Some windows demand a PERFECT slam, one
punishes it. Past the two-bloom crossing and the lying ledges of the Throat,
the final move is a vault over a lone red cap into a perfect slam — the
boing carries you through the star. No nets. The floor remembers.

### Level 3: THE SPOREWAY
Floating islands strung across the sky. It opens with a **bounce run**: the
first islands sit too far to vault across, so you launch into the gap, **drop
onto the red trampolines** floating there, and ride the boings to the next
island — no standing on the reds. Then the **web blooms become the bridges**:
vault off an island's edge, grab mid-air, ride the pendulum, and release on the
upswing. One gap takes two blooms in a single flight. New
here: **golden spores** turbo-charge your pole for seven seconds — the
double-height island hops they unlock must be chained before the glow fades
(the ring around your crosshair is the timer). Spores regrow, falls land in
the meadow, red caps wait under the flight lines.

### Level 2: THE MEGASHROOM
Step onto the flagged **warp pipe near the spawn** to visit the second level:
one colossal mushroom, ~150 m tall. Shelf-caps spiral tightly around the
stalk, so every fall drops straight back down the route you climbed — the
red caps you passed are your parachutes. Rises are taller (sprint every
plant), platforms are smaller, three gaps are blocked by the stalk itself
and must be **bounce-hopped** off a red cap, and one gate near the crown
demands a PERFECT. The warp pipes cycle: castle → megashroom → sporeway → castle.

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
