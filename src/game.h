// ============================================================================
// ShroomVault — a foddian first-person pole-vault ascent
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
static const float GRAV_UP     = 34.0f;   // gravity while rising
static const float GRAV_DOWN   = 46.0f;   // heavier fall = snappier arcs
static const float TERMINAL    = 55.0f;   // max fall speed
static const float RUN_SPEED   = 8.0f;    // ground run speed
static const float GROUND_ACC  = 60.0f;
static const float AIR_ACC     = 10.0f;   // air steer (nudge landings, not fly)
static const float PLAYER_R    = 0.35f;   // half-width
static const float PLAYER_HH   = 0.90f;   // half-height (center to feet)
static const float EYE_OFF     = 0.70f;   // eye above center
static const float STEP_UP     = 0.36f;   // auto-step height while grounded
static const float COYOTE      = 0.12f;

// vault: hold to swing pole 0..90 deg, release to launch. past 90 = risk zone.
static const float CHARGE_FULL = 0.90f;   // seconds to reach 90 deg (full power)
static const float PERFECT_WIN = 0.15f;   // first slice past full = PERFECT (+15%)
static const float OVERSHOOT   = 0.30f;   // total grace past full before FOUL
static const float PERFECT_MULT= 1.15f;
static const float VAULT_VY0   = 8.0f;    // vertical launch at tap
static const float VAULT_VY1   = 13.0f;   // + per power
static const float VAULT_FWD0  = 2.5f;    // forward impulse at tap
static const float VAULT_FWD1  = 5.5f;    // + per power
static const float VAULT_KEEP  = 0.6f;    // fraction of run speed carried through
static const float MOMENTUM_K  = 0.25f;   // sprint-into-launch conversion (adds vy)
static const float LAND_STICK  = 0.15f;   // horizontal velocity kept on hard landing
static const float BUFFER_T    = 0.12f;   // early-release grace: vault fires on touch
static const float BOUNCE_K    = 0.90f;   // red mushroom restitution
static const float BOUNCE_MIN  = 9.0f;
static const float BOUNCE_MAX  = 40.0f;
static const float HITSTOP_VAULT   = 0.05f;  // impact-frame slow-mo on plant
static const float HITSTOP_PERFECT = 0.11f;

static const Vector3 SPAWN_POS = { 0.0f, 0.91f, -8.0f };
static const Vector3 STAR_POS  = { 0.0f, 94.2f, 136.0f };  // the goal: bounce-height only
static const float   STAR_WIN_R = 2.3f;

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
            S_SHROOM_TAN, S_SHROOM_RED, S_CLOUD, S_GOLD, S_DARK };

struct Solid {
    bool    isCyl;
    Vector3 mn, mx;       // box bounds (also broad-phase for cyl)
    Vector3 base;         // cyl base center
    float   rad, hgt;     // cyl radius/height
    int     surf;
    bool    bouncy;
    bool    visible;      // some solids are drawn as custom decor instead
    bool    used;         // ?-blocks: coin already popped
};

enum DecKind { D_SPHERE, D_CUBE, D_CYL, D_CONE };
struct Decor {
    int kind; Vector3 pos, scale; Color col; int tex; float rotY;
};

// autopilot flags for the --demo verification mode (OR'd into real input)
static bool gBotHold = false, gBotFwd = false;

// game feel state shared between player physics and the frame loop
static float gHitstop = 0;    // remaining impact-frame slow-mo (main loop scales dt)
static float gShake   = 0;    // camera shake amplitude, decays each frame

// ------------------------------------------------------------ event hooks --
// implemented in main.cpp; called from player physics / world logic
void MSG(const char* text, float dur = 4.0f);
void FX_Land(Vector3 pos, float impact);
void FX_Bounce(Vector3 pos, float speed);
void FX_Vault(Vector3 plantPos, float charge, bool perfect);
void FX_Foul();
void FX_Whiff();
void FX_ChargeTick(int quarter);
void FX_Bump(Vector3 pos, int surf);
void FX_QCoin(Vector3 blockTop);
void FX_BigFall(float meters);
void FX_Win();
void SpawnDust(Vector3 pos, int n, Color col);
void SpawnSparkle(Vector3 pos, int n);
