# THE GREAT ASCENT — restructure blueprint

Goal: one continuous foddian mega-level. All six worlds stacked (mostly)
vertically into a single connected climb, ~670 m, one star at the very top.
Falls cascade down through earlier zones; red caps / plateaus / roofs are the
lucky catches. Pogostuck / Only Up structure with connected environments.

## 0. Bug first: slam eats the sail (updraft "broken" in Bonewood)

Mechanism: gLevel==5 force-unlocks slam AND sail. Both live on SHIFT.
IsKeyPressed starts a SLAM the same frame IsKeyDown starts the SAIL; gravity
picks slam (62 m/s²) while the canopy draws open. Updraft (+20 m/s²) loses.

Fix (player.h):
- E = instant slam, always.
- SHIFT when only slam owned: instant slam (Gorge feel untouched).
- SHIFT when only sail owned: hold = sail (Skyhaven feel untouched).
- BOTH owned: press opens sail immediately; release within SLAM_TAP (0.22 s)
  converts to a slam ("tap = dive, hold = glide"). Slam start clears sailing;
  sail requires !slamming. New Player.sailT tracks hold time.
- Harness: gBotShift virtual SHIFT (with edge detect) so --demo drives the
  REAL branch logic. New demo phases: sail+updraft climb over the Bonewood
  geyser; tap-slam PERFECT on the Drum.

## 1. Vertical stack (zone → build offset dx,dy,dz — tuned in code)

| # | zone        | local span | offset (approx)   | world band |
|---|-------------|-----------|--------------------|------------|
| 1 | Castle      | 0..94     | (0, 0, 0)          | 0–94       |
| 2 | Megashroom  | 0..150    | (0, +80, +150)     | 80–228     |
| 3 | Sporeway    | 0..78     | (+95, +225, +155)  | 228–303    |
| 4 | Gorge       | 0..78     | (+97, +310, +230)  | 310–388    |
| 5 | Skyhaven    | 0..151    | (+175, +385, +474) | 387–536    |
| 6 | Bonewood    | 0..133    | (+215, +540, +500) | 540–673 ★  |

Supports that sell the stack: Megashroom grows on a stone crag-plateau off the
castle's back wall; Sporeway islands drift above its crown; the Gorge is a
COLOSSAL FLOATING MESA (inverted-mountain underside, waterfall pouring into
the void); Skyhaven mills above its rim; Bonewood is the twilight bone-plateau
crowning everything. Ground extends under the whole drift (long sad walk back).

## 2. Seams (only already-owned mechanics; coins trace each)

1. Castle spire cap-bounce (90) → crag-plateau edge (80) → shelf 1 (85).
2. Crown cap (228) → Sporeway mound (228) — hop.
3. Sporeway star island (301) → two hop-islands → mesa mouth (310). Thunder
   shrine (slam) at the mouth.
4. Gorge star-column slam-bounce (376→387) → Skyhaven P0 (387). NEW Skysail
   shrine (type 2) on P0.
5. Skyhaven crown terrace (534) → arched bone-bridge → Bonewood rim (540).
   Springheart shrine (wallspring) at grove entry. Star at 672.

Unlock order on the route: web (Weaver's shelf ~149) → slam (310) → sail
(387) → wallspring (545). Save persists all four (usail/uwall added).

## 3. Systems

- gBOff build offset applied inside every primitive adder; gMega flag makes
  builders skip their own ground slab / warp pipe / horizon dressing / spawn
  and star (BuildAscent provides unified versions). BuildWorld(6)=Ascent;
  game always boots 6; 0..5 remain for harness/editor QA.
- Warp pipes removed from the game (no pipes in mega build).
- Sky gradient by altitude: day blue (0) → pale (300) → peach dusk (480) →
  twilight indigo + stars + low moon (600+). Fog color follows. Ambient wind
  by band (Skyhaven easterly in its band only).
- Music: 5 baked loops — meadow, castle march, sky drift, canyon drums (new),
  twilight grove (new). Band table for level 6; old levels keep legacy.
- Bounds: x ±320, z −80..720, clamps + save-mangle check updated. Collision
  loops get a ±y-band early-out (solids count ×6).
- edcheck: roundtrip 0..6 + seam assertions (GroundTopBelow at each seam).
- Shots: --shota = 6 ascent viewpoints. README/PROGRESS rewritten.

## 4. Quality bars checklist (playtest-loop)

- Feel: seams are cinematic (cap-bounce entries, bone-bridge, mesa reveal).
- Skill: seams never trivialize; one red bail max per seam.
- Completability: every seam gap ≤ sprint-vault (~12–14 m) or bounce-assisted;
  verified numerically in edcheck seam table.
- Necessity: shrine before the first gap that needs its power (web gaps 157+,
  first slam shaft 315+, first glide gap 390+, first chimney 546+).
- Visuals: per-zone supports + connective tissue + altitude sky. No floating
  ex-ground props (gMega skips them).
- Stability: no new per-frame allocations; star field is fixed-count.
