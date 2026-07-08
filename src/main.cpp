// ============================================================================
// ShroomVault - a foddian first-person pole-vault ascent (C++ / raylib)
// main.cpp : particles, HUD, messages, save file, main loop
// ============================================================================
#include "game.h"
#include "world.h"
#include "gfx.h"
#include "audio.h"
#include "player.h"
#include <string>

// ------------------------------------------------------------ global state -
static bool  gPaused = false, gWon = false, gQuit = false;
static float gTime = 0, gResetT = 0, gAutosaveT = 0;
static float gBestEver = 0, gBestTime = 0;
static int   gTotFalls = 0, gTotVaults = 0, gWins = 0;
static char  gSavePath[600];

// --------------------------------------------------------------- messages --
struct Msg { std::string s; float age, dur; };
static std::vector<Msg> gMsgs;
void MSG(const char* text, float dur){
    gMsgs.push_back({text, 0, dur});
    if (gMsgs.size() > 4) gMsgs.erase(gMsgs.begin());
    SND(sPop, 1.0f, 0.35f);
}
static void UpdateMsgs(float dt){
    for (auto& m : gMsgs) m.age += dt;
    while (!gMsgs.empty() && gMsgs.front().age > gMsgs.front().dur) gMsgs.erase(gMsgs.begin());
}

// --------------------------------------------------------------- particles -
struct Part { Vector3 p, v; float life, maxLife, size, grav; Color c; };
static std::vector<Part> gParts;
static void spawnPart(Vector3 p, Vector3 v, float life, float size, float grav, Color c){
    if (gParts.size() > 900) return;
    gParts.push_back({p, v, life, life, size, grav, c});
}
void SpawnDust(Vector3 pos, int n, Color col){
    for (int i=0;i<n;i++)
        spawnPart(pos + (Vector3){frnd(-0.4f,0.4f),0.05f,frnd(-0.4f,0.4f)},
                  {frnd(-1.5f,1.5f), frnd(0.5f,2.6f), frnd(-1.5f,1.5f)},
                  frnd(0.4f,0.9f), frnd(0.09f,0.22f), -5.0f, col);
}
void SpawnSparkle(Vector3 pos, int n){
    for (int i=0;i<n;i++)
        spawnPart(pos + (Vector3){frnd(-0.5f,0.5f),frnd(0,0.5f),frnd(-0.5f,0.5f)},
                  {frnd(-2,2), frnd(2,5), frnd(-2,2)},
                  frnd(0.35f,0.7f), frnd(0.06f,0.13f), -3.0f,
                  (i%2)? (Color){255,235,130,255} : WHITE);
}
static void UpdateParts(float dt){
    for (auto& q : gParts){ q.v.y += q.grav*dt; q.p = q.p + q.v*dt; q.life -= dt; }
    for (size_t i=0;i<gParts.size();)
        if (gParts[i].life <= 0){ gParts[i] = gParts.back(); gParts.pop_back(); } else i++;
}
static void DrawParts3D(void){
    for (auto& q : gParts){
        float k = q.life/q.maxLife;
        drawM(gCube, q.p, {q.size*k+0.02f, q.size*k+0.02f, q.size*k+0.02f}, q.c, TX_WHITE,
              {0.3f,1,0.2f}, q.life*300);
    }
}
static void ConfettiTick(void){
    static const Color cc[5] = {{235,55,40,255},{253,192,40,255},{80,220,100,255},{90,160,255,255},WHITE};
    if (gParts.size() < 650)
        for (int i=0;i<5;i++)
            spawnPart(gStarP + (Vector3){frnd(-1,1),frnd(-0.5f,1),frnd(-1,1)},
                      {frnd(-6,6), frnd(1,7), frnd(-6,6)},
                      frnd(1.8f,3.0f), 0.14f, -8.0f, cc[GetRandomValue(0,4)]);
}

// ------------------------------------------------------------------- coins -
static int   gCoinCount = 0, gQTotal = 0;
static int   gCoinCombo = 0; static float gCoinComboT = 0;
struct CoinPop { Vector3 p; float t; };            // coin leaping out of a ?-block
static std::vector<CoinPop> gCoinPops;
static void DrawCoins(float t){
    for (auto& c : gCoins){
        if (c.taken) continue;
        float by = c.p.y + sinf(t*2.4f + c.p.x*0.7f + c.p.z*0.3f)*0.12f;
        float spin = t*160.0f + c.p.x*31 + c.p.z*17;
        drawM(gSphere, {c.p.x,by,c.p.z}, {0.40f,0.46f,0.11f}, C_GOLD, TX_WHITE, {0,1,0}, spin);
        drawM(gSphere, {c.p.x,by,c.p.z}, {0.27f,0.33f,0.13f}, (Color){255,228,125,255}, TX_WHITE, {0,1,0}, spin);
    }
    for (auto& q : gCoinPops){                     // popped coins arc up and vanish
        float k = q.t/0.6f;
        float y = q.p.y + 0.3f + 2.2f*k - 1.1f*k*k;
        Color gold = C_GOLD; gold.a = (unsigned char)(255*clampf(1.4f-k*1.4f,0,1));
        drawM(gSphere, {q.p.x,y,q.p.z}, {0.42f,0.48f,0.12f}, gold, TX_WHITE, {0,1,0}, q.t*900);
    }
}
static void UpdateCoins(float dt){
    for (auto& q : gCoinPops) q.t += dt;
    for (size_t i=0;i<gCoinPops.size();)
        if (gCoinPops[i].t > 0.6f){ gCoinPops[i] = gCoinPops.back(); gCoinPops.pop_back(); } else i++;
    gCoinComboT = fmaxf(0.0f, gCoinComboT - dt);
    if (gCoinComboT <= 0) gCoinCombo = 0;
    for (auto& c : gCoins){
        if (c.taken) continue;
        float dx = pl.pos.x-c.p.x, dy = pl.pos.y-c.p.y, dz = pl.pos.z-c.p.z;
        if (dx*dx+dz*dz < 1.4f*1.4f && fabsf(dy) < 1.8f){
            c.taken = true; gCoinCount++;
            gCoinCombo = (gCoinCombo < 10)? gCoinCombo+1 : 10; gCoinComboT = 2.0f;
            SND(sDing, 1.15f + 0.05f*gCoinCombo, 0.6f);          // combo climbs the scale
            SpawnSparkle(c.p, 8);
        }
    }
}

// ------------------------------------------------------------ spores -------
static void UpdateSpores(float dt){
    if (gBoostT > 0){
        gBoostT -= dt;
        if (gBoostT <= 0){ gBoostT = 0; SND(sWhiff, 0.65f, 0.55f); }   // the glow fades
        else {                                                          // golden trail
            static float trailT = 0; trailT -= dt;
            if (trailT <= 0){ trailT = 0.07f;
                spawnPart(pl.pos + (Vector3){frnd(-0.3f,0.3f),frnd(-0.8f,0.1f),frnd(-0.3f,0.3f)},
                          {frnd(-0.5f,0.5f), frnd(-0.5f,0.5f), frnd(-0.5f,0.5f)},
                          0.4f, 0.11f, 0.5f, (Color){255,214,70,255});
            }
        }
    }
    for (auto& sp : gSpores){
        sp.cd = fmaxf(0.0f, sp.cd - dt);
        if (sp.cd > 0) continue;
        float dx = pl.pos.x-sp.p.x, dy = pl.pos.y-sp.p.y, dz = pl.pos.z-sp.p.z;
        if (dx*dx+dz*dz < 1.7f*1.7f && fabsf(dy) < 2.0f){
            sp.cd = SPORE_CD;
            gBoostT = BOOST_DUR;
            SND(sBoing, 1.9f, 0.7f); SND(sDing, 0.85f, 0.7f);
            SpawnSparkle(sp.p, 16);
            gShake += 0.10f;
            static bool tip = false;
            if (!tip){ tip = true; MSG("SPORE POWER! Your pole is supercharged - GO GO GO!", 3.5f); }
        }
    }
}
static void DrawSpores(float t){
    for (auto& sp : gSpores){
        if (sp.cd > 0){        // spent: dim husk regrowing
            float k = 1.0f - sp.cd/SPORE_CD;
            drawM(gSphere, sp.p, {0.16f+0.14f*k, 0.16f+0.14f*k, 0.16f+0.14f*k},
                  (Color){150,140,90,255}, TX_WHITE);
            continue;
        }
        float pulse = 0.42f + 0.07f*sinf(t*5.0f + sp.p.x);
        drawM(gSphere, sp.p, {pulse,pulse,pulse}, (Color){255,208,52,255}, TX_WHITE);
        drawM(gSphere, sp.p, {pulse*0.55f,pulse*0.55f,pulse*0.55f}, (Color){255,244,170,255}, TX_WHITE);
        for (int k=0;k<4;k++){ // petals
            float a = t*1.2f + k*1.5708f;
            drawM(gSphere, sp.p + (Vector3){cosf(a)*0.5f, -0.08f, sinf(a)*0.5f},
                  {0.16f,0.05f,0.16f}, (Color){250,250,245,255}, TX_WHITE);
        }
    }
}

// ------------------------------------------------------ shrines & unlocks --
static void SaveGame(void);
static const char* gUnlockName = "";
static const char* gUnlockSub = "";
static bool gOnWarp = false;       // standing on a warp pipe mouth
// ---- cinematic ability unlock (the shrine blooms; power flows into you) ----
static const float CINE_DUR = 3.8f;
static float   gUnlockCine = 0;    // counts down while the unlock cinematic plays
static int     gUnlockCineType = 0;
static Vector3 gUnlockCinePos = {0,0,0};
static void UpdateShrines(void){
    if (gUnlockCine > 0) return;   // one at a time
    for (auto& sh : gShrines){
        bool owned = (sh.type == 0)? gUnlockWeb : gUnlockSlam;
        if (owned) continue;
        float dx = pl.pos.x-sh.p.x, dy = pl.pos.y-sh.p.y, dz = pl.pos.z-sh.p.z;
        if (dx*dx+dz*dz < 2.6f*2.6f && fabsf(dy) < 2.6f){
            if (sh.type == 0){
                gUnlockWeb = true;
                gUnlockName = "WEB SWING";
                gUnlockSub  = "hold RMB near a glowing bloom - release to fly";
            } else {
                gUnlockSlam = true;
                gUnlockName = "THE SLAM";
                gUnlockSub  = "SHIFT mid-air to dive - slam a red cap at the last blink";
            }
            gUnlockCine = CINE_DUR; gUnlockCineType = sh.type; gUnlockCinePos = sh.p;
            gUnlockT = (sh.type == 0)? 5.2f : 0.0f;   // web: the distant blooms flare awake
            SND(sPop, 0.5f, 0.7f);              // a low awakening hum
            SaveGame();
        }
    }
}
static void DrawShrines(float t){
    for (auto& sh : gShrines){
        bool owned  = (sh.type == 0)? gUnlockWeb : gUnlockSlam;
        Color core   = (sh.type == 0)? (Color){130,225,255,255} : (Color){255,110,80,255};
        Color petalA = (sh.type == 0)? (Color){120,210,255,255} : (Color){255,120,70,255};
        Color petalB = (sh.type == 0)? (Color){178,155,255,255} : (Color){255,190,90,255};
        // bloom: 0 = closed bud, 1 = fully open. Ramps open during the cinematic.
        float bloom = 0.0f;
        bool cineHere = (gUnlockCine > 0 && Vector3Distance(sh.p, gUnlockCinePos) < 0.6f);
        if (cineHere) bloom = clampf((CINE_DUR - gUnlockCine)/1.5f, 0, 1);
        else if (owned) bloom = 1.0f;
        float breathe = 1.0f + 0.06f*sinf(t*2.2f);
        float glow = 0.55f + 0.45f*sinf(t*3.1f) + bloom*0.7f + (cineHere?1.0f:0.0f);
        // stem + green calyx (sepals cupping the bud)
        drawM(gCyl, sh.p + (Vector3){0,-1.35f,0}, {0.18f,0.95f,0.18f}, (Color){96,150,84,255}, TX_FIBER);
        for (int k=0;k<5;k++){
            float a = k*1.257f;
            drawM(gSphere, sh.p + (Vector3){cosf(a)*0.34f,-0.30f,sinf(a)*0.34f}, {0.15f,0.30f,0.15f},
                  (Color){84,140,72,255}, TX_FIBER);
        }
        // petals: unfurl from closed (upright, hugging the core) to open (spread, drooping)
        for (int k=0;k<6;k++){
            float az = k*1.047f + t*0.12f;
            float tilt = (14.0f + bloom*82.0f)*DEG2RAD;
            float reach = (0.30f + bloom*0.78f)*breathe;
            float hy = 0.42f - bloom*0.62f;
            Vector3 pos = sh.p + (Vector3){cosf(az)*sinf(tilt)*reach, hy, sinf(az)*sinf(tilt)*reach};
            drawM(gSphere, pos, {0.14f+bloom*0.07f, 0.36f-bloom*0.18f, 0.14f+bloom*0.07f},
                  ctint((k%2)? petalA : petalB, 0.85f+0.15f*(k%2)), TX_STREAK);
        }
        // glowing crystal core (rises + brightens as it opens)
        float cy = 0.28f + bloom*0.42f;
        drawM(gSphere, sh.p+(Vector3){0,cy,0}, {0.16f*glow+0.13f,0.22f*glow+0.15f,0.16f*glow+0.13f}, core, TX_WHITE);
        drawM(gCone,   sh.p+(Vector3){0,cy+0.14f,0}, {0.13f,0.36f,0.13f}, ctint(core,1.15f), TX_WHITE);
        drawM(gSphere, sh.p+(Vector3){0,cy,0}, {0.075f,0.09f,0.075f}, WHITE, TX_WHITE);
        // two counter-rotating rune rings (ornate)
        for (int r=0;r<2;r++){
            float rr = 0.72f + r*0.26f;
            for (int s=0;s<10;s++){
                float a = t*(r? -0.8f:1.1f) + s*0.6283f;
                drawM(gCube, sh.p+(Vector3){cosf(a)*rr, 0.08f+r*0.18f+0.06f*sinf(t*2.0f+s), sinf(a)*rr},
                      {0.05f,0.05f,0.13f}, ctint(core,0.9f), TX_WHITE, {0,1,0}, a*RAD2DEG);
            }
        }
        // orbiting gold seeds
        for (int k=0;k<4;k++){
            float a = t*1.6f + k*1.571f;
            drawM(gSphere, sh.p+(Vector3){cosf(a)*1.28f, 0.2f+sinf(t*2.2f+k)*0.35f, sinf(a)*1.28f},
                  {0.09f,0.09f,0.09f}, C_GOLD, TX_WHITE);
        }
        // beacon pillar — bright while dormant, fades as it blooms
        if (bloom < 0.6f){
            float b = 1.0f - bloom/0.6f;
            DrawCylinder((Vector3){sh.p.x, sh.p.y+42, sh.p.z}, 0.9f, 1.7f, 84, 14,
                         (Color){core.r,core.g,core.b,(unsigned char)((36+22*glow)*b)});
            DrawCylinder((Vector3){sh.p.x, sh.p.y+42, sh.p.z}, 0.32f, 0.55f, 84, 10,
                         (Color){255,255,255,(unsigned char)((28+16*glow)*b)});
        }
    }
}
// the energy orb of the cinematic: rises from the bloomed bud, then streaks in
static void DrawUnlockOrb(const Camera3D& cam){
    if (gUnlockCine <= 0) return;
    float el = CINE_DUR - gUnlockCine;
    Color oc = gUnlockCineType? (Color){255,140,90,255} : (Color){150,225,255,255};
    Vector3 fwd = Vector3Normalize(cam.target - cam.position);
    if (el < 1.9f){                               // gathering + rising from the core
        float rise = clampf(el/1.5f, 0, 1);
        Vector3 op = gUnlockCinePos + (Vector3){0, 0.8f + rise*1.7f, 0};
        float s = 0.18f + 0.30f*rise + 0.06f*sinf(el*12.0f);
        drawM(gSphere, op, {s*1.7f,s*1.7f,s*1.7f}, (Color){oc.r,oc.g,oc.b,90}, TX_WHITE);
        drawM(gSphere, op, {s,s,s}, oc, TX_WHITE);
        drawM(gSphere, op, {s*0.5f,s*0.5f,s*0.5f}, WHITE, TX_WHITE);
    } else if (el < 2.25f){                        // streaks into the camera (the power enters you)
        float k = (el-1.9f)/0.35f;
        Vector3 op = Vector3Lerp(gUnlockCinePos+(Vector3){0,2.5f,0}, cam.position + fwd*0.9f, k*k);
        float s = 0.5f*(1.0f-k)+0.12f;
        drawM(gSphere, op, {s*1.8f,s*1.8f,s*1.8f}, (Color){oc.r,oc.g,oc.b,120}, TX_WHITE);
        drawM(gSphere, op, {s,s,s}, WHITE, TX_WHITE);
    }
}
// ---- SKYHAVEN: windmills, banners, pinwheels, updraft columns -------------
static void DrawWindmills(float t){
    for (auto& w : gWindmills){
        float ang = t*w.spd*DEG2RAD + w.tilt;
        Color hub = ctint(w.col,0.75f);
        if (w.hazard){                                         // danger: a pulsing red hub
            float pu = 0.6f + 0.4f*sinf(t*6.0f);
            hub = (Color){(unsigned char)(40+200*pu), 28, 24, 255};
            drawM(gSphere, w.pos, {w.rad*0.22f, w.rad*0.22f, w.rad*0.22f},
                  (Color){235,70,50,(unsigned char)(70+90*pu)}, TX_WHITE);   // warning glow
        }
        drawM(gSphere, w.pos, {w.rad*0.14f, w.rad*0.14f, w.rad*0.14f}, hub, TX_STONE);
        for (int i=0;i<w.blades;i++){
            float a = ang + i*(6.2832f/w.blades);
            Vector3 dir = {0, -sinf(a), cosf(a)};              // Y-Z plane: faces along X (the travel axis)
            Vector3 mid = w.pos + dir*(w.rad*0.52f);
            Color sail = w.hazard? ((i%2)? (Color){222,54,40,255} : (Color){250,120,64,255})
                                 : ((i%2)? w.col : ctint(w.col,1.12f));
            drawM(gCube, mid, {0.12f, w.rad*0.32f, w.rad*1.04f}, sail,
                  TX_CLOTH, (Vector3){1,0,0}, a*RAD2DEG);        // cloth sail
            drawCylBetween(w.pos, w.pos + dir*w.rad, w.rad*0.04f, w.hazard? (Color){60,20,20,255} : C_WOOD);
        }
    }
}
static void DrawBanners(float t){
    for (auto& b : gBanners){
        int N = 6; Vector3 prev = b.top;
        for (int i=1;i<=N;i++){
            float fr = (float)i/N;
            float wave = sinf(t*3.2f + b.phase + fr*4.0f)*0.55f*fr;
            Vector3 p = b.top + (Vector3){wave + gAirWind.x*0.05f*fr, -b.len*fr, gAirWind.z*0.05f*fr};
            drawM(gCube, (prev+p)*0.5f, {b.w, b.len/N*1.12f, 0.05f},
                  (i%2)? b.col : ctint(b.col,0.85f), TX_CLOTH);
            prev = p;
        }
    }
}
static void DrawPinwheels(float t){
    for (auto& p : gPinwheels){
        float a0 = t*p.spd*DEG2RAD;
        for (int k=0;k<6;k++){
            float a = a0 + k*1.047f;
            Vector3 dir = {0, -sinf(a), cosf(a)};              // faces along X, like the mills
            drawM(gCube, p.pos + dir*(p.rad*0.5f), {0.04f, p.rad*0.4f, p.rad},
                  (k%2)? p.a : p.b, TX_CLOTH, (Vector3){1,0,0}, a*RAD2DEG);
        }
        drawM(gSphere, p.pos, {0.08f,0.08f,0.08f}, C_GOLD, TX_WHITE);
    }
}
static void DrawUpdrafts(float t){
    for (auto& u : gUpdrafts){
        DrawCylinder(u.base, u.rad, u.rad*0.4f, u.hgt, 18, (Color){205,228,255,24});   // faint spout
        for (int k=0;k<12;k++){
            float ph = t*1.6f + k*0.83f;
            float yy = fmodf(ph*3.4f + k*2.1f, u.hgt);
            float a  = ph*2.4f + k;
            float rr = u.rad*(0.28f + 0.55f*(0.5f+0.5f*sinf(k*1.7f)));
            drawM(gSphere, {u.base.x+cosf(a)*rr, u.base.y+yy, u.base.z+sinf(a)*rr},
                  {0.15f,0.15f,0.15f}, (Color){236,246,255,190}, TX_WHITE);
        }
    }
}
// ambient motes: butterflies / petals / fireflies wandering their anchors
static void DrawMotes(float t){
    for (size_t i=0;i<gMotes.size();i++){
        const Mote& m = gMotes[i];
        float dx = m.p.x-pl.pos.x, dz = m.p.z-pl.pos.z;
        if (dx*dx+dz*dz > 100*100) continue;
        for (int k=0;k<3;k++){
            float ph = t*m.spd + (float)i*2.13f + k*2.09f;
            Vector3 p = { m.p.x + sinf(ph)*m.r + sinf(ph*2.37f)*0.5f,
                          m.p.y + sinf(ph*0.63f + k)*m.r*0.45f,
                          m.p.z + cosf(ph*0.81f)*m.r };
            drawM(gSphere, p, {0.085f,0.085f,0.085f}, m.c, TX_WHITE);
        }
    }
}

// ------------------------------------------------------------ wind streaks -
struct Streak { Vector3 p, v; float life, maxLife; };
static std::vector<Streak> gStreaks;
static void UpdateStreaks(float dt){
    float spd = Vector3Length(pl.vel);
    if (spd > 13.0f && gStreaks.size() < 60){
        Vector3 d = pl.vel*(1.0f/spd);
        for (int i=0;i<2;i++)
            gStreaks.push_back({ pl.pos + d*frnd(2.0f,5.5f)
                                 + (Vector3){frnd(-2.8f,2.8f), frnd(-1.6f,2.4f), frnd(-2.8f,2.8f)},
                                 pl.vel, frnd(0.10f,0.17f), 0.17f });
    }
    for (auto& s : gStreaks) s.life -= dt;
    for (size_t i=0;i<gStreaks.size();)
        if (gStreaks[i].life <= 0){ gStreaks[i] = gStreaks.back(); gStreaks.pop_back(); } else i++;
}
static void DrawStreaks(void){
    for (auto& s : gStreaks){
        float k = clampf(s.life/s.maxLife, 0, 1);
        drawCylBetween(s.p, s.p - s.v*0.055f, 0.016f, (Color){255,255,255,(unsigned char)(150*k)});
    }
}

// ---------------------------------------------------------------- FX hooks -
void FX_Land(Vector3 pos, float impact){
    SND(sLand, clampf(1.05f-impact*0.006f, 0.55f, 1.05f), clampf(0.25f+impact*0.02f, 0, 1));
    if (impact > 16.0f) SND(sThud, 1.0f, clampf(impact/34.0f, 0.4f, 1.0f));   // big drop: body thud
    SpawnDust((Vector3){pos.x, pos.y-PLAYER_HH, pos.z}, (int)clampf(4+impact*0.3f,4,16), {168,140,100,255});
    gShake += clampf(impact*0.015f, 0.0f, 0.45f);
}
void FX_Bounce(Vector3 pos, float speed){
    SND(sBoing, clampf(1.28f-speed*0.013f, 0.6f, 1.2f), 0.95f);
    gShake += 0.22f;
    SpawnDust((Vector3){pos.x, pos.y-PLAYER_HH, pos.z}, 8, WHITE);
    SpawnSparkle((Vector3){pos.x, pos.y-PLAYER_HH, pos.z}, 5);
    static bool tip = false;
    if (!tip){ tip = true; MSG("Boing! Red caps bounce you - aim for them when you fall.", 5.0f); }
}
static float gPerfT = 0;
void FX_Plant(Vector3 plantPos, float charge, bool perfect){
    SND(sPlant, 0.96f + 0.06f*charge + (perfect?0.03f:0.0f), 0.78f + 0.12f*charge);
    SND(sPlant, 0.58f, 0.45f + 0.2f*charge);       // low creak: the pole loading up
    SpawnDust(plantPos, 6 + (int)(charge*6), {188,152,92,255});
    SpawnSparkle(plantPos, perfect ? 6 : 3);
    gShake += 0.05f + 0.07f*charge + (perfect?0.08f:0.0f);
}
void FX_Vault(Vector3 plantPos, float charge, bool perfect){
    SND(sWhoosh, 0.80f + 0.40f*charge + (perfect?0.06f:0.0f), 0.55f + 0.45f*charge);
    SND(sBoing, 1.75f, 0.30f);                     // fiberglass twang on release
    SpawnDust(plantPos, 4+(int)(charge*5), {210,180,125,255});
    gShake += 0.10f + 0.15f*charge + (perfect?0.20f:0.0f);
    if (perfect){
        gPerfT = 0.8f;
        SND(sPerfect, 1.0f, 0.95f);            // its own fanfare, not just a ding
        SpawnSparkle(plantPos, 14);
    }
    gTotVaults++;
}
void FX_Foul(){
    SND(sFoul, 1.0f, 0.9f);
    gShake += 0.35f;
    static int i = 0;
    static const char* lines[3] = {
        "FOUL! You held past the sweet spot.",
        "The pole slaps the dirt. Embarrassing.",
        "Too greedy. Release IN the green." };
    MSG(lines[(i++)%3], 2.6f);
}
void FX_Whiff(){ SND(sWhiff, 1.0f, 0.55f); }
void FX_ChargeTick(int q){
    if (q >= 4) SND(sDing, 1.0f, 0.5f);
    else SND(sTick, 0.85f+q*0.09f, 0.4f);
}
void FX_BigFall(float m){
    static int i = 0;
    static const char* lines[5] = {
        "Lost %d m. The mushrooms send their regards.",
        "%d meters. That's a lot of meters.",
        "Down %d m. The castle chuckles softly.",
        "A %d m tumble. Gravity remains undefeated.",
        "%d m gone. Breathe. Vault again." };
    MSG(TextFormat(lines[(i++)%5], (int)m), 4.0f);
    gMusicDuck = 0.10f;                  // the band stops. everyone saw that.
    gTotFalls++;
}
void FX_Bump(Vector3 pos, int surf){
    if (surf == S_QBLOCK){ SND(sDing, 1.3f, 0.6f); SpawnSparkle(pos, 8); }
    else SND(sLand, 1.2f, 0.3f);
}
void FX_QCoin(Vector3 blockTop){
    gCoinPops.push_back({blockTop, 0});
    gCoinCount++;
    SND(sDing, 1.45f, 0.7f);
    SpawnSparkle(blockTop, 10);
}
static float gSlamMsgT = 0;
void FX_Slam(){
    SND(sWhoosh, 0.52f, 0.85f);                    // the tuck: a low dive-roar
    pl.fovKick = -7.0f;                            // zoom in - commitment
    gShake += 0.06f;
}
void FX_SlamHit(Vector3 pos, bool perfect, float outVel){
    if (perfect){
        SND(sBoing, 0.62f, 1.0f);                  // deep mega-boing
        SND(sPerfect, 0.70f, 0.55f);
        gSlamMsgT = 0.8f;
        gShake += 0.45f;
        gHitstop = fmaxf(gHitstop, 0.08f);
        for (int i=0;i<16;i++){                    // shock ring
            float a = i*0.3927f;
            spawnPart((Vector3){pos.x+cosf(a)*0.8f, pos.y-PLAYER_HH, pos.z+sinf(a)*0.8f},
                      {cosf(a)*7.0f, 1.5f, sinf(a)*7.0f}, 0.5f, 0.16f, -4.0f, WHITE);
        }
        SpawnSparkle((Vector3){pos.x, pos.y-PLAYER_HH, pos.z}, 10);
    } else {
        SND(sBoing, 0.82f, 0.95f);
        gShake += 0.28f;
        SpawnDust((Vector3){pos.x, pos.y-PLAYER_HH, pos.z}, 10, WHITE);
    }
    static bool tip = false;
    if (!tip){ tip = true; MSG("SLAM BOUNCE! The later you press, the bigger the boing.", 4.5f); }
    (void)outVel;
}
void FX_Win(){
    SND(sWin, 1.0f, 0.95f);
    MSG("* YOU VAULTED THE CASTLE! *", 6.0f);
}

// -------------------------------------------------------------- save file --
static void SaveGame(void){
    char buf[512];
    snprintf(buf, sizeof(buf),
        "bestAlt=%.2f\nbestTime=%.2f\nwins=%d\ntotFalls=%d\ntotVaults=%d\nlevel=%d\n"
        "uweb=%d\nuslam=%d\n"
        "resume=%d\npx=%.2f\npy=%.2f\npz=%.2f\nyaw=%.4f\npitch=%.4f\ntime=%.2f\n",
        gBestEver, gBestTime, gWins, gTotFalls, gTotVaults, gLevel,
        gUnlockWeb?1:0, gUnlockSlam?1:0,
        gWon? 0:1, pl.pos.x, pl.pos.y, pl.pos.z, pl.yaw, pl.pitch, gTime);
    SaveFileText(gSavePath, buf);
}
static float readKey(const char* txt, const char* key, float def){
    char pat[64]; snprintf(pat, sizeof(pat), "%s=", key);
    const char* p = strstr(txt, pat);
    if (!p) return def;
    return (float)atof(p + strlen(pat));
}
// Deterministic per-world unlocks so the shrine CINEMATICS re-arm every fresh
// visit (that's the whole point of them). The Megashroom earns web at its
// Weaver's Bloom; the Gorge earns slam at its Thunder Shrine. The Sporeway &
// Gorge are simply handed web (their teacher lives back in the Megashroom).
// A mid-level RESUME keeps what you'd already earned so you're never stranded
// above a shrine. resumeY < 0 means a fresh entry (warp / new run).
static void SetLevelUnlocks(float resumeY){
    bool pastWeb  = (gLevel == 1 && resumeY > 68.0f);   // resumed above the Weaver's Bloom
    bool pastSlam = (gLevel == 3 && resumeY >  6.0f);   // resumed up a Gorge shaft, past the mouth
    gUnlockWeb  = (gLevel == 2 || gLevel == 3) || pastWeb;
    gUnlockSlam = pastSlam;
    gUnlockSail = (gLevel == 4);                        // SKYHAVEN: the hang-glider is the world
}
static void LoadSave(void){
    char* txt = LoadFileText(gSavePath);
    if (!txt) return;
    gBestEver  = readKey(txt,"bestAlt",0);
    gBestTime  = readKey(txt,"bestTime",0);
    gWins      = (int)readKey(txt,"wins",0);
    gTotFalls  = (int)readKey(txt,"totFalls",0);
    gTotVaults = (int)readKey(txt,"totVaults",0);
    int lv = (int)readKey(txt,"level",0);
    if (lv >= 1 && lv <= 4 && gLevel != lv){
        BuildWorld(lv);                               // resume on the saved level
        pl.pos = gSpawn;
    }
    bool resuming = readKey(txt,"resume",0) > 0.5f;
    float resumeY = resuming? readKey(txt,"py",gSpawn.y) : -1.0f;
    SetLevelUnlocks(resumeY);                         // shrines re-arm unless you resumed past them
    if (resuming){
        pl.pos = { readKey(txt,"px",gSpawn.x), readKey(txt,"py",gSpawn.y), readKey(txt,"pz",gSpawn.z) };
        pl.yaw = readKey(txt,"yaw",0); pl.pitch = clampf(readKey(txt,"pitch",0.05f), -1.55f, 1.55f);
        gTime = fmaxf(0.0f, readKey(txt,"time",0));
        gTimerStarted = gTime > 0.01f;
        if (pl.pos.y < 0.85f || fabsf(pl.pos.x) > 214 || pl.pos.z < -74 || pl.pos.z > 274)
            pl.pos = gSpawn;                          // mangled save: back to the meadow
        pl.apexY = pl.pos.y;
    }
    UnloadFileText(txt);
}

// -------------------------------------------------------------------- HUD --
static void drawTextSh(const char* s, int x, int y, int size, Color c){
    DrawText(s, x+2, y+2, size, (Color){20,20,28,170});
    DrawText(s, x, y, size, c);
}
static void drawTextC(const char* s, int cx, int y, int size, Color c){
    drawTextSh(s, cx - MeasureText(s, size)/2, y, size, c);
}
static void DrawChargeGauge(void){
    if (!pl.charging) return;
    int W = GetScreenWidth(), H = GetScreenHeight();
    float total = CHARGE_FULL + OVERSHOOT;
    float frac = clampf(pl.chargeT/total, 0, 1);
    float fullFrac = CHARGE_FULL/total;
    float perfFrac = (CHARGE_FULL + PERFECT_WIN)/total;
    int barH = 170, barW = 20;
    int x = W/2 - barW/2, y1 = H - 62, y0 = y1 - barH;
    DrawRectangleRounded((Rectangle){(float)x-6,(float)y0-6,(float)barW+12,(float)barH+12}, 0.5f, 6, (Color){18,20,30,150});
    // zones above full charge: bright green = PERFECT, amber = still full power
    DrawRectangle(x, (int)(y1 - barH*perfFrac), barW, (int)(barH*(perfFrac-fullFrac)), (Color){70,230,95,150});
    DrawRectangle(x, y0, barW, (int)(barH*(1.0f-perfFrac)), (Color){235,170,50,110});
    for (float q=0.25f; q<0.99f; q+=0.25f)
        DrawRectangle(x-3, (int)(y1 - barH*q*fullFrac), barW+6, 2, (Color){255,255,255,90});
    bool inPerf = pl.chargeT >= CHARGE_FULL && pl.chargeT <= CHARGE_FULL + PERFECT_WIN;
    Color fill = inPerf? (Color){90,245,120,255}
               : (frac >= fullFrac)? (Color){245,180,60,255}
               : (Color){(unsigned char)255, (unsigned char)(220-110*(frac/fullFrac)), 40, 255};
    if (pl.chargeT > CHARGE_FULL + PERFECT_WIN && sinf((float)GetTime()*45) > 0)
        fill = (Color){250,60,50,255};
    DrawRectangle(x, (int)(y1 - barH*frac), barW, (int)(barH*frac), fill);
    DrawRectangle(x-4, (int)(y1 - barH*frac)-2, barW+8, 3, WHITE);
    DrawRectangle(x-5, y0-3, barW+10, 3, (Color){250,60,50,220});   // foul line
    drawTextC(inPerf? "PERFECT" : "VAULT", W/2, y1+10, 16,
              inPerf? (Color){120,255,150,255} : (Color){255,255,255,200});
}
static void DrawHUD(float t){
    int W = GetScreenWidth(), H = GetScreenHeight();
    if (gFlyMode){
        // loud, unmistakable: this is a TEST tool, not part of the game
        float p = 0.6f + 0.4f*sinf(t*4.0f);
        DrawRectangle(0, 0, W, 40, (Color){140,20,20,180});
        drawTextC("FLY / TEST MODE  ·  NOT PART OF THE GAME  ·  F3 to exit",
                  W/2, 10, 22, (Color){255,240,120,(unsigned char)(200+55*p)});
        drawTextSh("WASD fly  ·  SPACE up  ·  CTRL/E down  ·  SHIFT boost", 16, 52, 18, (Color){235,235,245,220});
        drawTextSh(TextFormat("pos  x %.1f   y %.1f   z %.1f", pl.pos.x, pl.pos.y, pl.pos.z),
                   16, 78, 20, (Color){140,235,140,255});
        drawTextSh(TextFormat("yaw %.0f   pitch %.0f (deg)", pl.yaw*RAD2DEG, pl.pitch*RAD2DEG),
                   16, 102, 17, (Color){200,220,200,200});
        DrawCircle(W/2, H/2, 3.0f, (Color){255,240,120,220});   // aim dot
        return;
    }
    // THE FALL: meters drain before your eyes, the edges go red
    float drop = pl.apexY - pl.pos.y;
    if (!pl.grounded && !pl.webSwinging && drop > 5.0f){
        float k = clampf((drop-5.0f)/45.0f, 0.0f, 1.0f);
        unsigned char va = (unsigned char)(115*k);
        int ew = (int)(W*0.16f), eh = (int)(H*0.24f);
        DrawRectangleGradientH(0, 0, ew, H, (Color){120,8,8,va}, (Color){120,8,8,0});
        DrawRectangleGradientH(W-ew, 0, ew, H, (Color){120,8,8,0}, (Color){120,8,8,va});
        DrawRectangleGradientV(0, H-eh, W, eh, (Color){120,8,8,0}, (Color){120,8,8,va});
        int sz = 26 + (int)(26*k);
        drawTextC(TextFormat("- %d m", (int)drop), W/2, (int)(H*0.60f), sz,
                  (Color){255, (unsigned char)(130-70*k), (unsigned char)(90-60*k),
                          (unsigned char)(140 + 115*k)});
    }
    // crosshair
    DrawCircle(W/2, H/2, 3.2f, (Color){20,20,28,120});
    DrawCircle(W/2, H/2, 2.0f, (Color){255,255,255,220});
    float webHint = WebSwingRangeFactor();
    if (webHint > 0.0f){
        float pulse = 0.65f + 0.35f*sinf((float)GetTime()*8.0f);
        float rad = 18.0f + 8.0f*webHint + 4.0f*pulse;
        for (int i=0;i<2;i++)
            DrawCircleLines(W/2, H/2, rad + i*3.0f, (Color){130,220,255,(unsigned char)(80 + 70*webHint)});
        DrawCircle(W/2, H/2, 8.0f + 3.0f*webHint, (Color){120,220,255,(unsigned char)(60 + 80*webHint)});
        drawTextC("WEB", W/2, H/2 + 44, 18, (Color){130,220,255,(unsigned char)(180 + 55*webHint)});
    }
    // skysail / updraft state
    if (pl.sailing){
        float pulse = 0.7f + 0.3f*sinf((float)GetTime()*7.0f);
        Color sc = pl.inUpdraft? (Color){130,240,150,(unsigned char)(230*pulse)}
                               : (Color){205,228,255,220};
        drawTextC(pl.inUpdraft? ">> UPDRAFT <<" : "~ SAIL ~", W/2, H/2 + 44, 19, sc);
        if (pl.inUpdraft)                                 // rising chevrons
            for (int k=0;k<3;k++)
                DrawCircleLines(W/2, H/2, 16.0f + k*7.0f + 5.0f*pulse, (Color){130,240,150,120});
    }
    // spore boost: golden ring counting down around the crosshair
    if (gBoostT > 0){
        float frac = gBoostT/BOOST_DUR;
        bool urgent = gBoostT < 1.6f && sinf((float)GetTime()*22) > 0;
        Color rc = urgent? (Color){255,90,60,235} : (Color){255,208,52,220};
        DrawRing((Vector2){(float)W/2,(float)H/2}, 26, 31, -90, -90 + 360*frac, 40, rc);
        drawTextC(TextFormat("%.1f", gBoostT), W/2, H/2 + 40, 17, rc);
    }
    DrawChargeGauge();
    if (gPerfT > 0){
        gPerfT -= GetFrameTime();
        if (gPerfT > 0.62f)     // one golden flash frame set
            DrawRectangle(0,0,W,H,(Color){255,222,95,(unsigned char)(52*clampf((gPerfT-0.62f)/0.18f,0,1))});
        float a = clampf(gPerfT/0.8f, 0, 1);
        int sz = 30 + (int)(8*(1.0f-a));
        drawTextC("PERFECT!", W/2, H/2 - 90, sz, (Color){255,215,60,(unsigned char)(255*a)});
    }
    if (gSlamMsgT > 0){
        gSlamMsgT -= GetFrameTime();
        float a = clampf(gSlamMsgT/0.8f, 0, 1);
        int sz = 26 + (int)(8*(1.0f-a));
        drawTextC("PERFECT SLAM!", W/2, H/2 + 76, sz, (Color){255,150,60,(unsigned char)(255*a)});
    }
    if (gUnlockCine > 0){                              // the unlock cinematic overlay
        float el = CINE_DUR - gUnlockCine;
        Color tc = gUnlockCineType? (Color){255,150,90,255} : (Color){150,225,255,255};
        // cinematic letterbox bars
        float barK = clampf(fminf(el/0.4f, gUnlockCine/0.5f), 0, 1);
        int bh = (int)(H*0.11f*barK);
        DrawRectangle(0,0,W,bh,(Color){0,0,0,225});
        DrawRectangle(0,H-bh,W,bh,(Color){0,0,0,225});
        // the power enters: a bright color flash
        if (el > 1.95f && el < 2.35f){
            float fa = 1.0f - fabsf(el-2.12f)/0.18f;
            DrawRectangle(0,0,W,H,(Color){tc.r,tc.g,tc.b,(unsigned char)(215*clampf(fa,0,1))});
        }
        // the name slams in after the flash
        if (el > 2.05f){
            float in = clampf((el-2.05f)/0.28f, 0, 1);
            float out = clampf(gUnlockCine/0.7f, 0, 1);
            unsigned char a = (unsigned char)(255*fminf(in,out));
            int sz = 34 + (int)(18*in);
            drawTextC("- UNLOCKED -", W/2, (int)(H*0.28f), 22, (Color){tc.r,tc.g,tc.b,a});
            drawTextC(gUnlockName, W/2, (int)(H*0.28f)+30, sz, (Color){255,232,120,a});
            drawTextC(gUnlockSub,  W/2, (int)(H*0.28f)+30+sz+14, 20, (Color){255,246,210,a});
        }
    }
    // altitude
    drawTextSh(TextFormat("ALT  %3.0f m", pl.pos.y), W-198, 16, 30, WHITE);
    drawTextSh(TextFormat("BEST %3.0f m", gBestEver), W-198, 52, 20, (Color){240,240,250,190});
    // timer + salt
    if (gTimerStarted){
        drawTextSh(TextFormat("%02d:%05.2f", (int)(gTime/60), fmodf(gTime,60)), 16, 16, 30, WHITE);
        drawTextSh(TextFormat("vaults %d   fouls %d   falls %d", pl.vaults, pl.fouls, pl.falls),
                   16, 52, 19, (Color){240,240,250,180});
        DrawCircle(24, 86, 8, (Color){20,20,28,170});
        DrawCircle(23, 85, 7, C_GOLD);
        DrawCircle(23, 85, 4, (Color){255,228,125,255});
        drawTextSh(TextFormat("%d / %d", gCoinCount, (int)gCoins.size()+gQTotal), 38, 76, 19, (Color){255,225,120,220});
        if (pl.fallen > 1)
            drawTextSh(TextFormat("%d m eaten by gravity", (int)pl.fallen), 16, 100, 17, (Color){235,235,245,140});
    }
    // messages
    int my = (int)(H*0.13f);
    for (auto& m : gMsgs){
        float a = 1.0f;
        if (m.age < 0.15f) a = m.age/0.15f;
        if (m.dur - m.age < 0.4f) a = (m.dur - m.age)/0.4f;
        drawTextC(m.s.c_str(), W/2, my, 25, (Color){255,255,255,(unsigned char)(235*clampf(a,0,1))});
        my += 34;
    }
    // early controls hint - only the powers you actually own
    if (t < 22 && !gWon){
        char hint[260] = "WASD run  ·  LMB charge + release: VAULT";
        if (gUnlockWeb)  strcat(hint, "  ·  RMB near blooms: swing");
        if (gUnlockSlam) strcat(hint, "  ·  SHIFT mid-air: SLAM");
        if (gUnlockSail) strcat(hint, "  ·  hold SHIFT mid-air: SAIL");
        strcat(hint, "  ·  ESC pause");
        drawTextC(hint, W/2, H-30, 18, (Color){255,255,255,150});
    }
    if (gResetT > 0.04f){
        DrawRing((Vector2){(float)W/2,(float)H/2}, 24, 32, 0, 360*clampf(gResetT,0,1), 40, (Color){253,192,40,220});
        drawTextC("hold to reset…", W/2, H/2+44, 18, (Color){253,210,90,220});
    }
    if (gOnWarp && !gWon){
        float p = 0.7f + 0.3f*sinf((float)GetTime()*6.0f);
        static const char* worlds[5] = {"Castle", "Megashroom", "Sporeway", "Gorge", "Skyhaven"};
        int panelW = 460, panelH = 236, px = W/2 - panelW/2, py = H/2 + 40;
        DrawRectangleRounded((Rectangle){(float)px,(float)py,(float)panelW,(float)panelH}, 0.10f, 8, (Color){18,26,20,225});
        DrawRectangleRoundedLinesEx((Rectangle){(float)px,(float)py,(float)panelW,(float)panelH}, 0.10f, 8, 3,
                                    (Color){140,235,140,(unsigned char)(255*p)});
        drawTextC("WARP  ·  travel to any world", W/2, py+16, 22, (Color){140,235,140,255});
        for (int i=0;i<5;i++){
            bool here = (i == gLevel);
            Color c = here? (Color){120,180,120,200} : (Color){245,250,240,255};
            drawTextC(TextFormat("[%d]   %s%s", i+1, worlds[i], here? "   (you are here)" : ""),
                      W/2, py+52+i*28, 20, c);
        }
        drawTextC("press a number  ·  or walk off to stay", W/2, py+206, 17, (Color){200,220,200,220});
    }
    if (gMuted) drawTextSh("[M] sound off", 16, H-32, 17, (Color){255,255,255,130});
    drawTextSh(TextFormat("%d fps", GetFPS()), W-86, H-32, 17, (Color){255,255,255,140});
}
static void DrawPanelBG(int w, int h){
    int W = GetScreenWidth(), H = GetScreenHeight();
    DrawRectangle(0,0,W,H,(Color){10,15,30,110});
    DrawRectangleRounded((Rectangle){(float)(W-w)/2,(float)(H-h)/2,(float)w,(float)h}, 0.08f, 8, (Color){24,28,48,235});
    DrawRectangleRoundedLinesEx((Rectangle){(float)(W-w)/2,(float)(H-h)/2,(float)w,(float)h}, 0.08f, 8, 3, C_GOLD);
}
static void DrawPause(void){
    int W = GetScreenWidth(), H = GetScreenHeight();
    DrawPanelBG(680, 578);
    int y = (H-578)/2 + 28;
    drawTextC("SHROOMVAULT", W/2, y, 40, C_GOLD); y += 46;
    drawTextC("a foddian pole-vaulting ascent", W/2, y, 18, (Color){220,220,235,200}); y += 40;
    const char* lines[16]; int nl = 0;
    lines[nl++] = "WASD run   ·   mouse look";
    lines[nl++] = "hold LMB or SPACE - pole swings down as it charges";
    lines[nl++] = "release to PLANT & LAUNCH - sprint first: speed becomes height";
    lines[nl++] = "release in the bright GREEN = PERFECT (+15%) - hold past it: FOUL";
    lines[nl++] = "red mushrooms are trampolines. aim for them when you fall";
    if (gUnlockWeb)  lines[nl++] = "hold RMB near a glowing web bloom: swing. let go to fly with it";
    if (gUnlockSlam) lines[nl++] = "SHIFT mid-air: SLAM. slam a red at the last blink: PERFECT boing";
    if (gUnlockSail) lines[nl++] = "hold SHIFT mid-air: SKYSAIL. W dive, S flare, A/D steer, ride updrafts";
    lines[nl++] = "?-blocks pop a coin on first touch. many roads up, no checkpoints";
    lines[nl++] = "step on a warp pipe to travel to ANY world (new powers at shrines)";
    lines[nl++] = "";
    lines[nl++] = "R (hold 1s) restart run   ·   M mute   ·   F11 fullscreen";
    lines[nl++] = "F3  free-fly TEST camera (inspect levels · not part of the game)";
    lines[nl++] = "ESC resume   ·   Q save & quit";
    for (int i=0;i<nl;i++){ drawTextC(lines[i], W/2, y, 19, RAYWHITE); y += 28; }
    y += 8;
    if (gBestTime > 0)
        drawTextC(TextFormat("best climb %02d:%05.2f  ·  wins %d  ·  lifetime falls %d",
                  (int)(gBestTime/60), fmodf(gBestTime,60), gWins, gTotFalls), W/2, y, 17, (Color){253,210,90,220});
    else
        drawTextC(TextFormat("best altitude %d m  ·  lifetime falls %d  ·  the star waits",
                  (int)gBestEver, gTotFalls), W/2, y, 17, (Color){253,210,90,220});
}
static bool gWonMenu = false;      // the choice panel after touching the star
static void DrawWinPanel(void){
    int W = GetScreenWidth(), H = GetScreenHeight();
    static const char* titles[5] = {
        "* YOU VAULTED THE CASTLE *", "* THE MEGASHROOM IS CLIMBED *",
        "* THE SPOREWAY IS CROSSED *", "* THE GORGE IS CONQUERED *",
        "* SKYHAVEN IS YOURS *" };
    DrawPanelBG(640, 388);
    int y = (H-388)/2 + 30;
    drawTextC(titles[gLevel], W/2, y, 32, C_GOLD); y += 52;
    drawTextC(TextFormat("time  %02d:%05.2f", (int)(gTime/60), fmodf(gTime,60)), W/2, y, 30, RAYWHITE); y += 40;
    if (gBestTime > 0)
        drawTextC(TextFormat("best  %02d:%05.2f", (int)(gBestTime/60), fmodf(gBestTime,60)), W/2, y, 19, (Color){230,230,240,200});
    y += 32;
    drawTextC(TextFormat("%d vaults  ·  %d fouls  ·  %d falls  ·  %d/%d coins",
              pl.vaults, pl.fouls, pl.falls, gCoinCount, (int)gCoins.size()+gQTotal), W/2, y, 20, RAYWHITE); y += 46;
    drawTextC("[1]  run this world back", W/2, y, 22, (Color){255,246,210,255}); y += 32;
    drawTextC("[2]  onward - the next world", W/2, y, 22, (Color){255,246,210,255}); y += 32;
    drawTextC("[3]  close this - stay and enjoy the view", W/2, y, 22, (Color){255,246,210,255});
}

// ------------------------------------------------------------ frame render -
static void RenderAll(Camera3D cam, float t, bool hud){
    if (gShake > 0.002f){
        Vector3 off = { frnd(-1,1)*gShake*0.12f, frnd(-1,1)*gShake*0.12f, frnd(-1,1)*gShake*0.12f };
        cam.position = cam.position + off; cam.target = cam.target + off;
    }
    ClearBackground(SKY_TOP);
    int W = GetScreenWidth(), H = GetScreenHeight();
    DrawRectangleGradientV(0, 0, W, (int)(H*0.72f), SKY_TOP, SKY_FOG);
    DrawRectangleGradientV(0, (int)(H*0.72f), W, H-(int)(H*0.72f), SKY_FOG, (Color){228,240,252,255});
    GfxFrame(cam.position);
    BeginMode3D(cam);
        DrawSkyBits(t);
        DrawWebAnchors(cam.position);
        DrawWorld3D(cam.position);
        DrawCoins(t);
        DrawSpores(t);
        DrawShrines(t);
        DrawUnlockOrb(cam);
        if (gLevel == 4){ DrawUpdrafts(t); DrawWindmills(t); DrawBanners(t); DrawPinwheels(t); }
        DrawMotes(t);
        DrawStar(t, gWon);
        DrawGoalBeam(t);
        DrawParts3D();
        DrawStreaks();
        if (!gFlyMode) DrawBlobShadow(pl.pos);
        rlDrawRenderBatchActive();
        glClear(GLX_DEPTH_BUFFER_BIT);          // pole never clips into walls
        if (!gFlyMode) DrawPoleFP(cam, t);      // no body/pole in the debug fly-cam
    EndMode3D();
    if (hud) DrawHUD(t);
}

// ------------------------------------------------------- screenshot harness
static void ShotMode(void){
    struct Preset { Vector3 pos; float yawD, pitchD, charge; const char* file; };
    static const Preset castle[5] = {
        {{0,0.91f,-8},      0,   6, 0,     "shot1.png"},
        {{-10,16.41f,62},  22,   4, 0,     "shot2.png"},
        {{5,0.91f,17},      0,   2, 0.85f, "shot3.png"},
        {{10,70.0f,134},  192, -20, 0,     "shot4.png"},
        {{-7,88.4f,147.5f},145, 14, 0,     "shot5.png"},
    };
    static const Preset tower[4] = {
        {{0,0.91f,16},      0,  26, 0,     "shotT1.png"},
        {{18,30.0f,0},    -24,  30, 0,     "shotT2.png"},
        {{-1.6f,75.0f,62.0f}, 180,  8, 0,  "shotT3.png"},
        {{0,148.91f,34},    0,  10, 0,     "shotT4.png"},
    };
    static const Preset sky[4] = {
        {{-90,3.91f,40},   90,   8, 0,     "shotS1.png"},   // down the island chain
        {{-55,10.91f,40},  90,  12, 0,     "shotS2.png"},   // from I1: blooms ahead
        {{4,22.91f,42},    50,  20, 0,     "shotS3.png"},   // I3: the spore + boost islands
        {{-25,71.91f,39},  95,   6, 0,     "shotS4.png"},   // I8: final swing to the star
    };
    static const Preset gorge[5] = {
        {{0,0.91f,-30},     0,  10, 0,     "shotG1.png"},   // canyon mouth + gate
        {{-13.5f,10.91f,20}, 18, -2, 0,    "shotG2.png"},   // from L1: the Steps ahead
        {{0,44.5f,96},      0,  10, 0,     "shotG3.png"},   // the crossing: blooms over the river
        {{0,24.0f,200},     0,  55, 0,     "shotG4.png"},   // inside the Throat, looking up
        {{2,79.0f,224},     0,   4, 0,     "shotG5.png"},   // crown: the star pinnacle
    };
    static const Preset sky2[5] = {
        {{-64, 3.91f, 0},   75,  8, 0,     "shotK1.png"},   // launch terrace, route climbing east
        {{-40, 40.0f,-8},   78,  6, 0,     "shotK2.png"},   // a red hazard blade-gate + updraft
        {{ 11, 76.0f,-4},   80,  6, 0,     "shotK3.png"},   // mid-climb over the blue
        {{ 33,100.0f,-2},   62, 12, 0,     "shotK4.png"},   // the colossal Skymill ahead
        {{ 40,146.0f, 8},  118,  6, 0,     "shotK5.png"},   // the star crowning the great mill
    };
    const Preset* P = (gLevel==4)? sky2 : (gLevel==3)? gorge : (gLevel==2)? sky : gLevel? tower : castle;
    int n = (gLevel==2)? 4 : (gLevel==1)? 4 : 5;
    for (int i=0;i<n;i++){
        pl.pos = P[i].pos; pl.yaw = P[i].yawD*DEG2RAD; pl.pitch = P[i].pitchD*DEG2RAD;
        pl.charging = P[i].charge > 0;
        pl.chargeT = P[i].charge*CHARGE_FULL;
        pl.poleAngle = P[i].charge*90;
        for (int f=0; f<12; f++){
            BeginDrawing();
            RenderAll(GetCam(), (float)GetTime(), true);
            EndDrawing();
        }
        TakeScreenshot(P[i].file);
    }
}

// --------------------------------------------------- autopilot verification
// --demo: scripted run+vault+foul through the real input/physics path,
// telemetry to demo_log.txt, one mid-flight screenshot.
static void DemoMode(void){
    FILE* lf = fopen("demo_log.txt", "w");
    gSkipLook = 12;                       // physics ticks 2x per polled frame
    const float dt = 1.0f/120.0f;
    float T = 0, maxY = 0;
    int rf = 0; bool shotA = false, shotB = false, webPhase = false;
    bool boostPhase = false, slamPhase = false, edgePhase = false, sailPhase = false;
    while (T < 22.5f && !WindowShouldClose()){
        gBotFwd  = (T > 0.4f && T < 2.2f) || (T > 3.4f && T < 4.0f) || (T > 8.1f && T < 9.05f);
        gBotHold = (T > 1.05f && T < 2.2f) || (T > 3.5f && T < 5.0f)   // vault #1, then over-hold -> FOUL
                || (T > 8.2f && T < 9.15f);                            // boosted PERFECT vault
        if (!webPhase && T > 5.2f){        // phase 3: fling at a test bloom, mid-air grab
            webPhase = true;
            gWebAnchors.push_back({{0, 9.5f, 15.2f}, 3.0f, 0});   // harness-only anchor
            pl.pos = {0, 8.0f, 10.5f}; pl.vel = {0, 0, 8.0f}; pl.yaw = 0;
        }
        gBotWeb = (T > 5.25f && T < 6.6f); // hold the web, then let go and fly
        if (!boostPhase && T > 8.0f){      // phase 4: spore-boosted launch
            boostPhase = true;
            pl.pos = {0, 0.91f, -8}; pl.vel = {0,0,0}; pl.yaw = 0;
            gBoostT = 9.0f;
        }
        if (!slamPhase && T > 10.7f){      // phase 5: perfect slam onto the west red bail
            slamPhase = true;
            gBoostT = 0;
            pl.pos = {-26, 27.0f, 50}; pl.vel = {0,0,0}; pl.yaw = 0;
        }
        gBotSlam = (T > 11.35f && T < 11.43f);   // pressed ~0.27 s before impact = PERFECT
        if (!edgePhase && T > 13.4f){    // phase 6: EDGE-GLITCH test - off-center fast fall onto cap
            edgePhase = true;
            pl.pos = {-23.6f, 42.0f, 50}; pl.vel = {0,0,0}; pl.yaw = 0;   // 2.4 off the -26 center
        }
        if (!sailPhase && T > 15.4f){    // phase 7: SKYHAVEN anti-cheat test - ride U0 as high as it goes
            sailPhase = true;
            BuildWorld(4); gUnlockSail = true;
            pl = Player(); pl.pos = {-57, 7.0f, 8}; pl.vel = {0,-4,0}; pl.yaw = 1.4f;  // over updraft U0 (gap P0->P1)
        }
        gBotSail = (T > 15.45f && T < 22.0f);    // hold the sail the WHOLE time (cheat attempt)
        if (T > 15.45f && T < 22.0f){            // pin to U0's axis: a player perfectly fighting the wind.
            pl.pos.x = -57.0f; pl.pos.z = 8.0f;  // harness-only - proves the column's height CAP (~P1, not the moon)
            pl.vel.x = 0; pl.vel.z = 0;
        }
        PlayerUpdate(dt, false);
        maxY = fmaxf(maxY, pl.pos.y);
        T += dt;
        if (lf && ((int)(T*10) != (int)((T-dt)*10)))
            fprintf(lf, "t=%.1f pos=(%6.2f,%6.2f,%6.2f) vel=(%5.1f,%5.1f,%5.1f) gnd=%d chg=%d web=%d vaults=%d fouls=%d\n",
                    T, pl.pos.x, pl.pos.y, pl.pos.z, pl.vel.x, pl.vel.y, pl.vel.z,
                    pl.grounded?1:0, pl.charging?1:0, pl.webSwinging?1:0, pl.vaults, pl.fouls);
        if ((rf++ & 1) == 0){
            BeginDrawing();
            RenderAll(GetCam(), T, true);
            EndDrawing();
        }
        if (!shotA && T > 2.62f){ TakeScreenshot("demo_flight.png"); shotA = true; }   // vault kick
        if (!shotB && T > 5.9f){ TakeScreenshot("demo_swing.png"); shotB = true; }     // web swing
    }
    gBotFwd = gBotHold = gBotWeb = gBotSlam = gBotSail = false; gBoostT = 0;
    if (lf){
        fprintf(lf, "SUMMARY maxY=%.2f vaults=%d fouls=%d falls=%d final=(%.1f,%.1f,%.1f)\n",
                maxY, pl.vaults, pl.fouls, pl.falls, pl.pos.x, pl.pos.y, pl.pos.z);
        fclose(lf);
    }
}

// ------------------------------------------------------------------- reset -
static void DoReset(void){
    bool hadWon = gWon;
    pl = Player();
    pl.pos = gSpawn; pl.apexY = gSpawn.y;
    gWon = false; gWonMenu = false; gTime = 0; gTimerStarted = false;
    for (auto& c : gCoins) c.taken = false;        // coins respawn with the run
    for (auto& s : solids) s.used = false;         // ?-blocks refill
    gCoinCount = 0; gCoinCombo = 0; gCoinPops.clear();
    gBoostT = 0; for (auto& sp : gSpores) sp.cd = 0;
    for (auto& wa : gWebAnchors) wa.wilt = 0;      // blooms regrow - never soft-lock a web gap
    gMsgs.clear();
    MSG(hadWon? "Round two. The castle is not impressed." : "Back to the meadow. Breathe.", 3.5f);
    SaveGame();
}

// -------------------------------------------------------------------- main -
struct Trig { float alt; const char* txt; bool done; };
static Trig gTrigsCastle[] = {
    {15.8f, "The Ramparts! Three ways up here - three ways onward.", false},
    {24.5f, "Blocks, beams or the big shrooms. Pick a lane.", false},
    {36.0f, "The Keep face. Hug the wall, it forgives overshoots.", false},
    {45.8f, "Keep roof! Pegs on the right, broken stair on the left.", false},
    {62.0f, "Cloudwalk. Sprint, PERFECT, pray.", false},
    {83.6f, "THE SPIRE. The star sits over a giant red cap - bounce through it!", false},
};
static Trig gTrigsTower[] = {
    { 6.0f, "THE MEGASHROOM. Shelf-caps spiral the stalk. Reds are your parachutes.", false},
    {17.0f, "Follow the spiral - each cap turns the same way. Never fight the stalk.", false},
    {40.0f, "Short hop, half charge, full send - READ each gap. One hold fits none.", false},
    {72.0f, "Canopy heights. Fall lanes run all the way down - steer onto a red.", false},
    {110.0f,"The crown run. Reds are scarce and one gate demands a PERFECT.", false},
    {140.0f,"The crown. Don't look down. (Look down.)", false},
};
static Trig gTrigsSky[] = {
    { 4.5f, "THE SPOREWAY. Islands sit far apart - DROP onto the red trampolines to cross.", false},
    { 8.5f, "Vault high, fall onto a red, ride the boing to the next island.", false},
    {18.0f, "NOW the blooms: vault off, grab RMB mid-air, release on the upswing.", false},
    {24.0f, "GOLDEN SPORES turbo-charge the pole. They fade fast - chain the jumps!", false},
    {48.0f, "Spore, vault, land, TURN, vault. No dawdling.", false},
    {68.0f, "One long swing to the star. Time the release. Fly.", false},
};
static Trig gTrigsSky2[] = {
    { 5.0f, "SKYHAVEN. Vault off, then HOLD SHIFT to unfurl your skysail.", false},
    {12.0f, "Glide into a swirling UPDRAFT to soar. W dives for speed, S flares.", false},
    {28.0f, "Steer A/D and read the wind - it pushes every glide eastward.", false},
    {50.0f, "Chain it: vault, sail, catch the lift, line up the next terrace.", false},
    {74.0f, "The great mill. One last soar to its crown and the star.", false},
};
static Trig gTrigsGorge[] = {
    { 3.5f, "THE GORGE. Press SHIFT mid-air to SLAM. Slam a red cap: mega-boing.", false},
    { 8.5f, "Slam LATE - a blink before impact - for a PERFECT. Chain them to climb.", false},
    {19.0f, "Exit ledges wait partway up each shaft. The stone arches bite overshoots.", false},
    {38.0f, "The crossing. Two blooms, no nets. The river is very far down.", false},
    {50.0f, "The Throat: one PERFECT slam off the plank reaches the true exit.", false},
    {64.0f, "Spore the pinnacles. Then one last perfect slam... into the star.", false},
};
static Trig* gTrigs = gTrigsCastle;
static int   gTrigN = 6;

static void ApplyLevelMeta(void){
    gQTotal = 0; for (auto& s : solids) if (s.surf == S_QBLOCK) gQTotal++;
    if      (gLevel == 1){ gTrigs = gTrigsTower;  gTrigN = (int)(sizeof(gTrigsTower)/sizeof(Trig)); }
    else if (gLevel == 2){ gTrigs = gTrigsSky;    gTrigN = (int)(sizeof(gTrigsSky)/sizeof(Trig)); }
    else if (gLevel == 3){ gTrigs = gTrigsGorge;  gTrigN = (int)(sizeof(gTrigsGorge)/sizeof(Trig)); }
    else if (gLevel == 4){ gTrigs = gTrigsSky2;   gTrigN = (int)(sizeof(gTrigsSky2)/sizeof(Trig)); }
    else                 { gTrigs = gTrigsCastle; gTrigN = (int)(sizeof(gTrigsCastle)/sizeof(Trig)); }
    for (int i=0;i<gTrigN;i++) gTrigs[i].done = false;
}
static void GoToLevel(int lv){
    BuildWorld(((lv % 5) + 5) % 5);
    pl = Player();
    pl.pos = gSpawn; pl.apexY = gSpawn.y;
    gWon = false; gWonMenu = false; gTime = 0; gTimerStarted = false;
    gCoinCount = 0; gCoinCombo = 0; gCoinPops.clear();
    gBoostT = 0; gUnlockCine = 0;
    SetLevelUnlocks(-1.0f);            // fresh entry: the shrine here re-arms its cinematic
    ApplyLevelMeta();
    gMsgs.clear();
    SND(sPop, 0.6f, 0.9f); SND(sWhoosh, 0.7f, 0.8f);
    static const char* names[5] = {
        "Back to the castle meadow.",
        "* THE MEGASHROOM *  climb high, bounce well.",
        "* THE SPOREWAY *  swing far, chain the spores.",
        "* THE GORGE *  slam late. Climb the thunder.",
        "* SKYHAVEN *  ride the wind. Hold SHIFT to sail." };
    MSG(names[gLevel], 4.5f);
    SaveGame();
}
static void SwitchLevel(void){ GoToLevel((gLevel + 1) % 5); }

int main(int argc, char** argv){
    bool shot  = (argc > 1) && (strcmp(argv[1], "--shot") == 0);
    bool shotb = (argc > 1) && (strcmp(argv[1], "--shotb") == 0);
    bool shotc = (argc > 1) && (strcmp(argv[1], "--shotc") == 0);
    bool shotd = (argc > 1) && (strcmp(argv[1], "--shotd") == 0);
    bool shote = (argc > 1) && (strcmp(argv[1], "--shote") == 0);
    SetTraceLogLevel(LOG_WARNING);
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(1600, 900, "ShroomVault - a foddian pole vault");
    SetExitKey(KEY_NULL);
    // hard frame cap at the monitor's rate (vsync is only a HINT and often
    // fails) - uncapped GPU churn for minutes is how PCs overheat and die
    int rr = GetMonitorRefreshRate(GetCurrentMonitor());
    SetTargetFPS((int)clampf((float)rr, 60, 144));
    snprintf(gSavePath, sizeof(gSavePath), "%sshroomvault_save.txt", GetApplicationDirectory());
    LoadGfx(); InitSky(); BuildWorld(shote? 4 : shotd? 3 : shotc? 2 : shotb? 1 : 0);
    for (auto& s : solids) if (s.surf == S_QBLOCK) gQTotal++;

    if (shot || shotb || shotc || shotd || shote){ ShotMode(); CloseWindow(); return 0; }
    if (argc > 1 && strcmp(argv[1], "--demo") == 0){ DemoMode(); CloseWindow(); return 0; }

    LoadAudioAll(); InitWind(); StartMusic(); LoadSave();
    if (gLevel != 0) ApplyLevelMeta();      // save resumed us elsewhere
    DisableCursor();

    bool introA=false, introB=false, introC=false;
    while (!WindowShouldClose() && !gQuit){
        float t = (float)GetTime();
        float rawDt = clampf(GetFrameTime(), 0, 0.05f);
        float dt = rawDt;
        if (gHitstop > 0){ gHitstop -= rawDt; dt *= 0.12f; }   // impact frames
        gShake *= expf(-8.0f*rawDt);
        if (gUnlockT > 0) gUnlockT -= rawDt;
        if (gUnlockCine > 0){                                  // unlock cinematic: slow-mo
            gUnlockCine -= rawDt;
            dt *= 0.4f;
            if (gUnlockCine <= 0){ gUnlockCine = 0; gSkipLook = 2;
                SND(gUnlockCineType? sBoing : sDing, 1.2f, 0.7f); }
        }

        if (IsKeyPressed(KEY_ESCAPE)){
            gPaused = !gPaused;
            if (gPaused) EnableCursor(); else { DisableCursor(); gSkipLook = 2; }
        }
        if (!IsWindowFocused() && !gPaused){ gPaused = true; EnableCursor(); }
        if (IsKeyPressed(KEY_M)) gMuted = !gMuted;
        if (IsKeyPressed(KEY_F11)) ToggleBorderlessWindowed();
        if (gPaused && IsKeyPressed(KEY_Q)) gQuit = true;
        if (IsKeyPressed(KEY_F3)){                       // DEBUG free-fly toggle
            gFlyMode = !gFlyMode; gSkipLook = 2;
            MSG(gFlyMode? "FLY TEST MODE - not part of the game (F3 to exit)"
                        : "fly mode off - back to the game", 2.5f);
        }

        if (!gPaused && gFlyMode){                        // inspect the level, no gameplay
            FlyUpdate(dt);
        } else if (!gPaused && gUnlockCine > 0){          // frozen in the unlock cinematic
            float el = CINE_DUR - gUnlockCine;
            pl.vel = {0,0,0}; pl.charging = false; pl.slamming = false; pl.webSwinging = false;
            if (el > 0.4f && el < 1.9f && GetRandomValue(0,100) < 55){   // sparkles rise from the bloom
                float a = frnd(0,6.283f), rr = frnd(0.3f,1.3f);
                spawnPart(gUnlockCinePos + (Vector3){cosf(a)*rr, frnd(-0.3f,0.7f), sinf(a)*rr},
                          {cosf(a)*1.4f, frnd(2.0f,5.0f), sinf(a)*1.4f}, 0.7f, 0.12f, -1.5f,
                          gUnlockCineType? (Color){255,150,90,255} : (Color){150,225,255,255});
            }
            if (el > 2.05f && el < 2.20f){                              // the power bursts into you
                gShake = fmaxf(gShake, 0.5f);
                for (int i=0;i<3;i++){ float a=frnd(0,6.283f);
                    spawnPart(pl.pos + (Vector3){0,0.5f,0}, {cosf(a)*6.0f,frnd(1,5),sinf(a)*6.0f},
                              0.6f, 0.14f, -3.0f, (i%2)? C_GOLD : WHITE); }
            }
        } else if (!gPaused){
            if (IsKeyDown(KEY_R)){
                gResetT += dt;
                if (gResetT >= 1.0f){ DoReset(); gResetT = -0.6f; }
            } else gResetT = 0;

            PlayerUpdate(dt, false);
            UpdateCoins(dt);
            UpdateSpores(dt);
            UpdateShrines();
            UpdateStreaks(dt);
            for (auto& wa : gWebAnchors) wa.wilt = fmaxf(0.0f, wa.wilt - dt);
            // the soundtrack thins out while you plummet - dread, audible
            float fallDrop = pl.apexY - pl.pos.y;
            float duckTgt = (!pl.grounded && !pl.webSwinging && fallDrop > 6.0f)? 0.22f : 1.0f;
            gMusicDuck += (duckTgt - gMusicDuck)*fminf(1.0f, rawDt*2.5f);
            if (gTimerStarted && !gWon) gTime += dt;
            gBestEver = fmaxf(gBestEver, pl.pos.y);

            if (!introA && t > 0.6f){ introA=true; MSG("SHROOMVAULT - climb to the * on the spire.", 5.0f); }
            if (!introB && t > 4.8f){ introB=true; MSG("Hold LMB: pole swings. Release in the GREEN = PERFECT vault.", 5.5f); }
            if (!introC && t > 10.2f){ introC=true; MSG("Sprint before you plant - speed becomes height. Flags mark the routes.", 5.0f); }
            static bool webTip = false;
            if (!webTip && WebSwingRangeFactor() > 0.25f){
                webTip = true; MSG("A web bloom! Hold RMB to swing from it - let go to keep the speed.", 5.5f);
            }
            for (int i=0;i<gTrigN;i++)
                if (!gTrigs[i].done && pl.pos.y > gTrigs[i].alt){ gTrigs[i].done = true; MSG(gTrigs[i].txt, 4.0f); }

            // standing on a warp pipe mouth opens the world-select menu
            {
                float wx = pl.pos.x - gWarpTop.x, wz = pl.pos.z - gWarpTop.z;
                gOnWarp = pl.grounded && wx*wx + wz*wz < 2.0f*2.0f
                       && fabsf((pl.pos.y - PLAYER_HH) - gWarpTop.y) < 0.5f;
                if (gOnWarp){
                    int pick = -1;
                    if (IsKeyPressed(KEY_ONE))   pick = 0;
                    if (IsKeyPressed(KEY_TWO))   pick = 1;
                    if (IsKeyPressed(KEY_THREE)) pick = 2;
                    if (IsKeyPressed(KEY_FOUR))  pick = 3;
                    if (IsKeyPressed(KEY_FIVE))  pick = 4;
                    if (IsKeyPressed(KEY_ENTER)) pick = (gLevel + 1) % 5;
                    if (pick >= 0 && pick != gLevel) GoToLevel(pick);
                }
            }

            Vector3 sb = gStarP; sb.y += sinf(t*1.6f)*0.35f;
            if (!gWon && Vector3Distance(pl.pos, sb) < STAR_WIN_R){
                gWon = true; gWins++; gWonMenu = true;
                if (gBestTime <= 0 || gTime < gBestTime) gBestTime = gTime;
                FX_Win(); SaveGame();
            }
            if (gWon && gWonMenu){                     // the three roads from the top
                if (IsKeyPressed(KEY_ONE))   { gWonMenu = false; DoReset(); }
                if (IsKeyPressed(KEY_TWO))   { gWonMenu = false; SwitchLevel(); }
                if (IsKeyPressed(KEY_THREE)) { gWonMenu = false; MSG("The view is yours. Warp pipes wait below.", 4.0f); }
            }
            gAutosaveT += dt;
            if (gAutosaveT > 8){ gAutosaveT = 0; SaveGame(); }
        }
        UpdateWind(Vector3Length(pl.vel));
        UpdateMusic(rawDt, pl.pos.y, gWon);
        UpdateParts(dt);
        UpdateMsgs(dt);
        if (gWon) ConfettiTick();

        BeginDrawing();
        RenderAll(GetCam(), t, true);
        if (gWon && gWonMenu && !gPaused && !gFlyMode) DrawWinPanel();
        if (gPaused) DrawPause();
        EndDrawing();
    }
    SaveGame();
    if (gAudioOK){
        if (gWindOn) UnloadAudioStream(gWind);
        Sound* snds[13] = {&sBoing,&sPlant,&sWhoosh,&sWhiff,&sFoul,&sTick,&sDing,
                           &sLand,&sStep,&sPop,&sWin,&sPerfect,&sThud};
        for (int i=0;i<13;i++) UnloadSound(*snds[i]);
        for (int i=0;i<3;i++) UnloadSound(gMusic[i]);
        CloseAudioDevice();
    }
    UnloadShader(gToon);
    for (int i=0;i<TX_COUNT;i++) UnloadTexture(gTex[i]);
    Model* mdls[5] = {&gCube,&gCyl,&gSphere,&gCone,&gPlane};
    for (int i=0;i<5;i++) UnloadModel(*mdls[i]);
    CloseWindow();
    return 0;
}
