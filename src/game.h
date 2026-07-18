// ============================================================================
// ShroomVault - a foddian first-person pole-vault ascent
// game.h : includes, tuning constants, shared types, event hooks
// ============================================================================
#pragma once
#include "raylib.h"
#define RAYMATH_DISABLE_CPP_OPERATORS
#include "raymath.h"
#include "rlgl.h"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <vector>

// glClear (depth only) for first-person pole rendering on top of the world
extern "C" __declspec(dllimport) void __stdcall glClear(unsigned int mask);
#define GLX_DEPTH_BUFFER_BIT 0x00000100

// ---------------------------------------------------------------- tuning ---
// NOT const: every value here is live-tunable in the in-game MECHANIC EDITOR
// (F6). Defaults are captured at startup; overrides persist in mechanics.txt.
static float GRAV_UP     = 34.0f;   // gravity while rising
static float GRAV_DOWN   = 46.0f;   // heavier fall = snappier arcs
static float TERMINAL    = 55.0f;   // max fall speed
static float RUN_SPEED   = 8.0f;    // ground run speed
static float GROUND_ACC  = 60.0f;
static float AIR_ACC     = 3.5f;    // air steer (nudge landings, not fly)
static const float PLAYER_R    = 0.35f;   // half-width
static const float PLAYER_HH   = 0.90f;   // half-height (center to feet)
static const float EYE_OFF     = 0.70f;   // eye above center
static float STEP_UP     = 0.36f;   // auto-step height while grounded
static float COYOTE      = 0.12f;

// vault: hold to swing pole 0..90 deg, release to launch. past 90 = risk zone.
static float CHARGE_FULL = 0.90f;   // seconds to reach 90 deg (full power)
static float PERFECT_WIN = 0.15f;   // first slice past full = PERFECT (+15%)
static float OVERSHOOT   = 0.30f;   // total grace past full before FOUL
static float PERFECT_MULT= 1.15f;
static float VAULT_VY0   = 8.0f;    // vertical launch at tap
static float VAULT_VY1   = 13.0f;   // + per power
static float VAULT_FWD0  = 0.8f;    // forward push at tap - near-vertical hop
static float VAULT_FWD1  = 4.5f;    // + per power
static float VAULT_KEEP  = 0.65f;   // sprint carried through: run buys distance
static float MOMENTUM_K  = 0.25f;   // sprint-into-launch conversion (adds vy)
static float LAND_STICK  = 0.15f;   // horizontal velocity kept on hard landing
static float BUFFER_T    = 0.12f;   // early-release grace: vault fires on touch
static float BOUNCE_K    = 0.78f;   // red cap restitution, no slam: bounces decay
static float BOUNCE_MIN  = 9.0f;
static float BOUNCE_MAX  = 44.0f;
// THE SLAM: dive mid-air (SHIFT/E). Slam a red cap and the bounce returns
// MORE than you brought - if you pressed late enough.
static float SLAM_GRAV   = 62.0f;   // dive gravity
static float SLAM_TERM   = 60.0f;   // dive terminal velocity
static float SLAM_WIN    = 0.30f;   // pressed within this of impact = PERFECT
static float SLAM_K_GOOD = 0.88f;   // slammed, but early (extra dive speed compensates)
static float SLAM_K_PERF = 1.15f;   // slammed at the last blink
static float SLAM_STUN   = 0.35f;   // slamming solid rock hurts
static float SLAM_TAP    = 0.22f;   // slam+sail both owned: SHIFT tapped shorter than this = SLAM, held = SAIL
static float HITSTOP_VAULT   = 0.05f;  // impact-frame slow-mo on plant
static float HITSTOP_PERFECT = 0.11f;
// THE WALLSPRING (Bonewood): tap the vault key mid-air against a wall and the
// pole kicks you off it. Falling faster when you spring = a bigger spring, so
// a chain up a chimney is a rhythm, not a mash.
static float WALL_VY    = 13.0f;   // base vertical kick
static float WALL_OUT   = 7.0f;    // push away from the wall
static float WALL_CONV  = 0.35f;   // fall speed converted into extra kick
static float WALL_GRACE = 0.16f;   // press window after touching the wall

static const Vector3 SPAWN_POS = { 0.0f, 0.91f, -8.0f };
static const Vector3 STAR_POS  = { 0.0f, 94.2f, 136.0f };  // the goal: bounce-height only
static const float   STAR_WIN_R = 2.3f;

// per-level spawn/goal (BuildWorld sets these; defaults = castle level)
static Vector3 gSpawn = SPAWN_POS;
static Vector3 gStarP = STAR_POS;

// ---------------------------------------------------------------- helpers --
static inline Vector3 operator+(Vector3 a, Vector3 b){ return {a.x+b.x, a.y+b.y, a.z+b.z}; }
static inline Vector3 operator-(Vector3 a, Vector3 b){ return {a.x-b.x, a.y-b.y, a.z-b.z}; }
static inline Vector3 operator*(Vector3 a, float s)  { return {a.x*s, a.y*s, a.z*s}; }
static inline float clampf(float v, float lo, float hi){ return v<lo?lo:(v>hi?hi:v); }
static inline float lerpf(float a, float b, float t){ return a + (b-a)*t; }
static inline float frnd(float a, float b){ return a + (b-a)*((float)GetRandomValue(0,10000)/10000.0f); }
static inline Color ctint(Color c, float m){
    return { (unsigned char)clampf(c.r*m,0,255),(unsigned char)clampf(c.g*m,0,255),
             (unsigned char)clampf(c.b*m,0,255), c.a };
}

// ------------------------------------------------------------- world types -
enum Surf { S_GRASS, S_STONE, S_BRICK, S_WOOD, S_QBLOCK, S_PIPE,
            S_SHROOM_TAN, S_SHROOM_RED, S_CLOUD, S_GOLD, S_DARK, S_SAND, S_SKYSTONE };

struct Solid {
    bool    isCyl;
    Vector3 mn, mx;       // box bounds (also broad-phase for cyl)
    Vector3 base;         // cyl base center
    float   rad, hgt;     // cyl radius/height
    int     surf;
    bool    bouncy;
    bool    visible;      // some solids are drawn as custom decor instead
    bool    used;         // ?-blocks: coin already popped
    int     grp;          // LEVEL EDITOR: entities sharing a grp move as one
};

enum DecKind { D_SPHERE, D_CUBE, D_CYL, D_CONE };
struct Decor {
    int kind; Vector3 pos, scale; Color col; int tex; float rotY;
    int grp;
};

// autopilot flags for the --demo verification mode (OR'd into real input)
static bool gBotHold = false, gBotFwd = false, gBotWeb = false, gBotSlam = false, gBotSail = false;
static bool gBotKick = false;
static bool gBotShift = false, gBotShiftPrev = false;   // virtual SHIFT: drives the real tap-vs-hold branch

// web swing: how far a web bloom can be grabbed from
static float WEB_RANGE = 15.0f;

// game feel state shared between player physics and the frame loop
static float gHitstop = 0;    // remaining impact-frame slow-mo (main loop scales dt)
static float gShake   = 0;    // camera shake amplitude, decays each frame

struct WebAnchor { Vector3 pos; float radius; float wilt; int grp; };
static std::vector<WebAnchor> gWebAnchors;
// one swing per bloom: any detach wilts it; the silk snaps if you dawdle
static float WEB_MAX_T = 3.0f;       // attached longer than this: SNAP
static float WEB_WILT  = 5.0f;       // bloom regrow time after a swing

// golden spore: timed vault turbo - grab it, then chain the big jumps fast
struct Spore { Vector3 p; float cd; int grp; };
static std::vector<Spore> gSpores;
static float gBoostT = 0;                  // boost time remaining
static float BOOST_DUR   = 7.0f;     // how long a spore lasts
static float BOOST_VY    = 5.5f;     // extra launch speed while boosted
static float SPORE_CD    = 5.0f;     // regrow time after pickup

// mechanics unlock as the worlds introduce them (persisted in the save)
static bool gUnlockWeb  = false;           // won at the Weaver's Bloom (Megashroom)
static bool gUnlockSlam = false;           // won at the Thunder Shrine (Gorge)
static bool gUnlockSail = false;           // SKYHAVEN: hold SHIFT to hang-glide
static bool gUnlockWall = false;           // BONEWOOD: won at the Springheart shrine
struct Shrine { Vector3 p; int type; int grp; };  // type 0 = web, 1 = slam, 3 = wallspring
static std::vector<Shrine> gShrines;

// SKYHAVEN wind kingdom: updraft columns, windmills, banners, ambient wind
struct Updraft { Vector3 base; float rad, hgt, str; int grp; };  // a rising air column
static std::vector<Updraft> gUpdrafts;
struct Windmill { Vector3 pos; float rad, tilt, spd; int blades; Color col; int hazard; int grp; };
static std::vector<Windmill> gWindmills;
struct Banner { Vector3 top; float len, w; Color col; float phase; int grp; };
static std::vector<Banner> gBanners;
struct Pinwheel { Vector3 pos; float rad, spd; Color a, b; int grp; };
static std::vector<Pinwheel> gPinwheels;
static Vector3 gAirWind = {0,0,0};          // ambient wind (per-level), pushes gliders
static float SAIL_GRAV   = 8.0f;      // gentle glide gravity (vs 46 falling)
static float SAIL_TERM   = 7.0f;      // glide terminal fall speed
static float SAIL_STEER  = 26.0f;     // WASD authority under sail
static float SAIL_DIVE   = 22.0f;     // W: tuck & accelerate forward+down
static float SAIL_WINDK  = 1.0f;      // how hard ambient wind pushes a glider

// ambient life: fireflies / butterflies / petals orbiting anchor points
struct Mote { Vector3 p; Color c; float r, spd; int grp; };
static std::vector<Mote> gMotes;

// music ducks while you fall - the world holds its breath
static float gMusicDuck = 1.0f;
// unlock fanfare timer (drawn by the HUD; the blooms flare while it runs)
static float gUnlockT = 0;
// DEBUG free-fly camera (F3). NOT part of the game - a level-inspection tool.
static bool gFlyMode = false;

// ------------------------------------------------------------ event hooks --
// implemented in main.cpp; called from player physics / world logic
void MSG(const char* text, float dur = 4.0f);
void FX_Land(Vector3 pos, float impact);
void FX_Bounce(Vector3 pos, float speed);
void FX_Plant(Vector3 plantPos, float charge, bool perfect);
void FX_Vault(Vector3 plantPos, float charge, bool perfect);
void FX_Foul();
void FX_Whiff();
void FX_ChargeTick(int quarter);
void FX_Bump(Vector3 pos, int surf);
void FX_QCoin(Vector3 blockTop);
void FX_Slam();
void FX_SlamHit(Vector3 pos, bool perfect, float outVel);
void FX_WallSpring(Vector3 pos, float power);
void FX_BigFall(float meters);
void FX_Win();
void SpawnDust(Vector3 pos, int n, Color col);
void SpawnSparkle(Vector3 pos, int n);
