// ============================================================================
// tuning.h : the MECHANIC EDITOR (F6) - every physics constant of every
// mechanic, live-tunable while you play. Arrow keys adjust; changes apply the
// same frame, so you can retune gravity mid-vault and feel it. F5 saves to
// mechanics.txt (loaded on startup); delete the file for factory settings.
// ============================================================================
#pragma once
#include "game.h"
#include <cstdio>

// HUD text helpers live in main.cpp (same TU, defined later)
static void drawTextSh(const char* s, int x, int y, int size, Color c);

struct Tun { const char* name; float* v; float mn, mx, step; const char* desc; float def; };
static Tun gTun[] = {
    // -------- movement --------
    {"RUN_SPEED",   &RUN_SPEED,    2,  20, 0.5f, "ground run speed (m/s)"},
    {"GROUND_ACC",  &GROUND_ACC,  10, 200, 5.0f, "ground acceleration"},
    {"AIR_ACC",     &AIR_ACC,      0,  20, 0.5f, "mid-air steering strength"},
    {"STEP_UP",     &STEP_UP,      0,   1, 0.02f,"auto-step height while running"},
    {"COYOTE",      &COYOTE,       0, 0.5f,0.02f,"grace time to vault after leaving a ledge"},
    // -------- gravity --------
    {"GRAV_UP",     &GRAV_UP,      5, 100, 1.0f, "gravity while rising"},
    {"GRAV_DOWN",   &GRAV_DOWN,    5, 100, 1.0f, "gravity while falling"},
    {"TERMINAL",    &TERMINAL,    10, 120, 1.0f, "max fall speed"},
    // -------- the vault --------
    {"CHARGE_FULL", &CHARGE_FULL, 0.2f, 3, 0.05f,"seconds of hold for full power"},
    {"PERFECT_WIN", &PERFECT_WIN, 0.02f,0.6f,0.01f,"PERFECT window length (s past full)"},
    {"OVERSHOOT",   &OVERSHOOT,   0.05f,1, 0.02f,"grace past full before a FOUL"},
    {"PERFECT_MULT",&PERFECT_MULT,1, 1.6f,0.01f, "power multiplier on a PERFECT"},
    {"VAULT_VY0",   &VAULT_VY0,    0,  25, 0.5f, "vertical launch at a tap"},
    {"VAULT_VY1",   &VAULT_VY1,    0,  30, 0.5f, "vertical launch added per power"},
    {"VAULT_FWD0",  &VAULT_FWD0,   0,  10, 0.2f, "forward push at a tap"},
    {"VAULT_FWD1",  &VAULT_FWD1,   0,  15, 0.2f, "forward push added per power"},
    {"VAULT_KEEP",  &VAULT_KEEP,   0,   2, 0.05f,"fraction of sprint speed carried through"},
    {"MOMENTUM_K",  &MOMENTUM_K,   0,   1, 0.02f,"sprint converted to extra height"},
    {"BUFFER_T",    &BUFFER_T,     0, 0.5f,0.01f,"early-release buffer (fires on landing)"},
    {"LAND_STICK",  &LAND_STICK,   0,   1, 0.05f,"speed kept after a hard landing"},
    // -------- red-cap bounces --------
    {"BOUNCE_K",    &BOUNCE_K,   0.1f, 1.5f,0.02f,"red cap restitution (plain bounce)"},
    {"BOUNCE_MIN",  &BOUNCE_MIN,   0,  30, 0.5f, "minimum bounce-out speed"},
    {"BOUNCE_MAX",  &BOUNCE_MAX,  10, 100, 1.0f, "maximum bounce-out speed"},
    // -------- the slam --------
    {"SLAM_GRAV",   &SLAM_GRAV,   20, 150, 2.0f, "dive gravity"},
    {"SLAM_TERM",   &SLAM_TERM,   20, 150, 2.0f, "dive terminal velocity"},
    {"SLAM_WIN",    &SLAM_WIN,  0.05f,  1, 0.02f,"PERFECT window (s before impact)"},
    {"SLAM_K_GOOD", &SLAM_K_GOOD,0.3f,  2, 0.02f,"restitution: slammed early"},
    {"SLAM_K_PERF", &SLAM_K_PERF,0.5f,  2, 0.02f,"restitution: slammed at the last blink"},
    {"SLAM_STUN",   &SLAM_STUN,    0,   2, 0.05f,"stun after slamming solid rock"},
    // -------- web swing --------
    {"WEB_RANGE",   &WEB_RANGE,    4,  40, 1.0f, "bloom grab range"},
    {"WEB_MAX_T",   &WEB_MAX_T, 0.5f,  10, 0.25f,"seconds on the silk before it snaps"},
    {"WEB_WILT",    &WEB_WILT,     0,  20, 0.5f, "bloom regrow time after a swing"},
    // -------- spores --------
    {"BOOST_DUR",   &BOOST_DUR,    1,  30, 0.5f, "spore turbo duration"},
    {"BOOST_VY",    &BOOST_VY,     0,  20, 0.5f, "extra launch speed while boosted"},
    {"SPORE_CD",    &SPORE_CD,     0,  30, 0.5f, "spore regrow time"},
    // -------- the skysail --------
    {"SAIL_GRAV",   &SAIL_GRAV,    1,  40, 0.5f, "glide gravity"},
    {"SAIL_TERM",   &SAIL_TERM,    1,  40, 0.5f, "glide terminal fall speed"},
    {"SAIL_STEER",  &SAIL_STEER,   2,  80, 1.0f, "A/D authority under sail"},
    {"SAIL_DIVE",   &SAIL_DIVE,    2,  80, 1.0f, "W-tuck forward acceleration"},
    {"SAIL_WINDK",  &SAIL_WINDK,   0,   4, 0.1f, "how hard ambient wind pushes the sail"},
    // -------- the wallspring --------
    {"WALL_VY",     &WALL_VY,      4,  30, 0.5f, "base vertical kick off a wall"},
    {"WALL_OUT",    &WALL_OUT,     2,  20, 0.5f, "push away from the wall"},
    {"WALL_CONV",   &WALL_CONV,    0,   1, 0.02f,"fall speed converted into spring"},
    {"WALL_GRACE",  &WALL_GRACE,0.05f,0.6f,0.01f,"press window after touching the wall"},
    // -------- game feel --------
    {"HITSTOP_VAULT",  &HITSTOP_VAULT,  0, 0.3f, 0.01f, "impact-frame slow-mo on plant"},
    {"HITSTOP_PERFECT",&HITSTOP_PERFECT,0, 0.4f, 0.01f, "impact-frame slow-mo on PERFECT"},
};
static const int GTUN_N = (int)(sizeof(gTun)/sizeof(gTun[0]));

static bool gTunOpen = false;
static int  gTunSel  = 0;
static bool gTunDirty = false;      // unsaved changes

static void TunPath(char* out, int n){
    snprintf(out, n, "%smechanics.txt", GetApplicationDirectory());
}
static bool SaveTuning(void){
    char path[600]; TunPath(path, sizeof(path));
    FILE* f = fopen(path, "w");
    if (!f) return false;
    fprintf(f, "# ShroomVault mechanics - edited in-game with F6. Delete for defaults.\n");
    for (int i=0;i<GTUN_N;i++) fprintf(f, "%s=%.4f\n", gTun[i].name, *gTun[i].v);
    fclose(f);
    gTunDirty = false;
    return true;
}
static void InitTuning(void){
    for (int i=0;i<GTUN_N;i++) gTun[i].def = *gTun[i].v;      // capture code defaults
    char path[600]; TunPath(path, sizeof(path));
    FILE* f = fopen(path, "r");
    if (!f) return;
    char line[128], name[64]; float v;
    while (fgets(line, sizeof(line), f)){
        if (line[0] == '#') continue;
        if (sscanf(line, "%63[^=]=%f", name, &v) != 2) continue;
        for (int i=0;i<GTUN_N;i++)
            if (!strcmp(name, gTun[i].name)){ *gTun[i].v = clampf(v, gTun[i].mn, gTun[i].mx); break; }
    }
    fclose(f);
}
// keyboard: runs every frame while the panel is open (gameplay keeps running -
// tune gravity mid-jump and feel it immediately)
static void TuningUpdate(void){
    if (!gTunOpen) return;
    bool up   = IsKeyPressed(KEY_UP)   || IsKeyPressedRepeat(KEY_UP);
    bool down = IsKeyPressed(KEY_DOWN) || IsKeyPressedRepeat(KEY_DOWN);
    if (up)   gTunSel = (gTunSel + GTUN_N - 1) % GTUN_N;
    if (down) gTunSel = (gTunSel + 1) % GTUN_N;
    Tun& t = gTun[gTunSel];
    float st = t.step * (IsKeyDown(KEY_LEFT_ALT)? 0.1f : 1.0f);
    bool lt = IsKeyPressed(KEY_LEFT)  || IsKeyPressedRepeat(KEY_LEFT);
    bool rt = IsKeyPressed(KEY_RIGHT) || IsKeyPressedRepeat(KEY_RIGHT);
    if (lt){ *t.v = clampf(*t.v - st, t.mn, t.mx); gTunDirty = true; }
    if (rt){ *t.v = clampf(*t.v + st, t.mn, t.mx); gTunDirty = true; }
    if (IsKeyPressed(KEY_BACKSPACE)){
        if (IsKeyDown(KEY_LEFT_CONTROL)){ for (int i=0;i<GTUN_N;i++) *gTun[i].v = gTun[i].def; }
        else *t.v = t.def;
        gTunDirty = true;
    }
    if (IsKeyPressed(KEY_F5)){
        if (SaveTuning()) MSG("mechanics.txt saved - these are the physics now.", 3.0f);
        else              MSG("could not write mechanics.txt!", 3.0f);
    }
}
static void DrawTuning(void){
    if (!gTunOpen) return;
    int W = GetScreenWidth(), H = GetScreenHeight();
    int pw = 460, px = W - pw - 12, py = 12, ph = H - 24;
    DrawRectangleRounded((Rectangle){(float)px,(float)py,(float)pw,(float)ph}, 0.03f, 6, (Color){16,20,34,235});
    DrawRectangleRoundedLinesEx((Rectangle){(float)px,(float)py,(float)pw,(float)ph}, 0.03f, 6, 2, C_GOLD);
    drawTextSh("MECHANIC EDITOR", px+16, py+10, 24, C_GOLD);
    drawTextSh(gTunDirty? "edited (F5 to save)" : "saved", px+pw-160, py+16, 16,
               gTunDirty? (Color){255,170,60,255} : (Color){140,235,140,200});
    int rowH = 19, listY = py + 44;
    int fit = (ph - 44 - 74) / rowH;
    int first = gTunSel - fit/2;
    if (first > GTUN_N - fit) first = GTUN_N - fit;
    if (first < 0) first = 0;
    for (int r=0; r<fit && first+r<GTUN_N; r++){
        int i = first + r;
        int y = listY + r*rowH;
        bool sel = (i == gTunSel);
        bool moded = fabsf(*gTun[i].v - gTun[i].def) > 1e-5f;
        if (sel) DrawRectangle(px+8, y-2, pw-16, rowH, (Color){60,70,110,200});
        DrawText(gTun[i].name, px+16, y, 16, sel? WHITE : (Color){205,210,225,255});
        const char* vs = TextFormat(moded? "%.3f *" : "%.3f", *gTun[i].v);
        DrawText(vs, px+pw-24-MeasureText(vs,16), y, 16,
                 moded? (Color){255,208,90,255} : (Color){170,180,200,255});
    }
    int by = py + ph - 70;
    DrawRectangle(px+8, by-8, pw-16, 1, (Color){90,100,130,180});
    drawTextSh(gTun[gTunSel].desc, px+16, by, 16, (Color){190,225,255,255});
    drawTextSh(TextFormat("range %g .. %g   default %g", gTun[gTunSel].mn, gTun[gTunSel].mx, gTun[gTunSel].def),
               px+16, by+20, 14, (Color){160,170,190,255});
    drawTextSh("UP/DOWN pick  ·  LEFT/RIGHT adjust (ALT = fine)  ·  BKSP reset (CTRL+BKSP all)  ·  F5 save  ·  F6 close",
               px+16, by+42, 13, (Color){200,205,220,220});
}
