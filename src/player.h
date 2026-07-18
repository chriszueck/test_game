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
    bool  planting = false;                  // pole is bending: launch is imminent
    float plantDelay = 0, plantDelayMax = 0, plantC = 0, plantSprint = 0; bool plantPerf = false;
    Vector3 plantPos = {0,0,0}; float plantT = -10, plantYaw = 0;
    // camera feel
    float eyeDip = 0, eyeDipVel = 0, bobPhase = 0, roll = 0, rollT = 99, fovKick = 0;
    float pitchKick = 0, crouch = 0;         // launch head-throw + charge crouch
    // web swing (rope constraint to a bloom anchor)
    bool webSwinging = false, webToggled = false;
    Vector3 webAnchor = {0,0,0};
    float webLength = 0, webT = 0;
    int   webIdx = -1;
    // slam dive
    bool  slamming = false;
    float slamT = 0, stunT = 0;
    // skysail hang-glider
    bool  sailing = false;
    float sailOpen = 0, sailTilt = 0;      // 0..1 deploy, -1..1 dive/flare
    float sailT = 0;                       // seconds this sail has been held open
    bool  inUpdraft = false;
    // wallspring (Bonewood): recent wall contact + buffered press
    float wallT = 0;                       // grace window since last wall touch
    Vector3 wallN = {0,0,0};               // normal of that wall (points away)
    float kickBuf = 0;                     // press buffered before the touch
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
static inline float WebSwingRangeFactor(void){
    if (!gUnlockWeb) return 0.0f;
    float best = 0.0f;
    for (const WebAnchor& a : gWebAnchors){
        if (a.wilt > 0) continue;                     // spent blooms offer nothing
        float dist = Vector3Length(pl.pos - a.pos);
        if (dist < WEB_RANGE && a.pos.y > pl.pos.y + 1.0f)
            best = fmaxf(best, 1.0f - dist/WEB_RANGE);
    }
    return best;
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
        if (!pl.grounded){                       // airborne wall contact: springable
            pl.wallT = WALL_GRACE;
            pl.wallN = (axis==0)? (Vector3){ (d>0)? -1.0f : 1.0f, 0, 0 }
                                : (Vector3){ 0, 0, (d>0)? -1.0f : 1.0f };
        }
    }
}
static void resolveCylsRadial(void){
    float feet = pl.pos.y - PLAYER_HH, head = pl.pos.y + PLAYER_HH;
    for (const Solid& s : solids){
        if (!s.isCyl) continue;
        float top = s.base.y + s.hgt;
        if (feet >= top - 0.01f || head <= s.base.y + 0.01f) continue;
        // falling onto the rim is a LANDING for moveVert, not a side hit -
        // shoving the player outward here caused the bounce edge-glitch.
        // Bouncy caps get a taller grace band: fast falls cross it in one step.
        if (pl.vel.y < -0.1f && feet > top - (s.bouncy? 0.70f : 0.40f)) continue;
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
        if (!pl.grounded){ pl.wallT = WALL_GRACE; pl.wallN = {ox, 0, oz}; }
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
            // bouncy caps catch the full body width (their lips overhang too) -
            // the old narrow band let edge-hits slip past and get shoved off
            float lim = s.rad + (s.bouncy? 0.34f : 0.15f);
            if (dx*dx + dz*dz > lim*lim) continue;
        } else {
            if (pl.pos.x + PLAYER_R < s.mn.x || pl.pos.x - PLAYER_R > s.mx.x) continue;
            if (pl.pos.z + PLAYER_R < s.mn.z || pl.pos.z - PLAYER_R > s.mx.z) continue;
        }
        float sTop = s.isCyl? s.base.y + s.hgt : s.mx.y;
        float sBot = s.isCyl? s.base.y         : s.mn.y;
        if (pl.vel.y <= 0.01f){
            float pen = sTop - feet;
            float penMax = s.bouncy? 0.60f : 0.34f;   // caps also catch diagonal rim entries
            if (pen > 0 && pen <= penMax && head > sTop){
                pl.pos.y = sTop + PLAYER_HH + 0.001f;
                feet = sTop + 0.001f; head = pl.pos.y + PLAYER_HH;
                if (s.bouncy && pl.vel.y < -2.0f){
                    float k = BOUNCE_K; int tier = 0;          // slam quality picks the spring
                    if (pl.slamming){
                        tier = (pl.slamT <= SLAM_WIN)? 2 : 1;
                        k = (tier == 2)? SLAM_K_PERF : SLAM_K_GOOD;
                        pl.slamming = false;
                    }
                    float out = clampf(-pl.vel.y*k, BOUNCE_MIN, BOUNCE_MAX);
                    pl.vel.y = out;
                    pl.vel.x *= 0.75f; pl.vel.z *= 0.75f;      // trampoline absorbs the slide
                    if (tier) FX_SlamHit(pl.pos, tier == 2, out);
                    else      FX_Bounce(pl.pos, out);
                } else {
                    gLandImpact = fmaxf(gLandImpact, -pl.vel.y);
                    pl.vel.y = 0;
                    gLandedSolidGround = true;
                    if (pl.slamming){                          // cannonball into solid rock
                        pl.slamming = false;
                        pl.stunT = SLAM_STUN;
                    }
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
static void SetPlantPoint(void){
    Vector3 f = yawFwd(pl.yaw);
    pl.plantPos = pl.pos + f*1.9f;
    float g = GroundTopBelow((Vector3){pl.plantPos.x, pl.pos.y, pl.plantPos.z});
    pl.plantPos.y = (g > -900)? g : pl.pos.y - PLAYER_HH;
    pl.plantYaw = pl.yaw;
    pl.plantT = (float)GetTime();
}
static void BeginWebSwing(void){
    float dist = Vector3Length(pl.pos - pl.webAnchor);
    pl.webLength = fmaxf(2.0f, dist*0.985f);   // slight cinch so the rope bites at once
    pl.webSwinging = true;
    pl.grounded = false; pl.coyote = 0;        // momentum is KEPT - that's the point
    SND(sWhoosh, 1.3f, 0.5f);
}
static void DoVault(float c, bool perfect){
    float power = c * (perfect? PERFECT_MULT : 1.0f);
    Vector3 f = yawFwd(pl.yaw);
    // the pole redirects your run into the launch direction
    float sprint = fmaxf(pl.plantSprint, fmaxf(0.0f, pl.vel.x*f.x + pl.vel.z*f.z));
    pl.vel.x = f.x*(VAULT_KEEP*sprint + VAULT_FWD0 + VAULT_FWD1*power);
    pl.vel.z = f.z*(VAULT_KEEP*sprint + VAULT_FWD0 + VAULT_FWD1*power);
    pl.vel.y = VAULT_VY0 + VAULT_VY1*power + MOMENTUM_K*sprint*power;
    if (gBoostT > 0) pl.vel.y += BOOST_VY;                    // spore turbo
    pl.fovKick = 6.0f + 5.0f*power + (perfect? 2.5f : 0.0f) + (gBoostT>0? 3.0f : 0.0f);
    pl.pitchKick = 0.09f + 0.07f*power;
    gHitstop = perfect? HITSTOP_PERFECT : HITSTOP_VAULT;      // impact frame
    pl.grounded = false; pl.coyote = 0;
    pl.plantT = (float)GetTime();
    pl.poleHideT = 0.55f;
    pl.vaults++; gTimerStarted = true;
    FX_Vault(pl.plantPos, power, perfect);
}
static void BeginVaultPlant(float c, bool perfect){
    SetPlantPoint();
    Vector3 f = yawFwd(pl.yaw);
    pl.plantSprint = fmaxf(0.0f, pl.vel.x*f.x + pl.vel.z*f.z);   // momentum at the plant
    pl.planting = true;
    pl.plantC = c;
    pl.plantPerf = perfect;
    pl.plantDelayMax = perfect ? 0.16f : 0.12f;   // a beat of glorious pole bend
    pl.plantDelay = pl.plantDelayMax;
    pl.poleHideT = 0;
    pl.crouch = fmaxf(pl.crouch, 0.24f + 0.12f*c);
    gHitstop = fmaxf(gHitstop, 0.03f);            // the tip bites
    gTimerStarted = true;
    FX_Plant(pl.plantPos, c, perfect);
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
    // ---- web swing: attach, hold, release --------------------------------
    if (pl.webSwinging){
        pl.webT += dt;
        if (!inputLocked && IsKeyPressed(KEY_F)) pl.webToggled = false;   // F lets go
        bool holding = (!inputLocked && (IsMouseButtonDown(MOUSE_BUTTON_RIGHT) || pl.webToggled)) || gBotWeb;
        bool snapped = pl.webT > WEB_MAX_T;              // dawdle and the silk SNAPS
        if (!holding || pl.grounded || snapped){
            pl.webSwinging = false; pl.webToggled = false;
            if (pl.webIdx >= 0 && pl.webIdx < (int)gWebAnchors.size())
                gWebAnchors[pl.webIdx].wilt = WEB_WILT;  // one swing per bloom - it's spent
            if (snapped){
                SND(sFoul, 1.4f, 0.6f);                  // no parting gift for hesitation
            } else {
                pl.vel = pl.vel*1.05f;                   // fly with what you earned
                pl.vel.y += 2.0f;
                SND(sWhiff, 1.4f, 0.4f);
            }
        } else {
            // pump: steer along the swing with WASD (gentle - momentum rules)
            Vector3 sf = yawFwd(pl.yaw), sr = { -sf.z, 0, sf.x };
            if (IsKeyDown(KEY_W) || gBotFwd){ pl.vel.x += sf.x*7.0f*dt; pl.vel.z += sf.z*7.0f*dt; }
            if (IsKeyDown(KEY_S)){ pl.vel.x -= sf.x*5.0f*dt; pl.vel.z -= sf.z*5.0f*dt; }
            if (IsKeyDown(KEY_D)){ pl.vel.x += sr.x*6.0f*dt; pl.vel.z += sr.z*6.0f*dt; }
            if (IsKeyDown(KEY_A)){ pl.vel.x -= sr.x*6.0f*dt; pl.vel.z -= sr.z*6.0f*dt; }
            float sp = Vector3Length(pl.vel);
            if (sp > 24.0f) pl.vel = pl.vel*(24.0f/sp);  // swings stay readable
        }
    } else if (!inputLocked || gBotWeb){
        bool grab = (!inputLocked && gUnlockWeb
                     && (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) || IsKeyPressed(KEY_F))) || gBotWeb;
        if (grab && !pl.charging && !pl.planting){
            float bestD = WEB_RANGE;
            for (int ai=0; ai<(int)gWebAnchors.size(); ai++){
                const WebAnchor& a = gWebAnchors[ai];
                if (a.wilt > 0) continue;                // wilted blooms won't hold you
                float d = Vector3Length(pl.pos - a.pos);
                if (d < bestD && a.pos.y > pl.pos.y + 1.0f){
                    bestD = d; pl.webAnchor = a.pos; pl.webIdx = ai;
                }
            }
            if (bestD < WEB_RANGE){
                pl.webToggled = !inputLocked && IsKeyPressed(KEY_F);
                pl.webT = 0;
                BeginWebSwing();
            }
        }
    }
    if (pl.planting){
        pl.plantDelay -= dt;
        float bendT = 1.0f - pl.plantDelay/pl.plantDelayMax;
        float bendE = sinf(fminf(bendT*1.25f, 1.0f)*PI);    // loads fast, recoils INTO launch
        pl.vel.x *= expf(-6.0f*dt);                      // dig in, but keep the run alive
        pl.vel.z *= expf(-6.0f*dt);
        pl.vel.y = 0;
        pl.grounded = true;
        pl.coyote = COYOTE;
        pl.poleAngle = 95.0f + 26.0f*bendE;
        pl.crouch = fmaxf(pl.crouch, 0.30f + 0.16f*pl.plantC + 0.06f*bendE);
        pl.pitchKick = -0.07f*bendE;                     // eyes dip into the bend
        if (pl.plantDelay <= 0){
            pl.planting = false;
            DoVault(pl.plantC, pl.plantPerf);
        }
        return;
    }
    // ---- slam & sail share SHIFT. Owning ONE keeps the legacy feel: SHIFT
    // press = instant slam (Gorge), SHIFT hold = sail (Skyhaven). Owning BOTH:
    // the press opens the sail at once, and releasing within SLAM_TAP converts
    // it into the slam - tap to dive, hold to glide. E is always an instant
    // slam. A slam always snaps the canopy shut; the two states never overlap
    // (they used to, and slam gravity ate the sail - "the updraft is broken").
    pl.stunT = fmaxf(0.0f, pl.stunT - dt);
    bool shiftWasDown = gBotShiftPrev; gBotShiftPrev = gBotShift;
    bool shiftDown     = (!inputLocked && IsKeyDown(KEY_LEFT_SHIFT))      || gBotShift;
    bool shiftReleased = (!inputLocked && IsKeyReleased(KEY_LEFT_SHIFT))  || (shiftWasDown && !gBotShift);
    bool shiftPressed  = (!inputLocked && IsKeyPressed(KEY_LEFT_SHIFT))   || (gBotShift && !shiftWasDown);
    bool slamNow = gBotSlam
        || (!inputLocked && gUnlockSlam && IsKeyPressed(KEY_E))
        || (gUnlockSlam && !gUnlockSail && shiftPressed);
    if (gUnlockSlam && gUnlockSail && pl.sailing && pl.sailT < SLAM_TAP && shiftReleased)
        slamNow = true;                                  // the tap: cut the sail, tuck
    if (slamNow && !pl.grounded && !pl.webSwinging && !pl.slamming && !pl.planting){
        pl.sailing = false; pl.sailT = 0;                // canopy snaps shut into the dive
        pl.slamming = true; pl.slamT = 0;
        pl.charging = false; pl.pendT = 0;               // pole away - you're a cannonball
        pl.vel.x *= 0.60f; pl.vel.z *= 0.60f;            // commitment: the line is chosen
        FX_Slam();
    }
    if (pl.slamming){
        pl.slamT += dt;
        if (pl.grounded) pl.slamming = false;
    }
    // ---- skysail: hold SHIFT airborne to hang-glide
    Vector3 sf = yawFwd(pl.yaw), sr = { -sf.z, 0, sf.x };
    bool sailHeld = (gUnlockSail && shiftDown) || gBotSail;
    if (sailHeld && !pl.grounded && !pl.webSwinging && !pl.planting && !pl.slamming && pl.stunT <= 0){
        if (!pl.sailing){ pl.sailing = true; pl.sailT = 0; pl.charging = false; pl.pendT = 0; SND(sWhoosh, 0.7f, 0.45f); }
        pl.sailT += dt;
    } else pl.sailing = false;
    if (pl.sailing){
        float tiltTgt = 0.0f;
        if (IsKeyDown(KEY_W) || gBotFwd) tiltTgt =  1.0f;    // dive (tuck)
        if (IsKeyDown(KEY_S))            tiltTgt = -1.0f;    // flare (brake/rise a touch)
        pl.sailTilt += (tiltTgt - pl.sailTilt)*fminf(1.0f, dt*6.0f);
        if (pl.sailTilt > 0){                                // dive builds forward speed
            pl.vel.x += sf.x*SAIL_DIVE*pl.sailTilt*dt;
            pl.vel.z += sf.z*SAIL_DIVE*pl.sailTilt*dt;
        } else if (pl.sailTilt < 0){                          // flare bleeds speed
            pl.vel.x *= (1.0f - 0.9f*(-pl.sailTilt)*dt);
            pl.vel.z *= (1.0f - 0.9f*(-pl.sailTilt)*dt);
        }
        if (IsKeyDown(KEY_D)){ pl.vel.x += sr.x*SAIL_STEER*dt; pl.vel.z += sr.z*SAIL_STEER*dt; }
        if (IsKeyDown(KEY_A)){ pl.vel.x -= sr.x*SAIL_STEER*dt; pl.vel.z -= sr.z*SAIL_STEER*dt; }
        float hs = sqrtf(pl.vel.x*pl.vel.x + pl.vel.z*pl.vel.z);
        float hcap = 15.0f + 11.0f*fmaxf(0.0f, pl.sailTilt);
        if (hs > hcap){ pl.vel.x *= hcap/hs; pl.vel.z *= hcap/hs; }
    }
    pl.sailOpen += ((pl.sailing?1.0f:0.0f) - pl.sailOpen)*fminf(1.0f, dt*9.0f);
    // ---- run
    Vector3 f = yawFwd(pl.yaw), r = { -f.z, 0, f.x };   // r = screen-right = cross(f, up)
    Vector3 wish = {0,0,0};
    if (!inputLocked && pl.stunT <= 0){
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
    } else if (wl > 0.01f && !pl.webSwinging){           // swing pump handles its own steering
        float aa = AIR_ACC*(pl.slamming? 0.5f : 1.0f);   // a dive barely steers
        pl.vel.x += wish.x*aa*dt;
        pl.vel.z += wish.z*aa*dt;
    }
    // ---- WALLSPRING: tap the vault key against a wall mid-air and kick off it.
    // Falling speed converts into spring height - late, falling springs go BIG.
    pl.wallT   = fmaxf(0.0f, pl.wallT - dt);
    pl.kickBuf = fmaxf(0.0f, pl.kickBuf - dt);
    if (gUnlockWall && !pl.grounded && !pl.webSwinging && !pl.sailing
        && !pl.slamming && pl.stunT <= 0){
        bool kp = (!inputLocked && (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)
                                    || IsKeyPressed(KEY_SPACE))) || gBotKick;
        if (kp) pl.kickBuf = 0.12f;
        if (pl.kickBuf > 0 && pl.wallT > 0){
            float fall = fmaxf(0.0f, -pl.vel.y);
            Vector3 n = pl.wallN;
            float vn = pl.vel.x*n.x + pl.vel.z*n.z;          // strip the into-wall part,
            pl.vel.x = (pl.vel.x - n.x*vn)*0.55f + n.x*WALL_OUT;   // keep some slide
            pl.vel.z = (pl.vel.z - n.z*vn)*0.55f + n.z*WALL_OUT;
            pl.vel.y = WALL_VY + WALL_CONV*fall;
            pl.wallT = 0; pl.kickBuf = 0;
            pl.charging = false; pl.pendT = 0;
            pl.vaultCD = 0.25f;                              // that press was not a charge
            pl.poleHideT = 0.40f;                            // arms fling off the wall
            pl.fovKick += 3.5f + 0.12f*fall;
            pl.rollT = 0.55f;                                // a lick of roll, not a full tumble
            gHitstop = fmaxf(gHitstop, 0.03f);
            FX_WallSpring(pl.pos - n*0.5f, fall);
        }
    }
    // ---- vault charge
    bool hold = !inputLocked && pl.stunT <= 0 && !pl.sailing
             && (IsMouseButtonDown(MOUSE_BUTTON_LEFT) || IsKeyDown(KEY_SPACE) || gBotHold);
    pl.vaultCD = fmaxf(0.0f, pl.vaultCD - dt);
    if (hold && !pl.charging && pl.vaultCD <= 0 && !pl.webSwinging){
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
                if (pl.grounded || pl.coyote > 0) BeginVaultPlant(cc, perfect);
                else if (pl.chargeT >= 0.25f){               // buffer: plant on touchdown
                    pl.pendC = cc; pl.pendPerf = perfect; pl.pendT = BUFFER_T;
                }
            }
        }
    }
    if (!pl.charging) pl.poleAngle = fmaxf(0.0f, pl.poleAngle - dt*420.0f);
    // ---- updraft columns lift you (a lot under sail); ambient wind pushes you.
    // Lift pulls vel.y toward a target RISE RATE (str scaled) instead of adding
    // raw acceleration - a long dwell can never bank unbounded exit speed, so a
    // column's ceiling really is its ceiling (+ a small ballistic hop).
    pl.inUpdraft = false;
    for (auto& u : gUpdrafts){
        float dx = pl.pos.x - u.base.x, dz = pl.pos.z - u.base.z;
        if (dx*dx + dz*dz < u.rad*u.rad && pl.pos.y > u.base.y && pl.pos.y < u.base.y + u.hgt){
            pl.inUpdraft = true;
            float tgt = u.str * (pl.sailing? 0.45f : 0.22f);
            if (pl.vel.y < tgt) pl.vel.y += (tgt - pl.vel.y) * fminf(1.0f, 5.5f*dt);
            pl.vel.x -= dx*0.9f*dt; pl.vel.z -= dz*0.9f*dt;   // gentle inward pull
        }
    }
    // ---- SKYHAVEN hazard mills: a sweeping blade rips the sail off and bats you
    for (auto& w : gWindmills){
        if (!w.hazard || pl.stunT > 0) continue;
        float dx = pl.pos.x - w.pos.x;
        if (fabsf(dx) > 1.7f) continue;                    // only near the spinning plane
        float dy = pl.pos.y - w.pos.y, dz = pl.pos.z - w.pos.z;
        float rr = sqrtf(dy*dy + dz*dz);
        if (rr < 1.0f || rr > w.rad) continue;             // hub gap or outside the blades
        float pang = atan2f(-dy, dz);                      // your angle on the blade disc
        float base = (float)GetTime()*w.spd*DEG2RAD + w.tilt;
        bool hit = false;
        for (int b=0; b<w.blades && !hit; b++){
            float d = pang - (base + b*(6.2832f/w.blades));
            while (d >  PI) d -= 2.0f*PI;
            while (d < -PI) d += 2.0f*PI;
            if (fabsf(d) < 0.22f) hit = true;              // a blade is passing through you
        }
        if (hit){
            float nx = dy/rr, nz = dz/rr;                  // fling radially out of the disc
            pl.vel.y = nx*13.0f - 3.0f;
            pl.vel.z = pl.vel.z*0.3f + nz*13.0f;
            pl.vel.x = pl.vel.x*0.3f - 9.0f;               // spun back against the climb
            pl.sailing = false; pl.sailOpen = 0;           // the canopy collapses
            pl.stunT = 0.45f;                              // can't re-open the sail at once
            gShake += 0.8f; pl.rollT = 0;
            SND(sFoul, 0.8f, 0.65f);
        }
    }
    if (!pl.grounded){
        float wk = pl.sailing? SAIL_WINDK : 0.22f;
        pl.vel.x += gAirWind.x*wk*dt; pl.vel.z += gAirWind.z*wk*dt;
    }
    // ---- gravity + integrate with substeps (fast falls can't tunnel)
    float grv = pl.slamming? SLAM_GRAV : pl.sailing? SAIL_GRAV : (pl.vel.y > 0 ? GRAV_UP : GRAV_DOWN);
    pl.vel.y -= grv*dt;
    float term = pl.slamming? -SLAM_TERM : pl.sailing? -(SAIL_TERM + 13.0f*fmaxf(0.0f,pl.sailTilt)) : -TERMINAL;
    pl.vel.y = fmaxf(pl.vel.y, term);
    gLandImpact = 0; gLandedSolidGround = false;
    float maxd = fmaxf(fabsf(pl.vel.x), fmaxf(fabsf(pl.vel.y), fabsf(pl.vel.z)))*dt;
    int steps = (int)ceilf(maxd/0.28f); if (steps < 1) steps = 1; if (steps > 48) steps = 48;
    float sdt = dt/steps;
    for (int i=0;i<steps;i++){
        // integrate from CURRENT velocity each substep - a mid-frame bounce
        // used to keep applying the stale downward displacement, burying the
        // player in the cap so the radial pass ejected them out the side
        moveHoriz(0, pl.vel.x*sdt);
        moveHoriz(2, pl.vel.z*sdt);
        resolveCylsRadial();
        moveVert(pl.vel.y*sdt);
    }
    // ---- web rope constraint: stay on the sphere, keep tangential speed
    if (pl.webSwinging){
        Vector3 to = pl.pos - pl.webAnchor;
        float d = Vector3Length(to);
        if (d > pl.webLength && d > 0.001f){
            Vector3 n = to*(1.0f/d);
            pl.pos = pl.webAnchor + n*pl.webLength;
            float vr = pl.vel.x*n.x + pl.vel.y*n.y + pl.vel.z*n.z;
            if (vr > 0){ pl.vel.x -= n.x*vr; pl.vel.y -= n.y*vr; pl.vel.z -= n.z*vr; }
        }
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
            BeginVaultPlant(pl.pendC, pl.pendPerf);
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
    if (pl.webSwinging){                       // bank into the swing
        Vector3 sr = { -cosf(pl.yaw), 0, sinf(pl.yaw) };      // screen-right
        pl.roll += clampf((pl.vel.x*sr.x + pl.vel.z*sr.z)*0.7f, -14.0f, 14.0f);
    }
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
    if (pl.pos.y < -25){ pl.pos = gSpawn; pl.vel = {0,0,0}; }
}

// ============================================================================
// FlyUpdate - DEBUG / TEST free-fly camera. **NOT PART OF THE GAME.** Toggled
// with F3. It noclips the player anywhere on the map (through walls, no
// gravity, no collision) so a developer can inspect and lay out levels. All
// real physics and gameplay are suspended while flying; a loud on-screen
// banner tells the player this is a testing tool, not a mechanic.
//   WASD = fly along aim · Space/Ctrl = up/down · Shift = boost · F3 = exit
// ============================================================================
static void FlyUpdate(float dt){
    if (gSkipLook > 0){ GetMouseDelta(); gSkipLook--; }
    else {
        Vector2 md = GetMouseDelta();
        if (fabsf(md.x) > 350 || fabsf(md.y) > 350) md = (Vector2){0,0};
        pl.yaw   -= md.x*0.0021f;
        pl.pitch -= md.y*0.0021f;
        pl.pitch  = clampf(pl.pitch, -1.55f, 1.55f);
    }
    Vector3 fwd = lookDir(pl.yaw, pl.pitch);            // full 3D aim (includes pitch)
    Vector3 flat = yawFwd(pl.yaw);
    Vector3 right = { -flat.z, 0, flat.x };
    Vector3 mv = {0,0,0};
    if (IsKeyDown(KEY_W)) mv = mv + fwd;
    if (IsKeyDown(KEY_S)) mv = mv - fwd;
    if (IsKeyDown(KEY_D)) mv = mv + right;
    if (IsKeyDown(KEY_A)) mv = mv - right;
    if (IsKeyDown(KEY_SPACE)) mv.y += 1.0f;
    if (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_E)) mv.y -= 1.0f;
    float l = Vector3Length(mv);
    if (l > 0.001f){
        mv = mv*(1.0f/l);
        float sp = IsKeyDown(KEY_LEFT_SHIFT)? 95.0f : 32.0f;   // Shift = boost
        pl.pos = pl.pos + mv*(sp*dt);
    }
    // suspend everything so exiting fly mode is clean (drop from where you are)
    pl.vel = {0,0,0}; pl.grounded = false; pl.coyote = 0;
    pl.charging = false; pl.planting = false; pl.slamming = false; pl.webSwinging = false;
    pl.eyeDip = 0; pl.eyeDipVel = 0; pl.crouch = 0; pl.roll = 0; pl.fovKick = 0;
    pl.pitchKick = 0;
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
static Vector3 bez3(Vector3 a, Vector3 b, Vector3 c, float t){
    float u = 1.0f - t;
    return a*(u*u) + b*(2.0f*u*t) + c*(t*t);
}
static void DrawBentPole(Vector3 tip, Vector3 grip, Vector3 fwd, float load, float alpha){
    Vector3 mid = (tip + grip)*0.5f;
    Vector3 ctrl = mid + fwd*(1.3f + 2.2f*load) + (Vector3){0,-0.55f-0.95f*load,0};
    const int segs = 16;
    for (int i=0;i<segs;i++){
        float a = (float)i/segs, b = (float)(i+1)/segs;
        Color col = (i%2)? (Color){240,240,236,255} : (Color){228,44,34,255};
        col.a = (unsigned char)(255*alpha);
        drawCylBetween(bez3(tip, ctrl, grip, a), bez3(tip, ctrl, grip, b), 0.040f, col);
    }
    Vector3 dir = Vector3Normalize(bez3(tip, ctrl, grip, 0.04f) - tip);
    Vector3 axis = Vector3CrossProduct((Vector3){0,1,0}, dir);
    float ang = RAD2DEG*acosf(clampf(dir.y,-1,1));
    if (Vector3Length(axis)<0.001f) axis = {1,0,0};
    drawM(gCone, tip + dir*0.03f, {0.085f,0.28f,0.085f}, {42,42,52,(unsigned char)(255*alpha)}, TX_WHITE, axis, ang);
}
// ------------------------------------------------------- first-person body -
static const Color C_SLEEVE = {228, 44, 34,255};
static const Color C_PANTS  = { 56, 92,214,255};
static const Color C_SHOE   = {138, 66, 30,255};

// one leg, angles measured from straight-down toward facing (deg)
static void DrawLegFP(Vector3 hip, Vector3 fwd, float aThigh, float aShin, float alpha){
    Vector3 dn = {0,-1,0};
    float s1=sinf(aThigh*DEG2RAD), c1=cosf(aThigh*DEG2RAD);
    float s2=sinf(aShin*DEG2RAD),  c2=cosf(aShin*DEG2RAD);
    Vector3 knee  = hip  + fwd*(0.33f*s1) + dn*(0.33f*c1);
    Vector3 ankle = knee + fwd*(0.35f*s2) + dn*(0.35f*c2);
    Color pants = C_PANTS; pants.a = (unsigned char)(255*alpha);
    Color shoe  = C_SHOE;  shoe.a  = (unsigned char)(255*alpha);
    drawCylBetween(hip, knee, 0.054f, pants);
    drawM(gSphere, knee, {0.050f,0.050f,0.050f}, pants, TX_WHITE);
    drawCylBetween(knee, ankle, 0.041f, pants);
    // cartoon shoe: heel blob + toe blob pointing "leg-forward"
    Vector3 toeDir = fwd*c2 - dn*s2;
    drawM(gSphere, ankle, {0.062f,0.052f,0.062f}, shoe, TX_WHITE);
    drawM(gSphere, ankle + toeDir*0.08f, {0.050f,0.041f,0.050f}, shoe, TX_WHITE);
}
static void DrawBodyFP(const Camera3D& cam, float t){
    // ghost the body away as you look down - never block the landing read
    float bodyA = 1.0f - 0.90f*clampf((-0.32f - pl.pitch)/0.55f, 0.0f, 1.0f);
    if (bodyA <= 0.06f) return;
    Vector3 f = yawFwd(pl.yaw);
    Vector3 r = { -f.z, 0, f.x };
    // overalls torso, slim and set back (visible looking down; sinks with the crouch)
    Vector3 hipC  = cam.position + (Vector3){0,-0.66f,0} - f*0.24f;
    Vector3 chest = cam.position + (Vector3){0,-0.22f,0} - f*0.18f;
    Color pants = C_PANTS; pants.a = (unsigned char)(255*bodyA);
    Color gold  = C_GOLD;  gold.a  = (unsigned char)(255*bodyA);
    drawCylBetween(hipC, chest, 0.10f, pants);
    drawM(gSphere, chest + f*0.08f + r*0.06f, {0.022f,0.022f,0.022f}, gold, TX_WHITE);
    drawM(gSphere, chest + f*0.08f - r*0.06f, {0.022f,0.022f,0.022f}, gold, TX_WHITE);

    float hsp = sqrtf(pl.vel.x*pl.vel.x + pl.vel.z*pl.vel.z);
    float vaultT = t - pl.plantT;
    float aL, aR, sL, sR;
    if (pl.slamming){                                      // cannonball: heels to the sky
        float w = sinf(t*26.0f)*3.0f;
        aL = 66 + w;  sL = aL - 96;
        aR = 58 - w;  sR = aR - 92;
    } else if (!pl.grounded && !pl.webSwinging && vaultT > 0 && vaultT < 0.95f){
        // THE OLYMPIC KICK: legs whip up in front, hang, then unfold - framing
        // the view from below, never covering the landing
        float up   = sinf(fminf(vaultT/0.5f, 1.0f)*PI*0.5f);
        float down = clampf((vaultT-0.55f)/0.4f, 0.0f, 1.0f);
        float k  = up*(1.0f - down*down);
        float k2 = fmaxf(0.0f, k - 0.15f);                 // second leg lags the whip
        aL = 15 + 84*k;    sL = aL + 20*k;                 // shin EXTENDS: toes forward-up
        aR = 12 + 78*k2;   sR = aR + 16*k2;
    } else if (pl.sailing){                                // gliding: legs stretched back, relaxed sway
        float sw = sinf(t*2.6f)*5.0f;
        float back = -30.0f - 22.0f*fmaxf(0.0f, pl.sailTilt);
        aL = back + sw;      sL = aL - 16;
        aR = back - 6 - sw;  sR = aR - 20;
    } else if (pl.webSwinging){
        float trail = -clampf(hsp*1.8f, 0.0f, 38.0f);      // legs stream behind the swing
        aL = trail;      sL = aL - 24;
        aR = trail - 6;  sR = aR - 30;
    } else if (!pl.grounded){
        aL = -8  + sinf(t*2.2f)*4;       sL = aL - 22;     // dangling on the way down
        aR = -14 + sinf(t*2.2f+1.2f)*4;  sR = aR - 28;
    } else if (hsp > 1.8f){
        float sc = sinf(pl.bobPhase);                      // running scissor
        aL =  sc*30;  sL = aL - (sc>0? 10.0f : 36.0f);
        aR = -sc*30;  sR = aR - (sc<0? 10.0f : 36.0f);
    } else {
        aL = 4;  sL = -6;  aR = -3; sR = -12;              // standing easy
    }
    DrawLegFP(hipC + r*0.17f, f, aR, sR, bodyA);
    DrawLegFP(hipC - r*0.17f, f, aL, sL, bodyA);
}

static void DrawPoleFP(const Camera3D& cam, float t){
    DrawBodyFP(cam, t);
    if (pl.sailOpen > 0.03f){
        // the SKYSAIL: a wide striped cloth canopy billowing overhead, gripped
        // by both arms. Dive tips it forward/down, flare lifts it.
        float op = pl.sailOpen;
        Vector3 fwd = Vector3Normalize(cam.target - cam.position);
        Vector3 right = Vector3Normalize(Vector3CrossProduct(fwd, (Vector3){0,1,0}));
        Vector3 center = cam.position + fwd*(0.55f + 0.45f*op)
                       + (Vector3){0, 0.95f*op - 0.35f*pl.sailTilt, 0};
        int NP = 7;
        for (int i=0;i<NP;i++){
            float u = (i/(float)(NP-1) - 0.5f);              // -0.5..0.5 across the sail
            float billow = sinf(t*4.5f + i*0.9f)*0.07f*op;
            Vector3 p = center + right*(u*2.3f*op) + fwd*(-0.55f*u*u*op)
                      + (Vector3){0, billow - 0.25f*u*u*op, 0};
            Color cc = (i%2)? (Color){238,74,84,255} : (Color){246,246,250,255};   // red/white
            drawM(gCube, p, {0.36f*op, 0.055f, 0.52f*op}, cc, TX_CLOTH, right, -18.0f*pl.sailTilt);
        }
        Vector3 barL = center + right*(-1.05f*op), barR = center + right*(1.05f*op);
        drawCylBetween(barL, barR, 0.028f, C_WOOD);           // spar
        drawCylBetween(cam.position + right*0.34f + (Vector3){0,-0.5f,0}, barR, 0.045f, C_SLEEVE);
        drawCylBetween(cam.position - right*0.34f + (Vector3){0,-0.5f,0}, barL, 0.045f, C_SLEEVE);
        drawM(gSphere, barR, {0.06f,0.06f,0.06f}, WHITE, TX_WHITE);
        drawM(gSphere, barL, {0.06f,0.06f,0.06f}, WHITE, TX_WHITE);
        return;
    }
    if (pl.webSwinging){
        // sticky red mushroom SAP stretched from your hand to the pod: a thick
        // goopy rope, bulging where it clings and pulled thin in the middle,
        // two-tone and glossy with sagging drips. It reddens/thins as it strains.
        Vector3 fwd = Vector3Normalize(cam.target - cam.position);
        Vector3 right = Vector3Normalize(Vector3CrossProduct(fwd, (Vector3){0,1,0}));
        Vector3 hand = cam.position + fwd*0.55f + right*0.35f + (Vector3){0,-0.30f,0};
        Vector3 mid = (hand + pl.webAnchor)*0.5f; mid.y -= 0.55f;    // heavy sag
        float strain = clampf(pl.webT/WEB_MAX_T, 0, 1);
        Color sap  = { 214, (unsigned char)(44 + 26*strain), (unsigned char)(52 - 12*strain), 255 };
        Color sapD = { 150, 22, 30, 255 };                          // dark wet red
        int N = 20; Vector3 prev = hand;
        for (int i=1;i<=N;i++){
            float tt = (float)i/N;
            Vector3 p = bez3(hand, mid, pl.webAnchor, tt);
            float ends = fabsf(2.0f*tt - 1.0f);                     // 1 at the sticky ends, 0 mid
            float rad = (0.022f + 0.058f*ends*ends) * (1.0f - 0.4f*strain);
            rad *= 1.0f + 0.20f*sinf(tt*14.0f + t*4.0f);            // organic goopy bulge
            drawCylBetween(prev, p, rad, (i%2)? sap : sapD);        // two-tone, wet & ridged
            prev = p;
        }
        // droplets sagging off the underside (about to drip)
        for (int i=1;i<5;i++){
            float tt = i/5.0f;
            Vector3 p = bez3(hand, mid, pl.webAnchor, tt);
            float dr = 0.055f + 0.03f*sinf(t*2.0f + i);
            drawM(gSphere, p + (Vector3){0,-0.05f-dr,0}, {dr,dr*1.5f,dr}, sap, TX_STREAK);
            drawM(gSphere, p + (Vector3){-0.02f,-0.03f,0}, {0.018f,0.018f,0.018f}, (Color){255,170,170,200}, TX_WHITE);
        }
        // the sticky glob where the sap stretches out of your gloved fist
        drawM(gSphere, hand, {0.085f,0.085f,0.085f}, sap, TX_STREAK);
        drawM(gSphere, hand + (Vector3){-0.03f,0.04f,0}, {0.03f,0.03f,0.03f}, (Color){255,180,180,220}, TX_WHITE);
        Vector3 sh = cam.position + right*0.34f + (Vector3){0,-0.52f,0};
        drawCylBetween(sh, hand, 0.05f, C_SLEEVE);                    // arm to the fist
        return;
    }
    // ghost of the planted pole you just launched off - springs upright, wobbling
    float ga = 1.0f - (t - pl.plantT)/0.8f;
    if (ga > 0 && ga < 1){
        Vector3 f = yawFwd(pl.plantYaw);
        float wob = sinf((t - pl.plantT)*26.0f)*0.30f*ga;   // recoil after the whip
        Vector3 lean = Vector3Normalize((Vector3){ -f.x*(0.35f - wob), 1.0f, -f.z*(0.35f - wob) });
        DrawPoleStripes(pl.plantPos, lean, 4.0f, 0.045f, ga*0.9f);
    }
    Vector3 fwd = Vector3Normalize(cam.target - cam.position);
    Vector3 right = Vector3Normalize(Vector3CrossProduct(fwd, (Vector3){0,1,0}));   // screen-right
    if (pl.poleHideT > 0){
        // just launched: both arms thrown up past the ears, vaulter clearing the bar
        float k = 1.0f - pl.poleHideT/0.55f;
        float lift = sinf(fminf(k*1.35f, 1.0f)*PI);
        Vector3 shR = cam.position + right*0.38f + (Vector3){0,-0.55f,0};
        Vector3 shL = cam.position - right*0.38f + (Vector3){0,-0.55f,0};
        Vector3 hR  = cam.position + right*0.55f + fwd*0.50f + (Vector3){0,-0.40f + 0.88f*lift,0};
        Vector3 hL  = cam.position - right*0.55f + fwd*0.50f + (Vector3){0,-0.44f + 0.82f*lift,0};
        drawCylBetween(shR, hR, 0.050f, C_SLEEVE);
        drawCylBetween(shL, hL, 0.050f, C_SLEEVE);
        drawM(gSphere, hR, {0.072f,0.072f,0.072f}, WHITE, TX_WHITE);
        drawM(gSphere, hL, {0.068f,0.068f,0.068f}, WHITE, TX_WHITE);
        return;
    }
    Vector3 grip = cam.position + fwd*0.68f + right*0.44f + (Vector3){0,-0.46f,0};
    if (pl.planting){
        float k = clampf(1.0f - pl.plantDelay/pl.plantDelayMax, 0, 1);
        float load = sinf(fminf(k*1.25f,1.0f)*PI) * (1.15f + 1.15f*pl.plantC + (pl.plantPerf?0.40f:0.0f));
        Vector3 root = pl.plantPos + (Vector3){0,0.03f,0};
        Vector3 lean = Vector3Normalize((Vector3){-yawFwd(pl.plantYaw).x*0.35f, 1.0f, -yawFwd(pl.plantYaw).z*0.35f});
        DrawPoleStripes(root, lean, 1.0f, 0.055f, 0.95f);
        DrawBentPole(root, grip + (Vector3){0,-0.04f,0}, yawFwd(pl.plantYaw), load, 1.0f);
        Vector3 hA = grip + (Vector3){0.06f,-0.02f,0}, hB = grip + (Vector3){-0.10f,-0.05f,0};
        drawCylBetween(cam.position + right*0.40f + (Vector3){0,-0.55f,0}, hA, 0.05f, C_SLEEVE);
        drawCylBetween(cam.position + right*0.02f + (Vector3){0,-0.58f,0}, hB, 0.05f, C_SLEEVE);
        drawM(gSphere, hA, {0.075f,0.075f,0.075f}, WHITE, TX_WHITE);
        drawM(gSphere, hB, {0.068f,0.068f,0.068f}, WHITE, TX_WHITE);
        return;
    }
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
    // arms + white cartoon gloves on the pole
    Vector3 hA = grip + dir*0.09f, hB = grip - dir*0.13f;
    drawCylBetween(cam.position + right*0.40f + (Vector3){0,-0.55f,0}, hA, 0.048f, C_SLEEVE);
    drawCylBetween(cam.position + right*0.04f + (Vector3){0,-0.58f,0}, hB, 0.048f, C_SLEEVE);
    drawM(gSphere, hA, {0.062f,0.062f,0.062f}, WHITE, TX_WHITE);
    drawM(gSphere, hB, {0.058f,0.058f,0.058f}, WHITE, TX_WHITE);
}
