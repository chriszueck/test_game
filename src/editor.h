// ============================================================================
// editor.h : the LEVEL EDITOR (F4) - fly anywhere, click things, move/scale/
// delete/duplicate them, drop new prefabs, and F5-save the level to
// levels/levelN.txt. From then on the game loads YOUR file for that level
// (delete the file to get the code-built version back).
//
// Everything a composite builder created (a whole mushroom, a pipe, a sky
// platform + its coin) shares a group id and is edited as one object.
// ============================================================================
#pragma once
#include "game.h"
#include "world.h"
#include "gfx.h"
#include "player.h"
#include "level_io.h"
#include <algorithm>

static void drawTextC(const char* s, int cx, int y, int size, Color c);   // main.cpp
static void drawTextSh(const char* s, int x, int y, int size, Color c);   // main.cpp

static bool gEditMode  = false;
static bool gEditDirty = false;
enum ESelKind { ESEL_NONE, ESEL_GRP, ESEL_SPAWN, ESEL_STAR };
static int  gESelKind = ESEL_NONE;
static int  gESelGrp  = -1;
static Vector3 gEditAim = {0,0,0};      // where the crosshair ray lands (spawn point)

// ------------------------------------------------------- bounding volumes --
static BoundingBox SolidBB(const Solid& s){ return { s.mn, s.mx }; }
static BoundingBox DecorBB(const Decor& d){
    Vector3 p = d.pos, s = d.scale;
    switch (d.kind){
        case D_CUBE:
            return {{p.x-s.x*0.5f, p.y-s.y*0.5f, p.z-s.z*0.5f},
                    {p.x+s.x*0.5f, p.y+s.y*0.5f, p.z+s.z*0.5f}};
        case D_CYL: case D_CONE: {                    // mesh spans y 0..1, base at pos
            float r = fmaxf(s.x, s.z);
            return {{p.x-r, p.y, p.z-r},{p.x+r, p.y+s.y, p.z+r}};
        }
        default:                                       // sphere: scale = radii
            return {{p.x-s.x, p.y-s.y, p.z-s.z},{p.x+s.x, p.y+s.y, p.z+s.z}};
    }
}

// ---------------------------------------------------------------- picking --
static void EditorPick(Ray ray){
    float best = 1e9f;
    int bk = ESEL_NONE, bg = -1;
    auto tryHit = [&](RayCollision rc, int kind, int grp){
        if (rc.hit && rc.distance > 0.3f && rc.distance < best){
            best = rc.distance; bk = kind; bg = grp;
        }
    };
    for (const Solid& s : solids)
        tryHit(GetRayCollisionBox(ray, SolidBB(s)), ESEL_GRP, s.grp);
    for (const Decor& d : decor)
        tryHit(GetRayCollisionBox(ray, DecorBB(d)), ESEL_GRP, d.grp);
    for (const Coin& c : gCoins)      tryHit(GetRayCollisionSphere(ray, c.p, 0.8f),   ESEL_GRP, c.grp);
    for (const WebAnchor& a : gWebAnchors) tryHit(GetRayCollisionSphere(ray, a.pos, 1.0f), ESEL_GRP, a.grp);
    for (const Spore& s : gSpores)    tryHit(GetRayCollisionSphere(ray, s.p, 0.8f),   ESEL_GRP, s.grp);
    for (const Shrine& s : gShrines)  tryHit(GetRayCollisionSphere(ray, s.p, 1.4f),   ESEL_GRP, s.grp);
    for (const Updraft& u : gUpdrafts){
        BoundingBox bb = {{u.base.x-u.rad, u.base.y, u.base.z-u.rad},
                          {u.base.x+u.rad, u.base.y+u.hgt, u.base.z+u.rad}};
        tryHit(GetRayCollisionBox(ray, bb), ESEL_GRP, u.grp);
    }
    for (const Windmill& w : gWindmills)
        tryHit(GetRayCollisionSphere(ray, w.pos, fmaxf(1.4f, w.rad*0.2f)), ESEL_GRP, w.grp);
    for (const Banner& b : gBanners)  tryHit(GetRayCollisionSphere(ray, b.top, 0.9f), ESEL_GRP, b.grp);
    for (const Pinwheel& p : gPinwheels) tryHit(GetRayCollisionSphere(ray, p.pos, 0.9f), ESEL_GRP, p.grp);
    for (const Mote& m : gMotes)      tryHit(GetRayCollisionSphere(ray, m.p, 1.0f),   ESEL_GRP, m.grp);
    tryHit(GetRayCollisionSphere(ray, gSpawn, 1.2f), ESEL_SPAWN, -1);
    tryHit(GetRayCollisionSphere(ray, gStarP, 1.4f), ESEL_STAR,  -1);
    gESelKind = bk; gESelGrp = bg;
}

// -------------------------------------------------------------- group ops --
static int GrpCount(int g){
    int n = 0;
    for (auto& e : solids)      if (e.grp==g) n++;
    for (auto& e : decor)       if (e.grp==g) n++;
    for (auto& e : gCoins)      if (e.grp==g) n++;
    for (auto& e : gWebAnchors) if (e.grp==g) n++;
    for (auto& e : gSpores)     if (e.grp==g) n++;
    for (auto& e : gShrines)    if (e.grp==g) n++;
    for (auto& e : gUpdrafts)   if (e.grp==g) n++;
    for (auto& e : gWindmills)  if (e.grp==g) n++;
    for (auto& e : gBanners)    if (e.grp==g) n++;
    for (auto& e : gPinwheels)  if (e.grp==g) n++;
    for (auto& e : gMotes)      if (e.grp==g) n++;
    return n;
}
static void GrpMove(int g, Vector3 d){
    for (auto& s : solids) if (s.grp==g){ s.mn=s.mn+d; s.mx=s.mx+d; s.base=s.base+d; }
    for (auto& e : decor)       if (e.grp==g) e.pos  = e.pos  + d;
    for (auto& e : gCoins)      if (e.grp==g) e.p    = e.p    + d;
    for (auto& e : gWebAnchors) if (e.grp==g) e.pos  = e.pos  + d;
    for (auto& e : gSpores)     if (e.grp==g) e.p    = e.p    + d;
    for (auto& e : gShrines)    if (e.grp==g) e.p    = e.p    + d;
    for (auto& e : gUpdrafts)   if (e.grp==g) e.base = e.base + d;
    for (auto& e : gWindmills)  if (e.grp==g) e.pos  = e.pos  + d;
    for (auto& e : gBanners)    if (e.grp==g) e.top  = e.top  + d;
    for (auto& e : gPinwheels)  if (e.grp==g) e.pos  = e.pos  + d;
    for (auto& e : gMotes)      if (e.grp==g) e.p    = e.p    + d;
    if (g == gWarpGrp) gWarpTop = gWarpTop + d;    // the warp trigger travels with its pipe
}
// scale about the group's footprint center at its lowest point (a mushroom
// scales up from its roots, not its belly)
static bool GrpBounds(int g, Vector3* mn, Vector3* mx){
    bool any = false;
    auto acc = [&](Vector3 a, Vector3 b){
        if (!any){ *mn=a; *mx=b; any=true; return; }
        mn->x=fminf(mn->x,a.x); mn->y=fminf(mn->y,a.y); mn->z=fminf(mn->z,a.z);
        mx->x=fmaxf(mx->x,b.x); mx->y=fmaxf(mx->y,b.y); mx->z=fmaxf(mx->z,b.z);
    };
    for (auto& s : solids) if (s.grp==g) acc(s.mn, s.mx);
    for (auto& d : decor)  if (d.grp==g){ BoundingBox bb=DecorBB(d); acc(bb.min, bb.max); }
    for (auto& e : gCoins)      if (e.grp==g) acc(e.p, e.p);
    for (auto& e : gWebAnchors) if (e.grp==g) acc(e.pos, e.pos);
    for (auto& e : gSpores)     if (e.grp==g) acc(e.p, e.p);
    for (auto& e : gShrines)    if (e.grp==g) acc(e.p, e.p);
    for (auto& e : gUpdrafts)   if (e.grp==g) acc(e.base, e.base);
    for (auto& e : gWindmills)  if (e.grp==g) acc(e.pos, e.pos);
    for (auto& e : gBanners)    if (e.grp==g) acc(e.top, e.top);
    for (auto& e : gPinwheels)  if (e.grp==g) acc(e.pos, e.pos);
    for (auto& e : gMotes)      if (e.grp==g) acc(e.p, e.p);
    return any;
}
static void GrpScale(int g, float k){
    Vector3 mn, mx;
    if (!GrpBounds(g, &mn, &mx)) return;
    Vector3 pv = { (mn.x+mx.x)*0.5f, mn.y, (mn.z+mx.z)*0.5f };
    auto sp = [&](Vector3 p){ return pv + (p - pv)*k; };
    for (auto& s : solids) if (s.grp==g){
        if (s.isCyl){
            s.base = sp(s.base); s.rad *= k; s.hgt *= k;
            s.mn = {s.base.x-s.rad, s.base.y, s.base.z-s.rad};
            s.mx = {s.base.x+s.rad, s.base.y+s.hgt, s.base.z+s.rad};
        } else { s.mn = sp(s.mn); s.mx = sp(s.mx); }
    }
    for (auto& d : decor)       if (d.grp==g){ d.pos = sp(d.pos); d.scale = d.scale*k; }
    for (auto& e : gCoins)      if (e.grp==g) e.p = sp(e.p);
    for (auto& e : gWebAnchors) if (e.grp==g) e.pos = sp(e.pos);
    for (auto& e : gSpores)     if (e.grp==g) e.p = sp(e.p);
    for (auto& e : gShrines)    if (e.grp==g) e.p = sp(e.p);
    for (auto& e : gUpdrafts)   if (e.grp==g){ e.base = sp(e.base); e.rad *= k; e.hgt *= k; }
    for (auto& e : gWindmills)  if (e.grp==g){ e.pos = sp(e.pos); e.rad *= k; }
    for (auto& e : gBanners)    if (e.grp==g){ e.top = sp(e.top); e.len *= k; e.w *= k; }
    for (auto& e : gPinwheels)  if (e.grp==g){ e.pos = sp(e.pos); e.rad *= k; }
    for (auto& e : gMotes)      if (e.grp==g){ e.p = sp(e.p); e.r *= k; }
}
static void GrpDelete(int g){
    auto rmS = [&](std::vector<Solid>& v){ v.erase(std::remove_if(v.begin(), v.end(),
        [&](const Solid& e){ return e.grp==g; }), v.end()); };
    rmS(solids);
    decor.erase(std::remove_if(decor.begin(), decor.end(), [&](const Decor& e){ return e.grp==g; }), decor.end());
    gCoins.erase(std::remove_if(gCoins.begin(), gCoins.end(), [&](const Coin& e){ return e.grp==g; }), gCoins.end());
    gWebAnchors.erase(std::remove_if(gWebAnchors.begin(), gWebAnchors.end(), [&](const WebAnchor& e){ return e.grp==g; }), gWebAnchors.end());
    gSpores.erase(std::remove_if(gSpores.begin(), gSpores.end(), [&](const Spore& e){ return e.grp==g; }), gSpores.end());
    gShrines.erase(std::remove_if(gShrines.begin(), gShrines.end(), [&](const Shrine& e){ return e.grp==g; }), gShrines.end());
    gUpdrafts.erase(std::remove_if(gUpdrafts.begin(), gUpdrafts.end(), [&](const Updraft& e){ return e.grp==g; }), gUpdrafts.end());
    gWindmills.erase(std::remove_if(gWindmills.begin(), gWindmills.end(), [&](const Windmill& e){ return e.grp==g; }), gWindmills.end());
    gBanners.erase(std::remove_if(gBanners.begin(), gBanners.end(), [&](const Banner& e){ return e.grp==g; }), gBanners.end());
    gPinwheels.erase(std::remove_if(gPinwheels.begin(), gPinwheels.end(), [&](const Pinwheel& e){ return e.grp==g; }), gPinwheels.end());
    gMotes.erase(std::remove_if(gMotes.begin(), gMotes.end(), [&](const Mote& e){ return e.grp==g; }), gMotes.end());
}
static int GrpDuplicate(int g, Vector3 off){
    int ng = ++gGrp;
    size_t n;
    n = solids.size();      for (size_t i=0;i<n;i++) if (solids[i].grp==g){ Solid e=solids[i]; e.grp=ng; e.mn=e.mn+off; e.mx=e.mx+off; e.base=e.base+off; solids.push_back(e); }
    n = decor.size();       for (size_t i=0;i<n;i++) if (decor[i].grp==g){ Decor e=decor[i]; e.grp=ng; e.pos=e.pos+off; decor.push_back(e); }
    n = gCoins.size();      for (size_t i=0;i<n;i++) if (gCoins[i].grp==g){ Coin e=gCoins[i]; e.grp=ng; e.p=e.p+off; gCoins.push_back(e); }
    n = gWebAnchors.size(); for (size_t i=0;i<n;i++) if (gWebAnchors[i].grp==g){ WebAnchor e=gWebAnchors[i]; e.grp=ng; e.pos=e.pos+off; gWebAnchors.push_back(e); }
    n = gSpores.size();     for (size_t i=0;i<n;i++) if (gSpores[i].grp==g){ Spore e=gSpores[i]; e.grp=ng; e.p=e.p+off; gSpores.push_back(e); }
    n = gShrines.size();    for (size_t i=0;i<n;i++) if (gShrines[i].grp==g){ Shrine e=gShrines[i]; e.grp=ng; e.p=e.p+off; gShrines.push_back(e); }
    n = gUpdrafts.size();   for (size_t i=0;i<n;i++) if (gUpdrafts[i].grp==g){ Updraft e=gUpdrafts[i]; e.grp=ng; e.base=e.base+off; gUpdrafts.push_back(e); }
    n = gWindmills.size();  for (size_t i=0;i<n;i++) if (gWindmills[i].grp==g){ Windmill e=gWindmills[i]; e.grp=ng; e.pos=e.pos+off; gWindmills.push_back(e); }
    n = gBanners.size();    for (size_t i=0;i<n;i++) if (gBanners[i].grp==g){ Banner e=gBanners[i]; e.grp=ng; e.top=e.top+off; gBanners.push_back(e); }
    n = gPinwheels.size();  for (size_t i=0;i<n;i++) if (gPinwheels[i].grp==g){ Pinwheel e=gPinwheels[i]; e.grp=ng; e.pos=e.pos+off; gPinwheels.push_back(e); }
    n = gMotes.size();      for (size_t i=0;i<n;i++) if (gMotes[i].grp==g){ Mote e=gMotes[i]; e.grp=ng; e.p=e.p+off; gMotes.push_back(e); }
    return ng;
}

// --------------------------------------------------------------- aim point -
static Vector3 EditorAim(Ray ray){
    float best = 1e9f; bool hit = false; Vector3 p{};
    for (const Solid& s : solids){
        RayCollision rc = GetRayCollisionBox(ray, SolidBB(s));
        if (rc.hit && rc.distance > 0.3f && rc.distance < best){ best = rc.distance; p = rc.point; hit = true; }
    }
    if (!hit) p = ray.position + ray.direction*14.0f;
    return p;
}

// ------------------------------------------------------------ frame update -
static void EditorUpdate(float dt){
    FlyUpdate(dt);                                   // same fly camera as F3
    Camera3D cam = GetCam();
    int W = GetScreenWidth(), H = GetScreenHeight();
    Ray ray = GetScreenToWorldRay((Vector2){W*0.5f, H*0.5f}, cam);
    gEditAim = EditorAim(ray);

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) EditorPick(ray);
    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)){ gESelKind = ESEL_NONE; gESelGrp = -1; }

    // ---- nudge: arrows in the horizontal plane (camera-relative, snapped to
    //      world axes so moves are predictable), PgUp/PgDn vertical
    float step = IsKeyDown(KEY_LEFT_ALT)? 0.1f : 0.5f;
    Vector3 f = yawFwd(pl.yaw);
    Vector3 fs = (fabsf(f.x) > fabsf(f.z))? (Vector3){f.x>0?1.0f:-1.0f,0,0}
                                          : (Vector3){0,0,f.z>0?1.0f:-1.0f};
    Vector3 rs = { -fs.z, 0, fs.x };
    auto rep = [](int k){ return IsKeyPressed(k) || IsKeyPressedRepeat(k); };
    Vector3 mv = {0,0,0};
    if (rep(KEY_UP))        mv = mv + fs;
    if (rep(KEY_DOWN))      mv = mv - fs;
    if (rep(KEY_RIGHT))     mv = mv + rs;
    if (rep(KEY_LEFT))      mv = mv - rs;
    if (rep(KEY_PAGE_UP))   mv.y += 1;
    if (rep(KEY_PAGE_DOWN)) mv.y -= 1;
    if (mv.x != 0 || mv.y != 0 || mv.z != 0){
        Vector3 d = mv*step;
        if      (gESelKind == ESEL_GRP)   { GrpMove(gESelGrp, d); gEditDirty = true; }
        else if (gESelKind == ESEL_SPAWN) { gSpawn = gSpawn + d;  gEditDirty = true; }
        else if (gESelKind == ESEL_STAR)  { gStarP = gStarP + d;  gEditDirty = true; }
    }
    // ---- scale
    if (gESelKind == ESEL_GRP){
        if (rep(KEY_EQUAL)){ GrpScale(gESelGrp, 1.06f);      gEditDirty = true; }
        if (rep(KEY_MINUS)){ GrpScale(gESelGrp, 1.0f/1.06f); gEditDirty = true; }
        // ---- delete / duplicate
        if (IsKeyPressed(KEY_X) || IsKeyPressed(KEY_DELETE)){
            GrpDelete(gESelGrp); gESelKind = ESEL_NONE; gESelGrp = -1; gEditDirty = true;
        }
        if (IsKeyPressed(KEY_C)){
            gESelGrp = GrpDuplicate(gESelGrp, rs*3.0f); gEditDirty = true;
            MSG("duplicated - nudge it into place", 2.0f);
        }
    }
    // ---- drop prefabs at the crosshair
    Vector3 p = gEditAim;
    int born = -1;
    if (IsKeyPressed(KEY_ONE)){   addShroom(p, p.y+4.5f, 3.2f, false); born = gGrp; }
    if (IsKeyPressed(KEY_TWO)){   addShroom(p, p.y+4.5f, 3.4f, true);  born = gGrp; }
    if (IsKeyPressed(KEY_THREE)){ addPipe(p, 2.4f, 6.0f);              born = gGrp; }
    if (IsKeyPressed(KEY_FOUR)){  addBox({p.x-2.2f,p.y,p.z-2.2f},{p.x+2.2f,p.y+0.5f,p.z+2.2f}, S_WOOD); born = gGrp; }
    if (IsKeyPressed(KEY_FIVE)){  addQBlock({p.x, p.y+2.8f, p.z});     born = gGrp; }
    if (IsKeyPressed(KEY_SIX)){   addCoin(p.x, p.y+1.2f, p.z);         born = gGrp; }
    if (IsKeyPressed(KEY_SEVEN)){ addWebAnchor({p.x, p.y+6.0f, p.z});  born = gGrp; }
    if (IsKeyPressed(KEY_EIGHT)){ addSpore(p.x, p.y+1.2f, p.z);        born = gGrp; }
    if (IsKeyPressed(KEY_NINE)){  addUpdraft({p.x, p.y, p.z}, 5.0f, 12.0f, 16.0f); born = gGrp; }
    if (IsKeyPressed(KEY_ZERO)){  addSkyPlat(p.x, p.z, p.y+6.0f, 3.0f); born = gGrp; }
    if (born >= 0){ gESelKind = ESEL_GRP; gESelGrp = born; gEditDirty = true; }

    // ---- save / revert
    if (IsKeyPressed(KEY_F5)){
        if (SaveLevelFile(gLevel)){
            gEditDirty = false;
            MSG(TextFormat("saved levels/level%d.txt - this file IS the level now.", gLevel), 4.0f);
        } else MSG("could not write the level file!", 3.0f);
    }
    if (IsKeyPressed(KEY_F9)){
        BuildWorld(gLevel);                          // reload last save (or code default)
        gESelKind = ESEL_NONE; gESelGrp = -1; gEditDirty = false;
        MSG("level reloaded - unsaved edits discarded.", 3.0f);
    }
}

// ------------------------------------------------------------ 3D overlays --
static void DrawEditor3D(void){
    // collision-only solids are invisible in game - show their cages here
    for (const Solid& s : solids)
        if (!s.visible) DrawBoundingBox(SolidBB(s), (Color){80,180,255,110});
    // the selected group glows magenta
    if (gESelKind == ESEL_GRP){
        int g = gESelGrp;
        for (const Solid& s : solids) if (s.grp==g) DrawBoundingBox(SolidBB(s), MAGENTA);
        for (const Decor& d : decor)  if (d.grp==g) DrawBoundingBox(DecorBB(d), (Color){255,120,255,200});
        for (const Coin& c : gCoins)      if (c.grp==g) DrawSphereWires(c.p, 0.8f, 6, 8, MAGENTA);
        for (const WebAnchor& a : gWebAnchors) if (a.grp==g) DrawSphereWires(a.pos, 1.0f, 6, 8, MAGENTA);
        for (const Spore& s : gSpores)    if (s.grp==g) DrawSphereWires(s.p, 0.8f, 6, 8, MAGENTA);
        for (const Shrine& s : gShrines)  if (s.grp==g) DrawSphereWires(s.p, 1.4f, 6, 8, MAGENTA);
        for (const Updraft& u : gUpdrafts) if (u.grp==g)
            DrawBoundingBox({{u.base.x-u.rad,u.base.y,u.base.z-u.rad},
                             {u.base.x+u.rad,u.base.y+u.hgt,u.base.z+u.rad}}, MAGENTA);
        for (const Windmill& w : gWindmills) if (w.grp==g) DrawSphereWires(w.pos, w.rad, 8, 12, MAGENTA);
        for (const Banner& b : gBanners)  if (b.grp==g) DrawSphereWires(b.top, 0.9f, 6, 8, MAGENTA);
        for (const Pinwheel& pw : gPinwheels) if (pw.grp==g) DrawSphereWires(pw.pos, 0.9f, 6, 8, MAGENTA);
        for (const Mote& m : gMotes)      if (m.grp==g) DrawSphereWires(m.p, 1.0f, 6, 8, MAGENTA);
    }
    // spawn (green) and star (gold) are draggable markers too
    DrawSphereWires(gSpawn, 1.2f, 8, 10, gESelKind==ESEL_SPAWN? MAGENTA : (Color){90,240,110,255});
    DrawSphereWires(gStarP, 1.4f, 8, 10, gESelKind==ESEL_STAR?  MAGENTA : C_GOLD);
    // the crosshair's landing point (where prefabs drop)
    DrawSphereWires(gEditAim, 0.35f, 5, 6, (Color){255,240,120,230});
}

// ------------------------------------------------------------------- HUD ---
static void DrawEditorHUD(void){
    int W = GetScreenWidth(), H = GetScreenHeight();
    float t = (float)GetTime();
    float pu = 0.6f + 0.4f*sinf(t*4.0f);
    DrawRectangle(0, 0, W, 40, (Color){20,80,140,190});
    drawTextC(TextFormat("LEVEL EDITOR  ·  world %d  ·  %s  ·  F4 to exit", gLevel,
              gEditDirty? "UNSAVED EDITS - F5 to save" : "saved"),
              W/2, 10, 22, gEditDirty? (Color){255,210,90,(unsigned char)(200+55*pu)} : (Color){210,240,255,255});
    int y = 52;
    auto ln = [&](const char* s){ drawTextSh(s, 16, y, 17, (Color){232,238,248,225}); y += 24; };
    ln("WASD fly · SPACE/CTRL up/down · SHIFT boost");
    ln("LMB select object · RMB deselect");
    ln("ARROWS move · PGUP/PGDN raise/lower (ALT = fine)");
    ln("+ / - scale · C duplicate · X delete");
    ln("drop: 1 shroom 2 red 3 pipe 4 slab 5 ?block");
    ln("      6 coin 7 bloom 8 spore 9 updraft 0 skyplat");
    ln("F5 save level · F9 reload · blue cages = invisible collision");
    if (gESelKind == ESEL_GRP){
        Vector3 mn, mx;
        if (GrpBounds(gESelGrp, &mn, &mx))
            drawTextSh(TextFormat("selected: group %d  (%d parts)  center (%.1f, %.1f, %.1f)",
                       gESelGrp, GrpCount(gESelGrp),
                       (mn.x+mx.x)*0.5f, (mn.y+mx.y)*0.5f, (mn.z+mx.z)*0.5f),
                       16, y+6, 19, (Color){255,150,255,255});
    }
    else if (gESelKind == ESEL_SPAWN) drawTextSh("selected: SPAWN POINT", 16, y+6, 19, (Color){120,245,140,255});
    else if (gESelKind == ESEL_STAR)  drawTextSh("selected: THE STAR",    16, y+6, 19, C_GOLD);
    drawTextSh(TextFormat("pos  x %.1f  y %.1f  z %.1f", pl.pos.x, pl.pos.y, pl.pos.z),
               16, H-58, 18, (Color){140,235,140,255});
    drawTextSh(TextFormat("%d solids · %d decor · %d coins", (int)solids.size(), (int)decor.size(), (int)gCoins.size()),
               16, H-32, 15, (Color){200,210,225,190});
    DrawCircle(W/2, H/2, 3.0f, (Color){255,240,120,230});
}
