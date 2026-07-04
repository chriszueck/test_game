// ============================================================================
// ShroomVault — a foddian first-person pole-vault ascent (C++ / raylib)
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
            spawnPart(STAR_POS + (Vector3){frnd(-1,1),frnd(-0.5f,1),frnd(-1,1)},
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
    SpawnDust((Vector3){pos.x, pos.y-PLAYER_HH, pos.z}, (int)clampf(4+impact*0.3f,4,16), {168,140,100,255});
    gShake += clampf(impact*0.015f, 0.0f, 0.45f);
}
void FX_Bounce(Vector3 pos, float speed){
    SND(sBoing, clampf(1.28f-speed*0.013f, 0.6f, 1.2f), 0.95f);
    gShake += 0.22f;
    SpawnDust((Vector3){pos.x, pos.y-PLAYER_HH, pos.z}, 8, WHITE);
    SpawnSparkle((Vector3){pos.x, pos.y-PLAYER_HH, pos.z}, 5);
    static bool tip = false;
    if (!tip){ tip = true; MSG("Boing! Red caps bounce you — aim for them when you fall.", 5.0f); }
}
static float gPerfT = 0;
void FX_Vault(Vector3 plantPos, float charge, bool perfect){
    SND(sPlant, 1.0f, 0.9f);
    SND(sWhoosh, 0.75f+0.5f*charge, 0.45f+0.5f*charge);
    SpawnDust(plantPos, 6+(int)(charge*8), {180,150,105,255});
    gShake += 0.12f + 0.15f*charge + (perfect? 0.20f : 0.0f);
    if (perfect){
        gPerfT = 0.8f;
        SND(sDing, 1.5f, 0.85f);
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
void FX_Win(){
    SND(sWin, 1.0f, 0.95f);
    MSG("* YOU VAULTED THE CASTLE! *", 6.0f);
}

// -------------------------------------------------------------- save file --
static void SaveGame(void){
    char buf[512];
    snprintf(buf, sizeof(buf),
        "bestAlt=%.2f\nbestTime=%.2f\nwins=%d\ntotFalls=%d\ntotVaults=%d\n"
        "resume=%d\npx=%.2f\npy=%.2f\npz=%.2f\nyaw=%.4f\npitch=%.4f\ntime=%.2f\n",
        gBestEver, gBestTime, gWins, gTotFalls, gTotVaults,
        gWon? 0:1, pl.pos.x, pl.pos.y, pl.pos.z, pl.yaw, pl.pitch, gTime);
    SaveFileText(gSavePath, buf);
}
static float readKey(const char* txt, const char* key, float def){
    char pat[64]; snprintf(pat, sizeof(pat), "%s=", key);
    const char* p = strstr(txt, pat);
    if (!p) return def;
    return (float)atof(p + strlen(pat));
}
static void LoadSave(void){
    char* txt = LoadFileText(gSavePath);
    if (!txt) return;
    gBestEver  = readKey(txt,"bestAlt",0);
    gBestTime  = readKey(txt,"bestTime",0);
    gWins      = (int)readKey(txt,"wins",0);
    gTotFalls  = (int)readKey(txt,"totFalls",0);
    gTotVaults = (int)readKey(txt,"totVaults",0);
    if (readKey(txt,"resume",0) > 0.5f){
        pl.pos = { readKey(txt,"px",SPAWN_POS.x), readKey(txt,"py",SPAWN_POS.y), readKey(txt,"pz",SPAWN_POS.z) };
        pl.yaw = readKey(txt,"yaw",0); pl.pitch = clampf(readKey(txt,"pitch",0.05f), -1.55f, 1.55f);
        gTime = fmaxf(0.0f, readKey(txt,"time",0));
        gTimerStarted = gTime > 0.01f;
        if (pl.pos.y < 0.85f || fabsf(pl.pos.x) > 214 || pl.pos.z < -74 || pl.pos.z > 274)
            pl.pos = SPAWN_POS;                       // mangled save: back to the meadow
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
    // crosshair
    DrawCircle(W/2, H/2, 3.2f, (Color){20,20,28,120});
    DrawCircle(W/2, H/2, 2.0f, (Color){255,255,255,220});
    DrawChargeGauge();
    if (gPerfT > 0){
        gPerfT -= GetFrameTime();
        if (gPerfT > 0.62f)     // one golden flash frame set
            DrawRectangle(0,0,W,H,(Color){255,222,95,(unsigned char)(52*clampf((gPerfT-0.62f)/0.18f,0,1))});
        float a = clampf(gPerfT/0.8f, 0, 1);
        int sz = 30 + (int)(8*(1.0f-a));
        drawTextC("PERFECT!", W/2, H/2 - 90, sz, (Color){255,215,60,(unsigned char)(255*a)});
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
    // early controls hint
    if (t < 22 && !gWon)
        drawTextC("WASD run  ·  hold LMB / SPACE to swing pole, release to vault  ·  ESC pause",
                  W/2, H-30, 18, (Color){255,255,255,150});
    if (gResetT > 0.04f){
        DrawRing((Vector2){(float)W/2,(float)H/2}, 24, 32, 0, 360*clampf(gResetT,0,1), 40, (Color){253,192,40,220});
        drawTextC("hold to reset…", W/2, H/2+44, 18, (Color){253,210,90,220});
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
    DrawPanelBG(640, 430);
    int y = (H-430)/2 + 28;
    drawTextC("SHROOMVAULT", W/2, y, 40, C_GOLD); y += 46;
    drawTextC("a foddian pole-vaulting ascent", W/2, y, 18, (Color){220,220,235,200}); y += 40;
    const char* lines[] = {
        "WASD run   ·   mouse look",
        "hold LMB or SPACE — pole swings down as it charges",
        "release to PLANT & LAUNCH — sprint first: speed becomes height",
        "release in the bright GREEN = PERFECT (+15%) — hold past it: FOUL",
        "red mushrooms are trampolines — the star itself sits on one",
        "?-blocks pop a coin on first touch. many roads up, no checkpoints",
        "",
        "R (hold 1s) restart run   ·   M mute   ·   F11 fullscreen",
        "ESC resume   ·   Q save & quit" };
    for (auto s : lines){ drawTextC(s, W/2, y, 19, RAYWHITE); y += 28; }
    y += 8;
    if (gBestTime > 0)
        drawTextC(TextFormat("best climb %02d:%05.2f  ·  wins %d  ·  lifetime falls %d",
                  (int)(gBestTime/60), fmodf(gBestTime,60), gWins, gTotFalls), W/2, y, 17, (Color){253,210,90,220});
    else
        drawTextC(TextFormat("best altitude %d m  ·  lifetime falls %d  ·  the star waits",
                  (int)gBestEver, gTotFalls), W/2, y, 17, (Color){253,210,90,220});
}
static void DrawWinPanel(void){
    int W = GetScreenWidth(), H = GetScreenHeight();
    DrawPanelBG(600, 330);
    int y = (H-330)/2 + 30;
    drawTextC("* YOU VAULTED THE CASTLE *", W/2, y, 34, C_GOLD); y += 56;
    drawTextC(TextFormat("time  %02d:%05.2f", (int)(gTime/60), fmodf(gTime,60)), W/2, y, 30, RAYWHITE); y += 42;
    if (gBestTime > 0)
        drawTextC(TextFormat("best  %02d:%05.2f", (int)(gBestTime/60), fmodf(gBestTime,60)), W/2, y, 20, (Color){230,230,240,200});
    y += 36;
    drawTextC(TextFormat("%d vaults  ·  %d fouls  ·  %d falls  ·  %d/%d coins",
              pl.vaults, pl.fouls, pl.falls, gCoinCount, (int)gCoins.size()+gQTotal), W/2, y, 20, RAYWHITE); y += 44;
    drawTextC("hold R to run it back  ·  or stay and enjoy the view", W/2, y, 19, (Color){253,210,90,230});
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
        DrawWorld3D(cam.position);
        DrawCoins(t);
        DrawStar(t, gWon);
        DrawGoalBeam(t);
        DrawParts3D();
        DrawStreaks();
        DrawBlobShadow(pl.pos);
        rlDrawRenderBatchActive();
        glClear(GLX_DEPTH_BUFFER_BIT);          // pole never clips into walls
        DrawPoleFP(cam, t);
    EndMode3D();
    if (hud) DrawHUD(t);
}

// ------------------------------------------------------- screenshot harness
static void ShotMode(void){
    struct { Vector3 pos; float yawD, pitchD, charge; const char* file; } P[5] = {
        {{0,0.91f,-8},      0,   6, 0,     "shot1.png"},
        {{-18,15.91f,56},  22,   4, 0,     "shot2.png"},
        {{5,0.91f,17},      0,   2, 0.85f, "shot3.png"},
        {{10,70.0f,134},  192, -20, 0,     "shot4.png"},
        {{-7,88.4f,147.5f},145, 14, 0,     "shot5.png"},
    };
    for (int i=0;i<5;i++){
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
    int rf = 0; bool tookShot = false;
    while (T < 6.0f && !WindowShouldClose()){
        gBotFwd  = (T > 0.4f && T < 2.2f) || (T > 3.4f && T < 4.0f);
        gBotHold = (T > 1.05f && T < 2.2f) || (T > 3.5f);   // vault #1, then over-hold -> FOUL
        PlayerUpdate(dt, false);
        maxY = fmaxf(maxY, pl.pos.y);
        T += dt;
        if (lf && ((int)(T*10) != (int)((T-dt)*10)))
            fprintf(lf, "t=%.1f pos=(%6.2f,%6.2f,%6.2f) vel=(%5.1f,%5.1f,%5.1f) gnd=%d chg=%d vaults=%d fouls=%d\n",
                    T, pl.pos.x, pl.pos.y, pl.pos.z, pl.vel.x, pl.vel.y, pl.vel.z,
                    pl.grounded?1:0, pl.charging?1:0, pl.vaults, pl.fouls);
        if ((rf++ & 1) == 0){
            BeginDrawing();
            RenderAll(GetCam(), T, true);
            EndDrawing();
        }
        if (!tookShot && T > 2.6f){ TakeScreenshot("demo_flight.png"); tookShot = true; }
    }
    gBotFwd = gBotHold = false;
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
    gWon = false; gTime = 0; gTimerStarted = false;
    for (auto& c : gCoins) c.taken = false;        // coins respawn with the run
    for (auto& s : solids) s.used = false;         // ?-blocks refill
    gCoinCount = 0; gCoinCombo = 0; gCoinPops.clear();
    gMsgs.clear();
    MSG(hadWon? "Round two. The castle is not impressed." : "Back to the meadow. Breathe.", 3.5f);
    SaveGame();
}

// -------------------------------------------------------------------- main -
static struct { float alt; const char* txt; bool done; } gTrigs[] = {
    {15.8f, "The Ramparts! Three ways up here — three ways onward.", false},
    {24.5f, "Blocks, beams or the big shrooms. Pick a lane.", false},
    {36.0f, "The Keep face. Hug the wall, it forgives overshoots.", false},
    {45.8f, "Keep roof! Pegs on the right, broken stair on the left.", false},
    {62.0f, "Cloudwalk. Sprint, PERFECT, pray.", false},
    {83.6f, "THE SPIRE. The star sits over a giant red cap — bounce through it!", false},
};

int main(int argc, char** argv){
    bool shot = (argc > 1) && (strcmp(argv[1], "--shot") == 0);
    SetTraceLogLevel(LOG_WARNING);
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(1600, 900, "ShroomVault — a foddian pole vault");
    SetExitKey(KEY_NULL);
    SetTargetFPS(240);
    snprintf(gSavePath, sizeof(gSavePath), "%sshroomvault_save.txt", GetApplicationDirectory());
    LoadGfx(); InitSky(); BuildWorld();
    for (auto& s : solids) if (s.surf == S_QBLOCK) gQTotal++;

    if (shot){ ShotMode(); CloseWindow(); return 0; }
    if (argc > 1 && strcmp(argv[1], "--demo") == 0){ DemoMode(); CloseWindow(); return 0; }

    LoadAudioAll(); InitWind(); LoadSave();
    DisableCursor();

    bool introA=false, introB=false, introC=false;
    while (!WindowShouldClose() && !gQuit){
        float t = (float)GetTime();
        float rawDt = clampf(GetFrameTime(), 0, 0.05f);
        float dt = rawDt;
        if (gHitstop > 0){ gHitstop -= rawDt; dt *= 0.12f; }   // impact frames
        gShake *= expf(-8.0f*rawDt);

        if (IsKeyPressed(KEY_ESCAPE)){
            gPaused = !gPaused;
            if (gPaused) EnableCursor(); else { DisableCursor(); gSkipLook = 2; }
        }
        if (!IsWindowFocused() && !gPaused){ gPaused = true; EnableCursor(); }
        if (IsKeyPressed(KEY_M)) gMuted = !gMuted;
        if (IsKeyPressed(KEY_F11)) ToggleBorderlessWindowed();
        if (gPaused && IsKeyPressed(KEY_Q)) gQuit = true;

        if (!gPaused){
            if (IsKeyDown(KEY_R)){
                gResetT += dt;
                if (gResetT >= 1.0f){ DoReset(); gResetT = -0.6f; }
            } else gResetT = 0;

            PlayerUpdate(dt, false);
            UpdateCoins(dt);
            UpdateStreaks(dt);
            if (gTimerStarted && !gWon) gTime += dt;
            gBestEver = fmaxf(gBestEver, pl.pos.y);

            if (!introA && t > 0.6f){ introA=true; MSG("SHROOMVAULT — climb to the * on the spire.", 5.0f); }
            if (!introB && t > 4.8f){ introB=true; MSG("Hold LMB: pole swings. Release in the GREEN = PERFECT vault.", 5.5f); }
            if (!introC && t > 10.2f){ introC=true; MSG("Sprint before you plant — speed becomes height. Flags mark the routes.", 5.0f); }
            for (auto& tr : gTrigs)
                if (!tr.done && pl.pos.y > tr.alt){ tr.done = true; MSG(tr.txt, 4.0f); }

            Vector3 sb = STAR_POS; sb.y += sinf(t*1.6f)*0.35f;
            if (!gWon && Vector3Distance(pl.pos, sb) < STAR_WIN_R){
                gWon = true; gWins++;
                if (gBestTime <= 0 || gTime < gBestTime) gBestTime = gTime;
                FX_Win(); SaveGame();
            }
            gAutosaveT += dt;
            if (gAutosaveT > 8){ gAutosaveT = 0; SaveGame(); }
        }
        UpdateWind(Vector3Length(pl.vel));
        UpdateParts(dt);
        UpdateMsgs(dt);
        if (gWon) ConfettiTick();

        BeginDrawing();
        RenderAll(GetCam(), t, true);
        if (gWon && !gPaused) DrawWinPanel();
        if (gPaused) DrawPause();
        EndDrawing();
    }
    SaveGame();
    if (gAudioOK) CloseAudioDevice();
    CloseWindow();
    return 0;
}
