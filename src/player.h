// ============================================================================
// player.h : first-person controller, collision, and THE POLE VAULT
// ============================================================================
#pragma once
#include "game.h"
#include "world.h"
#include "gfx.h"
#include "audio.h"

struct Player {
    Vector3 pos = SPAWN_POS, vel = {0,0,0};
    float yaw = 0, pitch = 0.05f;            // yaw 0 faces +Z (toward the castle)
    bool  grounded = false;
    float coyote = 0;
    // vault
    bool  charging = false;
    float chargeT = 0, vaultCD = 0, poleAngle = 0, poleHideT = 0;
    int   lastQuarter = 0;
    float pendC = 0, pendT = 0; bool pendPerf = false;   // buffered early release
    float dangerT = 0;
    Vector3 plantPos = {0,0,0}; float plantT = -10, plantYaw = 0;
    // camera feel
    float eyeDip = 0, eyeDipVel = 0, bobPhase = 0, roll = 0, rollT = 99, fovKick = 0;
    float pitchKick = 0, crouch = 0;         // launch head-throw + charge crouch
    // progress
    float apexY = 0; bool wasGrounded = true;
    float stepT = 0; int stepAlt = 0;
    int   vaults = 0, fouls = 0, falls = 0; float fallen = 0;
};
static Player pl;

static inline Vector3 yawFwd(float yaw){ return { sinf(yaw), 0, cosf(yaw) }; }
static inline Vector3 lookDir(float yaw, float pitch){
    return { sinf(yaw)*cosf(pitch), sinf(pitch), cosf(yaw)*cosf(pitch) };
}

// --------------------------------------------------------------- collide ---
static inline bool aabbOverlap(Vector3 mn, Vector3 mx, const Solid& s){
    return mn.x < s.mx.x && mx.x > s.mn.x && mn.y < s.mx.y && mx.y > s.mn.y
        && mn.z < s.mx.z && mx.z > s.mn.z;
}
static bool freeAt(Vector3 p){                       // headroom test for step-up
    Vector3 mn = {p.x-PLAYER_R, p.y-PLAYER_HH, p.z-PLAYER_R};
    Vector3 mx = {p.x+PLAYER_R, p.y+PLAYER_HH, p.z+PLAYER_R};
    for (const Solid& s : solids) if (aabbOverlap(mn,mx,s)) return false;
    return true;
}
static void moveHoriz(int axis, float d){            // axis 0=x, 2=z (boxes only)
    if (d == 0) return;
    float* pc = (axis==0)? &pl.pos.x : &pl.pos.z;
    float* vc = (axis==0)? &pl.vel.x : &pl.vel.z;
    *pc += d;
    for (const Solid& s : solids){
        if (s.isCyl) continue;
        Vector3 mn = {pl.pos.x-PLAYER_R, pl.pos.y-PLAYER_HH, pl.pos.z-PLAYER_R};
        Vector3 mx = {pl.pos.x+PLAYER_R, pl.pos.y+PLAYER_HH, pl.pos.z+PLAYER_R};
        if (!aabbOverlap(mn,mx,s)) continue;
        float feet = pl.pos.y - PLAYER_HH;
        if (pl.grounded && s.mx.y > feet && s.mx.y - feet <= STEP_UP){   // auto-step small lips
            Vector3 up = pl.pos; up.y = s.mx.y + PLAYER_HH + 0.002f;
            if (freeAt(up)){ pl.pos.y = up.y; continue; }
        }
        float smn = (axis==0)? s.mn.x : s.mn.z;
        float smx = (axis==0)? s.mx.x : s.mx.z;
        *pc = (d > 0)? smn - PLAYER_R - 0.001f : smx + PLAYER_R + 0.001f;
        *vc = 0;
    }
}
static void resolveCylsRadial(void){
    float feet = pl.pos.y - PLAYER_HH, head = pl.pos.y + PLAYER_HH;
    for (const Solid& s : solids){
        if (!s.isCyl) continue;
        float top = s.base.y + s.hgt;
        if (feet >= top - 0.01f || head <= s.base.y + 0.01f) continue;
        float dx = pl.pos.x - s.base.x, dz = pl.pos.z - s.base.z;
        float minD = s.rad + PLAYER_R;
        float d2 = dx*dx + dz*dz;
        if (d2 >= minD*minD) continue;
        float d = sqrtf(d2);
        float ox, oz;
        if (d < 0.001f){ ox = 1; oz = 0; } else { ox = dx/d; oz = dz/d; }
        pl.pos.x = s.base.x + ox*(minD + 0.001f);
        pl.pos.z = s.base.z + oz*(minD + 0.001f);
        float vd = pl.vel.x*ox + pl.vel.z*oz;
        if (vd < 0){ pl.vel.x -= ox*vd; pl.vel.z -= oz*vd; }
    }
}
static float gLandImpact = 0;                 // set during Y-resolve, consumed after
static bool  gLandedSolidGround = false;
static void moveVert(float d){
    pl.pos.y += d;
    float feet = pl.pos.y - PLAYER_HH, head = pl.pos.y + PLAYER_HH;
    for (Solid& s : solids){
        // horizontal overlap?
        if (s.isCyl){
            float dx = pl.pos.x - s.base.x, dz = pl.pos.z - s.base.z;
            float lim = s.rad + 0.15f;        // must be mostly over a round top
            if (dx*dx + dz*dz > lim*lim) continue;
        } else {
            if (pl.pos.x + PLAYER_R < s.mn.x || pl.pos.x - PLAYER_R > s.mx.x) continue;
            if (pl.pos.z + PLAYER_R < s.mn.z || pl.pos.z - PLAYER_R > s.mx.z) continue;
        }
        float sTop = s.isCyl? s.base.y + s.hgt : s.mx.y;
        float sBot = s.isCyl? s.base.y         : s.mn.y;
        if (pl.vel.y <= 0.01f){
            float pen = sTop - feet;
            if (pen > 0 && pen <= 0.34f && head > sTop){
                pl.pos.y = sTop + PLAYER_HH + 0.001f;
                feet = sTop + 0.001f; head = pl.pos.y + PLAYER_HH;
                if (s.bouncy && pl.vel.y < -2.0f){
                    float out = clampf(-pl.vel.y*BOUNCE_K, BOUNCE_MIN, BOUNCE_MAX);
                    pl.vel.y = out;
                    pl.vel.x *= 0.75f; pl.vel.z *= 0.75f;      // trampoline absorbs the slide
                    FX_Bounce(pl.pos, out);
                } else {
                    gLandImpact = fmaxf(gLandImpact, -pl.vel.y);
                    pl.vel.y = 0;
                    gLandedSolidGround = true;
                    if (s.surf == S_QBLOCK && !s.used){        // ?-blocks pay out on first touch
                        s.used = true;
                        FX_QCoin((Vector3){(s.mn.x+s.mx.x)*0.5f, s.mx.y, (s.mn.z+s.mx.z)*0.5f});
                    }
                }
            }
        } else {
            float pen = head - sBot;          // rising into a ceiling
            if (pen > 0 && pen <= 0.34f && feet < sBot){
                pl.pos.y = sBot - PLAYER_HH - 0.001f;
                feet = pl.pos.y - PLAYER_HH; head = sBot - 0.001f;
                if (pl.vel.y > 4) FX_Bump((Vector3){pl.pos.x, head, pl.pos.z}, s.surf);
                if (s.surf == S_QBLOCK && !s.used){            // classic bonk from below
                    s.used = true;
                    FX_QCoin((Vector3){(s.mn.x+s.mx.x)*0.5f, s.mx.y, (s.mn.z+s.mx.z)*0.5f});
                }
                pl.vel.y = -0.5f;
            }
        }
    }
}
static bool supportScan(void){
    float feet = pl.pos.y - PLAYER_HH;
    for (const Solid& s : solids){
        float sTop = s.isCyl? s.base.y + s.hgt : s.mx.y;
        if (feet - sTop > 0.08f || sTop - feet > 0.05f) continue;
        if (s.isCyl){
            float dx = pl.pos.x - s.base.x, dz = pl.pos.z - s.base.z;
            float lim = s.rad + 0.15f;
            if (dx*dx + dz*dz > lim*lim) continue;
        } else {
            if (pl.pos.x + PLAYER_R < s.mn.x || pl.pos.x - PLAYER_R > s.mx.x) continue;
            if (pl.pos.z + PLAYER_R < s.mn.z || pl.pos.z - PLAYER_R > s.mx.z) continue;
        }
        if (s.bouncy) continue;               // can't stand on trampolines
        return true;
    }
    return false;
}

// ----------------------------------------------------------------- vault ---
static bool gTimerStarted = false;
static void DoVault(float c, bool perfect){
    float power = c * (perfect? PERFECT_MULT : 1.0f);
    Vector3 f = yawFwd(pl.yaw);
    float sprint = fmaxf(0.0f, pl.vel.x*f.x + pl.vel.z*f.z);   // speed toward look dir
    pl.vel.x = pl.vel.x*VAULT_KEEP + f.x*(VAULT_FWD0 + VAULT_FWD1*power);
    pl.vel.z = pl.vel.z*VAULT_KEEP + f.z*(VAULT_FWD0 + VAULT_FWD1*power);
    pl.vel.y = VAULT_VY0 + VAULT_VY1*power + MOMENTUM_K*sprint*power;
    pl.fovKick = 6.0f + 5.0f*power + (perfect? 2.5f : 0.0f);
    pl.pitchKick = 0.09f + 0.07f*power;                       // head thrown back
    gHitstop = perfect? HITSTOP_PERFECT : HITSTOP_VAULT;      // impact frame
    pl.grounded = false; pl.coyote = 0;
    pl.plantPos = pl.pos + f*1.9f;
    float g = GroundTopBelow((Vector3){pl.plantPos.x, pl.pos.y, pl.plantPos.z});
    pl.plantPos.y = (g > -900)? g : pl.pos.y - PLAYER_HH;
    pl.plantT = (float)GetTime(); pl.plantYaw = pl.yaw;
    pl.poleHideT = 0.55f;
    pl.vaults++; gTimerStarted = true;
    FX_Vault(pl.plantPos, power, perfect);
}
static void DoFoul(void){
    Vector3 f = yawFwd(pl.yaw);
    if (pl.grounded || pl.coyote > 0){
        pl.vel.x = pl.vel.x*0.6f + f.x*2.0f;
        pl.vel.z = pl.vel.z*0.6f + f.z*2.0f;
        pl.vel.y = 4.5f;
        pl.grounded = false; pl.coyote = 0;
    }
    pl.rollT = 0; pl.fouls++; pl.poleHideT = 0.5f;
    FX_Foul();
}

// ------------------------------------------------------------ per frame ----
static int gSkipLook = 2;
static void PlayerUpdate(float dt, bool inputLocked){
    // ---- look
    if (gSkipLook > 0) { GetMouseDelta(); gSkipLook--; }
    else if (!inputLocked){
        Vector2 md = GetMouseDelta();
        // cursor-recapture spikes (startup, alt-tab) are not look input
        if (fabsf(md.x) > 350 || fabsf(md.y) > 350) md = (Vector2){0,0};
        pl.yaw   -= md.x*0.0021f;      // yaw decreases turning right (+Z faces -X-right)
        pl.pitch -= md.y*0.0021f;
        pl.pitch  = clampf(pl.pitch, -1.55f, 1.55f);
    }
    // ---- run
    Vector3 f = yawFwd(pl.yaw), r = { -f.z, 0, f.x };   // r = screen-right = cross(f, up)
    Vector3 wish = {0,0,0};
    if (!inputLocked){
        if (IsKeyDown(KEY_W) || gBotFwd) wish = wish + f;
        if (IsKeyDown(KEY_S)) wish = wish - f;
        if (IsKeyDown(KEY_D)) wish = wish + r;
        if (IsKeyDown(KEY_A)) wish = wish - r;
    }
    float wl = Vector3Length(wish);
    if (wl > 0.01f){ wish = wish*(1.0f/wl); gTimerStarted = true; }
    if (pl.grounded){
        float tx = wish.x*RUN_SPEED, tz = wish.z*RUN_SPEED;
        pl.vel.x = pl.vel.x + clampf(tx - pl.vel.x, -GROUND_ACC*dt, GROUND_ACC*dt);
        pl.vel.z = pl.vel.z + clampf(tz - pl.vel.z, -GROUND_ACC*dt, GROUND_ACC*dt);
    } else if (wl > 0.01f){
        pl.vel.x += wish.x*AIR_ACC*dt;
        pl.vel.z += wish.z*AIR_ACC*dt;
    }
    // ---- vault charge
    bool hold = !inputLocked && (IsMouseButtonDown(MOUSE_BUTTON_LEFT) || IsKeyDown(KEY_SPACE) || gBotHold);
    pl.vaultCD = fmaxf(0.0f, pl.vaultCD - dt);
    if (hold && !pl.charging && pl.vaultCD <= 0){
        pl.charging = true; pl.chargeT = 0; pl.lastQuarter = 0;
    }
    if (pl.charging){
        pl.chargeT += dt;
        float c = fminf(pl.chargeT/CHARGE_FULL, 1.0f);
        pl.poleAngle = c*90.0f + fmaxf(0.0f,(pl.chargeT-CHARGE_FULL)/OVERSHOOT)*15.0f;
        int q = (int)(c*4);
        if (c >= 1.0f && pl.lastQuarter < 4){ pl.lastQuarter = 4; FX_ChargeTick(4); }
        else if (q > pl.lastQuarter && q < 4){ pl.lastQuarter = q; FX_ChargeTick(q); }
        if (pl.chargeT > CHARGE_FULL + PERFECT_WIN){        // rapid ticks: foul is near
            pl.dangerT -= dt;
            if (pl.dangerT <= 0){ pl.dangerT = 0.06f; SND(sTick, 1.5f, 0.5f); }
        }
        if (pl.chargeT > CHARGE_FULL + OVERSHOOT){          // held too long!
            pl.charging = false; pl.vaultCD = 0.45f;
            DoFoul();
        } else if (!hold){
            pl.charging = false; pl.vaultCD = 0.22f;
            float cc = fminf(pl.chargeT/CHARGE_FULL, 1.0f);
            bool perfect = pl.chargeT >= CHARGE_FULL && pl.chargeT <= CHARGE_FULL + PERFECT_WIN;
            if (pl.chargeT >= 0.06f){
                if (pl.grounded || pl.coyote > 0) DoVault(cc, perfect);
                else if (pl.chargeT >= 0.25f){               // buffer: plant on touchdown
                    pl.pendC = cc; pl.pendPerf = perfect; pl.pendT = BUFFER_T;
                }
            }
        }
    }
    if (!pl.charging) pl.poleAngle = fmaxf(0.0f, pl.poleAngle - dt*420.0f);
    // ---- gravity + integrate with substeps (fast falls can't tunnel)
    pl.vel.y -= (pl.vel.y > 0 ? GRAV_UP : GRAV_DOWN)*dt;
    pl.vel.y = fmaxf(pl.vel.y, -TERMINAL);
    gLandImpact = 0; gLandedSolidGround = false;
    Vector3 disp = pl.vel*dt;
    float maxd = fmaxf(fabsf(disp.x), fmaxf(fabsf(disp.y), fabsf(disp.z)));
    int steps = (int)ceilf(maxd/0.28f); if (steps < 1) steps = 1; if (steps > 48) steps = 48;
    for (int i=0;i<steps;i++){
        moveHoriz(0, disp.x/steps);
        moveHoriz(2, disp.z/steps);
        resolveCylsRadial();
        moveVert(disp.y/steps);
    }
    // ---- grounded state
    bool sup = supportScan() && pl.vel.y <= 0.01f;
    pl.grounded = sup;
    if (sup) pl.coyote = COYOTE; else pl.coyote -= dt;
    if (!sup) pl.apexY = fmaxf(pl.apexY, pl.pos.y);
    if (sup && !pl.wasGrounded){                       // landing event
        float drop = pl.apexY - pl.pos.y;
        if (gLandImpact > 3.0f) FX_Land(pl.pos, gLandImpact);
        if (drop > 12.0f){ pl.falls++; pl.fallen += drop; FX_BigFall(drop); }
        pl.apexY = pl.pos.y;
        pl.eyeDipVel -= clampf(gLandImpact*0.05f, 0.0f, 1.6f);
        if (gLandImpact > 9.0f){                       // stick big landings (fairness);
            pl.vel.x *= LAND_STICK; pl.vel.z *= LAND_STICK;   // small hops keep their flow
        }
        if (pl.pendT > 0){                             // buffered release: plant NOW
            pl.pendT = 0; pl.coyote = COYOTE;
            DoVault(pl.pendC, pl.pendPerf);
        }
    } else if (pl.pendT > 0){
        pl.pendT -= dt;
        if (pl.pendT <= 0) FX_Whiff();                 // buffer expired mid-air
    }
    if (sup) pl.apexY = pl.pos.y;
    pl.wasGrounded = sup;
    // ---- ground friction when idle
    if (pl.grounded && wl <= 0.01f){
        float sp = sqrtf(pl.vel.x*pl.vel.x + pl.vel.z*pl.vel.z);
        float ns = fmaxf(0.0f, sp - 40.0f*dt);
        if (sp > 0.01f){ pl.vel.x *= ns/sp; pl.vel.z *= ns/sp; }
    }
    // ---- camera springs
    pl.eyeDipVel += (-140.0f*pl.eyeDip - 16.0f*pl.eyeDipVel)*dt;
    pl.eyeDip    += pl.eyeDipVel*dt;
    float hsp = sqrtf(pl.vel.x*pl.vel.x + pl.vel.z*pl.vel.z);
    if (pl.grounded && hsp > 1.5f) pl.bobPhase += dt*hsp*1.35f;
    pl.rollT += dt;
    pl.roll = (pl.rollT < 1.2f)? sinf(pl.rollT*16.0f)*9.0f*expf(-pl.rollT*3.5f) : 0.0f;
    pl.fovKick *= expf(-6.0f*dt);
    pl.pitchKick *= expf(-5.0f*dt);
    float crouchTgt = pl.charging? 0.24f*fminf(pl.chargeT/CHARGE_FULL,1.0f) : 0.0f;
    pl.crouch += (crouchTgt - pl.crouch)*fminf(1.0f, dt*10.0f);   // sink while loading the spring
    pl.poleHideT = fmaxf(0.0f, pl.poleHideT - dt);
    // ---- footsteps
    if (pl.grounded && hsp > 2.0f){
        pl.stepT += hsp*dt;
        if (pl.stepT > 2.7f){ pl.stepT = 0; pl.stepAlt ^= 1; SND(sStep, pl.stepAlt? 1.07f:0.93f, 0.5f); }
    } else pl.stepT = 1.8f;
    // ---- world bounds / failsafe
    pl.pos.x = clampf(pl.pos.x, -215, 215);
    pl.pos.z = clampf(pl.pos.z, -75, 275);
    if (pl.pos.y < -25){ pl.pos = SPAWN_POS; pl.vel = {0,0,0}; }
}

// ---------------------------------------------------------------- camera ---
static Camera3D GetCam(void){
    Camera3D cam{};
    float bob = pl.grounded? sinf(pl.bobPhase)*0.045f*clampf(Vector3Length(pl.vel)/RUN_SPEED,0,1) : 0;
    cam.position = { pl.pos.x, pl.pos.y + EYE_OFF + bob + pl.eyeDip - pl.crouch, pl.pos.z };
    Vector3 dir = lookDir(pl.yaw, clampf(pl.pitch + pl.pitchKick, -1.55f, 1.55f));
    cam.target = cam.position + dir;
    cam.up = Vector3RotateByAxisAngle((Vector3){0,1,0}, dir, pl.roll*DEG2RAD);
    float spd = Vector3Length(pl.vel);
    float c = pl.charging? fminf(pl.chargeT/CHARGE_FULL,1.0f) : 0;
    cam.fovy = 74.0f + clampf((spd-9.0f)/32.0f, 0, 1)*9.0f - c*5.0f + pl.fovKick;
    cam.projection = CAMERA_PERSPECTIVE;
    return cam;
}

// ------------------------------------------------------------- pole draw ---
static void DrawPoleStripes(Vector3 bottom, Vector3 dir, float len, float rad, float alpha){
    int nseg = 8;
    float seg = len/nseg;
    for (int i=0;i<nseg;i++){
        Color col = (i%2)? (Color){240,240,236,255} : (Color){228,44,34,255};
        col.a = (unsigned char)(255*alpha);
        drawCylBetween(bottom + dir*(i*seg), bottom + dir*((i+1)*seg), rad, col);
    }
    // dark steel tip at the bottom end
    Vector3 axis = Vector3CrossProduct((Vector3){0,1,0}, dir*-1.0f);
    float ang = RAD2DEG*acosf(clampf(-dir.y,-1,1));
    if (Vector3Length(axis)<0.001f) axis = {1,0,0};
    Color tipc = {60,60,72,(unsigned char)(255*alpha)};
    drawM(gCone, bottom, {rad*1.8f, 0.22f, rad*1.8f}, tipc, TX_WHITE, axis, ang);
}
static void DrawPoleFP(const Camera3D& cam, float t){
    // ghost of the planted pole you just launched off
    float ga = 1.0f - (t - pl.plantT)/0.5f;
    if (ga > 0 && ga < 1){
        Vector3 f = yawFwd(pl.plantYaw);
        Vector3 lean = Vector3Normalize((Vector3){ -f.x*0.4f, 1.0f, -f.z*0.4f });
        DrawPoleStripes(pl.plantPos, lean, 4.0f, 0.045f, ga*0.9f);
    }
    if (pl.poleHideT > 0) return;
    Vector3 fwd = Vector3Normalize(cam.target - cam.position);
    Vector3 right = Vector3Normalize(Vector3CrossProduct(fwd, (Vector3){0,1,0}));   // screen-right
    Vector3 grip = cam.position + fwd*0.68f + right*0.44f + (Vector3){0,-0.46f,0};
    Vector3 rYaw = { fwd.z, 0, -fwd.x };
    float rl = Vector3Length(rYaw);
    rYaw = (rl>0.01f)? rYaw*(1.0f/rl) : (Vector3){1,0,0};
    float sway = sinf(pl.bobPhase*0.5f)*1.5f;
    float ang = (4.0f + pl.poleAngle + sway);
    // danger shake near foul
    if (pl.charging && pl.chargeT > CHARGE_FULL)
        ang += sinf((float)GetTime()*70)*1.6f;
    Vector3 dir = Vector3RotateByAxisAngle((Vector3){0,1,0}, rYaw, ang*DEG2RAD);
    // at rest the pole leans out to the right so it doesn't block the view
    float lean = 16.0f*(1.0f - clampf(pl.poleAngle/90.0f, 0, 1));
    dir = Vector3RotateByAxisAngle(dir, fwd, lean*DEG2RAD);
    Vector3 bottom = grip - dir*1.25f;
    DrawPoleStripes(bottom, dir, 4.0f, 0.032f, 1.0f);
    // white cartoon gloves
    drawM(gSphere, grip + dir*0.09f, {0.062f,0.062f,0.062f}, WHITE, TX_WHITE);
    drawM(gSphere, grip - dir*0.13f, {0.058f,0.058f,0.058f}, WHITE, TX_WHITE);
}
