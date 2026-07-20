// ============================================================================
// world.h : level geometry - meadow routes, castle, courtyard routes, spire
//
// Route map (foddian, no checkpoints - but many roads up):
//   Zone 1  ground -> outer wall (16m)
//     [green flag]  West Ladder : tan shroom -> pipe -> two tall tans
//     [yellow flag] Pipe Yard   : five pipes of rising height (small pads)
//     [red flag]    Watchtower  : two PERFECT run-vaults skip everything
//   Zone 2  wall -> keep roof (46m)
//     (A) gate-tower slabs -> ?-block bridge -> keep balconies
//     (B) giant courtyard shroom stack -> east wall beams
//     (C) bold gap-bricks straight across the courtyard
//   Zone 3  keep roof -> spire top (90m) -> the star
//     (A) peg spiral round the bare mini-tower -> cloudwalk -> balcony ring
//     (B) broken brick stair spiraling the spire -> balcony ring
// ============================================================================
#pragma once
#include "game.h"

static std::vector<Solid> solids;
static std::vector<Decor> decor;
static bool gBakeDirty = true;     // static-decor mesh bake (gfx.h) needs a refresh

// texture slots (filled in gfx.h)
enum TexId { TX_WHITE, TX_GRASS, TX_STONE, TX_BRICK, TX_Q,
             TX_WOOD, TX_FIBER, TX_STREAK, TX_PIPEG, TX_SAND,
             TX_SKY, TX_CLOTH, TX_COUNT };

// ------------------------------------------------------------- palette -----
static const Color C_GRASS_A = {  98, 201,  72, 255 };
static const Color C_GRASS_B = {  90, 191,  65, 255 };
static const Color C_STONE_A = { 228, 224, 212, 255 };
static const Color C_STONE_B = { 217, 213, 200, 255 };
static const Color C_BRICK   = { 205, 115,  50, 255 };
static const Color C_BRICK_D = { 140,  66,  26, 255 };
static const Color C_PIPE    = {  62, 190,  60, 255 };
static const Color C_PIPE_D  = {  44, 150,  44, 255 };
static const Color C_RED     = { 235,  55,  40, 255 };
static const Color C_TAN     = { 231, 171,  95, 255 };
static const Color C_CREAM   = { 253, 240, 208, 255 };
static const Color C_CLOUD   = { 251, 251, 253, 255 };
static const Color C_WOOD    = { 165, 108,  50, 255 };
static const Color C_GOLD    = { 253, 192,  40, 255 };
static const Color C_ROOF    = { 222,  60,  48, 255 };

// ------------------------------------------------------------- builders ----
// LEVEL EDITOR groups: every primitive gets a group id. A composite builder
// (addShroom, addPipe, ...) opens a GrpScope so all its parts share one id and
// the editor can pick/move/scale/delete the whole thing as a unit. A bare
// primitive call at depth 0 gets its own fresh id.
static int gGrp = 0;             // last issued group id
static int gGrpDepth = 0;        // >0 while inside a composite builder
static int grpUse(void){ if (gGrpDepth == 0) gGrp++; return gGrp; }
struct GrpScope {
    GrpScope(){ if (gGrpDepth++ == 0) gGrp++; }
    ~GrpScope(){ gGrpDepth--; }
};

// THE GREAT ASCENT: level 6 stacks every world into one continuous climb.
// gBOff shifts every primitive a zone builder emits (applied in the LEAF
// adders only, so composite builders never double-offset). gBFlip mirrors
// local z first (the Bonewood is built flipped so its climb faces back over
// the realm). gMega makes the builders skip their standalone-only parts (own
// ground slab, warp pipe, horizon dressing) - BuildAscent connects them.
static Vector3 gBOff = {0,0,0};
static bool    gMega = false;
static bool    gBFlip = false;
static inline Vector3 bofs(Vector3 p){ if (gBFlip) p.z = -p.z; return p + gBOff; }
static Solid* addBox(Vector3 mn, Vector3 mx, int surf, bool bouncy=false, bool vis=true){
    Vector3 a = bofs(mn), b = bofs(mx);                 // a mirrored box swaps its z faces
    Solid s{}; s.isCyl=false;
    s.mn = { fminf(a.x,b.x), fminf(a.y,b.y), fminf(a.z,b.z) };
    s.mx = { fmaxf(a.x,b.x), fmaxf(a.y,b.y), fmaxf(a.z,b.z) };
    s.surf=surf; s.bouncy=bouncy; s.visible=vis;
    s.grp=grpUse();
    solids.push_back(s); return &solids.back();
}
static Solid* addCyl(Vector3 base, float rad, float hgt, int surf, bool bouncy=false, bool vis=true){
    Solid s{}; s.isCyl=true; s.base=bofs(base); s.rad=rad; s.hgt=hgt;
    s.mn={s.base.x-rad, s.base.y, s.base.z-rad}; s.mx={s.base.x+rad, s.base.y+hgt, s.base.z+rad};
    s.surf=surf; s.bouncy=bouncy; s.visible=vis;
    s.grp=grpUse();
    solids.push_back(s); return &solids.back();
}
static void addDecor(int kind, Vector3 pos, Vector3 scale, Color col, int tex=TX_WHITE, float rotY=0){
    decor.push_back({kind, bofs(pos), scale, col, tex, gBFlip? -rotY : rotY, grpUse()});
}
static void addWebAnchor(Vector3 pos, float radius=3.0f){ gWebAnchors.push_back({bofs(pos), radius, 0, grpUse()}); }

// mushroom cap only: collision cylinder + toadstool silhouette
// (overhanging rim lip + domed crown + pale gilled underside + spots)
static void addCap(float x, float z, float capTopY, float capR, bool red){
    GrpScope _g;
    // wide clamps: big caps read chunky, small caps read dainty
    float capH = clampf(capR*0.70f, 0.9f, 3.8f);
    addCyl({x, capTopY - capH, z}, capR*0.96f, capH, red ? S_SHROOM_RED : S_SHROOM_TAN, red, false);
    Color cc = red ? C_RED : C_TAN;
    float domeTop = capTopY + 0.28f, domeBot = capTopY - capH - 0.15f;
    float ry = (domeTop-domeBot)*0.5f;
    Vector3 capC = { x, (domeTop+domeBot)*0.5f, z };
    // rim lip: wider than the crown, flares past the edge like a real toadstool
    addDecor(D_SPHERE, {x, domeBot + capH*0.34f, z},
             {capR*1.14f, capH*0.42f, capR*1.14f}, ctint(cc,0.90f), TX_STREAK);
    // crown: narrower, taller - reads "cap", not "egg"
    addDecor(D_SPHERE, {capC.x, capC.y + 0.06f, capC.z},
             {capR*0.88f, ry*1.04f, capR*0.88f}, cc, TX_STREAK);
    // pale underside + gill shadow ring under the lip
    addDecor(D_CYL, {x, domeBot+0.02f, z}, {capR*0.86f, 0.18f, capR*0.86f}, (Color){250,242,222,255});
    addDecor(D_CYL, {x, domeBot+0.21f, z}, {capR*0.95f, 0.05f, capR*0.95f}, (Color){232,216,184,255});
    int nd = 3 + (int)(capR*1.1f);
    for (int i=0;i<nd;i++){
        float a  = (i + frnd(-0.18f,0.18f)) * 6.283f/nd;
        float rr = frnd(capR*0.26f, capR*0.56f);
        float dotR = clampf(capR*0.22f, 0.32f, 0.85f)*frnd(0.85f,1.15f);
        float dy = capC.y + 0.06f + ry*1.04f*sqrtf(fmaxf(0.0f, 1.0f - (rr*rr)/(capR*0.88f*capR*0.88f))) - dotR*0.22f;
        addDecor(D_SPHERE, {x+cosf(a)*rr, dy, z+sinf(a)*rr}, {dotR, dotR*0.45f, dotR}, C_CREAM);
    }
}
// full mushroom: stem + cap
static void addShroom(Vector3 groundPos, float capTopY, float capR, bool red){
    GrpScope _g;
    float capH   = clampf(capR*0.70f, 0.9f, 3.8f);
    float stemR  = clampf(capR*0.40f, 0.4f, 2.4f);
    float stemH  = capTopY - groundPos.y - capH*0.60f;
    addCyl(groundPos, stemR, stemH + 0.2f, S_DARK, false, false);                  // stem collision
    // stem: fat base flare, fibrous shaft, collar bulge under the cap
    addDecor(D_CYL, groundPos, {stemR*1.30f, fmaxf(stemH*0.30f,0.5f), stemR*1.30f}, (Color){247,236,210,255}, TX_FIBER);
    addDecor(D_CYL, groundPos, {stemR, stemH + 0.15f, stemR}, C_CREAM, TX_FIBER);
    addDecor(D_SPHERE, {groundPos.x, groundPos.y+stemH-0.25f, groundPos.z},
             {stemR*1.35f, 0.55f, stemR*1.35f}, (Color){246,233,203,255});
    addCap(groundPos.x, groundPos.z, capTopY, capR, red);
}
// tiny decorative button mushroom - pure decor, no collision, so patches of
// them can freckle the meadows without tripping the player
static void addMiniShroom(float x, float y, float z, float capR, bool red){
    GrpScope _g;
    float stemH = capR*frnd(1.1f, 1.9f);
    Color cc = red ? C_RED : C_TAN;
    addDecor(D_CYL, {x, y, z}, {capR*0.30f, stemH, capR*0.30f}, C_CREAM, TX_FIBER);
    addDecor(D_SPHERE, {x, y + stemH + capR*0.10f, z}, {capR, capR*0.62f, capR}, cc, TX_STREAK);
    addDecor(D_SPHERE, {x, y + stemH - 0.02f, z}, {capR*0.72f, capR*0.16f, capR*0.72f},
             (Color){250,242,222,255});
    int nd = (capR > 0.55f)? 3 : 2;
    for (int i=0;i<nd;i++){
        float a = i*(6.283f/nd) + x + z;                    // deterministic-ish scatter
        float rr = capR*frnd(0.25f, 0.55f);
        addDecor(D_SPHERE, {x+cosf(a)*rr, y + stemH + capR*0.52f, z+sinf(a)*rr},
                 {capR*0.22f, capR*0.10f, capR*0.22f}, C_CREAM);
    }
}
// a family of button shrooms huddled together - sizes vary wildly on purpose
static void addShroomPatch(float x, float y, float z, int n, float spread){
    GrpScope _g;
    for (int i=0;i<n;i++){
        float a = i*2.4f + x*0.7f;                          // golden-angle scatter
        float d = spread*(0.25f + 0.75f*((i+1)/(float)n));
        addMiniShroom(x + cosf(a)*d, y, z + sinf(a)*d,
                      frnd(0.30f, 1.15f), (i%3)==1);
    }
}
static void addPipe(Vector3 groundPos, float rad, float hgt){
    GrpScope _g;
    addCyl(groundPos, rad, hgt-0.7f, S_PIPE);
    addCyl({groundPos.x, groundPos.y+hgt-0.7f, groundPos.z}, rad+0.28f, 0.7f, S_PIPE); // lip
    addDecor(D_CYL, {groundPos.x, groundPos.y+hgt-0.06f, groundPos.z},
             {rad+0.02f, 0.08f, rad+0.02f}, C_PIPE_D); // dark opening rim
}
static void addQBlock(Vector3 topCenter, float s=2.2f){
    addBox({topCenter.x-s/2, topCenter.y-s, topCenter.z-s/2},
           {topCenter.x+s/2, topCenter.y,   topCenter.z+s/2}, S_QBLOCK);
}
static void addTowerRoof(Vector3 topCenter, float rad){
    GrpScope _g;
    addDecor(D_CYL,  {topCenter.x, topCenter.y-0.1f, topCenter.z}, {rad+0.5f, 0.5f, rad+0.5f}, C_STONE_B, TX_STONE);
    addDecor(D_CONE, {topCenter.x, topCenter.y+0.4f, topCenter.z}, {rad+0.7f, rad*1.5f, rad+0.7f}, C_ROOF);
    addDecor(D_SPHERE, {topCenter.x, topCenter.y+0.4f+rad*1.5f, topCenter.z}, {0.35f,0.35f,0.35f}, C_GOLD);
}
// route flag: pole + pennant (pure decor - marks where a path starts)
static void addFlag(Vector3 groundPos, Color col){
    GrpScope _g;
    addDecor(D_CYL,   groundPos, {0.08f, 3.0f, 0.08f}, C_WOOD);
    addDecor(D_CUBE,  {groundPos.x+0.5f, groundPos.y+2.62f, groundPos.z}, {0.9f, 0.45f, 0.06f}, col);
    addDecor(D_SPHERE,{groundPos.x, groundPos.y+3.08f, groundPos.z}, {0.14f,0.14f,0.14f}, C_GOLD);
}
// spinning collectible coins trace the routes (drawn + collected in main.cpp)
struct Coin { Vector3 p; bool taken; int grp; };
static std::vector<Coin> gCoins;
static void addCoin(float x, float y, float z){ gCoins.push_back({bofs({x,y,z}), false, grpUse()}); }
// hanging pennant banner on a wall face (face normal along -Z; rotY to reorient)
static void addBanner(Vector3 topCenter, Color col, float w=1.5f, float h=2.6f){
    GrpScope _g;
    addDecor(D_CUBE, {topCenter.x, topCenter.y-h*0.5f, topCenter.z}, {w, h, 0.14f}, col);
    addDecor(D_CUBE, {topCenter.x, topCenter.y+0.06f, topCenter.z}, {w+0.25f, 0.22f, 0.2f}, C_GOLD);
}
static void addTorch(Vector3 groundPos){
    GrpScope _g;
    addDecor(D_CYL,    groundPos, {0.09f, 2.7f, 0.09f}, C_WOOD);
    addDecor(D_SPHERE, {groundPos.x, groundPos.y+2.95f, groundPos.z}, {0.30f,0.42f,0.30f}, {255,140,30,255});
    addDecor(D_SPHERE, {groundPos.x, groundPos.y+3.06f, groundPos.z}, {0.16f,0.24f,0.16f}, {255,225,95,255});
}
static void addIvy(Vector3 base, float faceZ, int n){
    GrpScope _g;
    for (int i=0;i<n;i++)
        addDecor(D_SPHERE, {base.x+frnd(-1.4f,1.4f), base.y+frnd(0.3f,5.5f), faceZ},
                 {frnd(0.5f,1.0f), frnd(0.5f,0.9f), 0.35f}, {56,150,60,255});
}

// ------------------------------------------------------------ levels ------
static int     gLevel   = 0;            // 0 = castle, 1 = megashroom tower
static Vector3 gWarpTop = {0,0,0};      // stand here to switch levels
static int     gWarpGrp = -1;           // the warp pipe's group: moving it moves gWarpTop

// warp pipe: a fat green pipe + flag; standing on its lip cycles to the next level
static void addWarpPipe(Vector3 groundPos, Color flagCol){
    if (gMega) return;                 // one connected world: nowhere to warp
    GrpScope _g;
    gWarpGrp = gGrp;
    addPipe(groundPos, 2.0f, 2.6f);
    gWarpTop = bofs(groundPos) + (Vector3){0, 2.6f, 0};
    addFlag({groundPos.x + 2.9f, groundPos.y, groundPos.z}, flagCol);
    addDecor(D_SPHERE, {groundPos.x, groundPos.y + 3.6f, groundPos.z},
             {0.30f,0.30f,0.30f}, C_GOLD);         // beacon ball over the mouth
}
static void addSpore(float x, float y, float z){
    GrpScope _g;
    gSpores.push_back({bofs({x,y,z}), 0, gGrp});
    addDecor(D_CYL, {x, y-1.1f, z}, {0.09f, 1.0f, 0.09f}, {96,180,84,255});   // stem (orb drawn live)
}
// THUNDERSEED: a storm-fruit that loads the pole with n slam-bounces
static void addThunder(float x, float y, float z, int n){
    GrpScope _g;
    gThunders.push_back({bofs({x,y,z}), n, 0, gGrp});
    addDecor(D_CYL, {x, y-1.0f, z}, {0.11f, 0.9f, 0.11f}, {92,70,52,255}, TX_FIBER);   // gnarled stem
}
// power shrine: touch it and a new mechanic is yours (orb drawn live)
static void addShrine(Vector3 groundPos, int type){
    GrpScope _g;
    gShrines.push_back({bofs(groundPos) + (Vector3){0, 1.7f, 0}, type, gGrp});
    addCyl(groundPos, 1.2f, 0.9f, S_STONE);
    addDecor(D_CYL, {groundPos.x, groundPos.y + 0.88f, groundPos.z}, {1.35f,0.14f,1.35f}, C_GOLD);
    addDecor(D_CYL, {groundPos.x, groundPos.y - 0.02f, groundPos.z}, {1.6f,0.1f,1.6f}, C_STONE_B, TX_STONE);
}
static void addMote(Vector3 p, Color c, float r, float spd){ gMotes.push_back({bofs(p), c, r, spd, grpUse()}); }

// ---- SKYHAVEN builders ----------------------------------------------------
static void addUpdraft(Vector3 base, float rad, float hgt, float str=32.0f){
    gUpdrafts.push_back({bofs(base), rad, hgt, str, grpUse()});
}
static void addWindmill(Vector3 pos, float rad, float tilt, float spd, int blades, Color col, int hazard=0){
    gWindmills.push_back({bofs(pos), rad, tilt, spd, blades, col, hazard, grpUse()});
}
static void addStreamer(Vector3 top, float len, float w, Color col){
    gBanners.push_back({bofs(top), len, w, col, frnd(0,6.283f), grpUse()});
}
static void addPinwheel(Vector3 pos, float rad, float spd, Color a, Color b){
    GrpScope _g;
    addDecor(D_CYL, {pos.x, pos.y-1.3f, pos.z}, {0.07f, 1.3f, 0.07f}, C_WOOD);   // pole
    gPinwheels.push_back({bofs(pos), rad, spd, a, b, gGrp});
}
// a floating sky-stone platform (walkable disc + tapered underside + rim)
static void addSkyPlat(float x, float z, float top, float r){
    GrpScope _g;
    addCyl({x, top-1.6f, z}, r, 1.6f, S_SKYSTONE);
    addDecor(D_CYL, {x, top-0.28f, z}, {r+0.18f, 0.4f, r+0.18f}, (Color){236,242,252,255}, TX_SKY);
    addDecor(D_CONE, {x, top-4.6f, z}, {r*0.9f, 3.2f, r*0.9f}, (Color){206,216,234,255}, TX_SKY);   // underside
    addCoin(x, top+1.3f, z);
}
// floating grass island: turf disc + dirt belly + lip (Sporeway + the seams)
static void addIsle(float x, float z, float top, float r){
    GrpScope _g;
    addCyl({x, top-2.2f, z}, r, 2.2f, S_GRASS);
    addDecor(D_SPHERE, {x, top-2.5f, z}, {r*0.94f, 2.8f, r*0.94f}, (Color){124,88,52,255});
    addDecor(D_SPHERE, {x, top-4.4f, z}, {r*0.45f, 1.6f, r*0.45f}, (Color){104,72,42,255});
    addDecor(D_CYL, {x, top-0.32f, z}, {r+0.18f, 0.38f, r+0.18f}, C_GRASS_A, TX_GRASS);
    addCoin(x, top+1.3f, z);
}

// ------------------------------------------------------- level 0: castle --
static void BuildCastle(void){

    gSpawn = SPAWN_POS;
    gStarP = STAR_POS;

    // ---- meadow ------------------------------------------------------------
    addBox({-220,-2,-80},{220,0,280}, S_GRASS);

    // warp pipe to THE MEGASHROOM (level 2), just right of spawn
    addWarpPipe({12,0,-8}, {80,220,90,255});

    // tutorial brick wall (forces the first vault)
    addBox({-15,0,13.4f},{15,2.6f,16.6f}, S_BRICK);

    // the first world teaches vault + bounce only - no webs here.
    // meadow butterflies
    addMote({  4, 2.2f,  6}, {255,250,235,255}, 2.6f, 0.9f);
    addMote({-20, 2.6f, 30}, {255,225,120,255}, 3.0f, 0.7f);
    addMote({ 24, 2.4f, 48}, {255,250,235,255}, 2.4f, 1.1f);
    addMote({ -8, 3.0f, 90}, {255,225,120,255}, 3.2f, 0.8f);

    // dirt path: spawn -> wall -> gate (pure decor, sells the "way in")
    {
        float px[19] = { 0, 0.6f, 0, -0.5f, 0, 0.4f,   1.5f, 2.5f, 3.5f, 3.2f,
                         2.5f, 1.5f, 0.5f, -0.5f, 0, 0, 0, 0, 0 };
        float pz[19] = { -6, -2.5f, 1, 4.5f, 8, 11.5f,  19, 23, 27, 31,
                         35, 39, 43, 47, 51, 55, 59, 63.5f, 67.5f };
        for (int i=0;i<19;i++)
            addDecor(D_CYL, {px[i], 0.03f, pz[i]}, {frnd(1.4f,1.8f), 0.025f, frnd(1.1f,1.5f)},
                     (i%2)? (Color){203,172,112,255} : (Color){193,161,103,255});
    }

    // route flags just past the wall - pick a lane
    addFlag({10.5f,0,17.0f}, { 80,220, 90,255});   // green  : west ladder (steady)
    addFlag({16.0f,0,22.0f}, {255,210, 60,255});   // yellow : pipe yard (precision)
    addFlag({ 4.0f,0,35.0f}, C_RED);               // red    : watchtower skip (expert)

    // ---- zone 1 / WEST LADDER: shroom -> pipe -> two tall tans -> wall ------
    // sizes taper as the ladder climbs: fat welcoming first cap, tight crux
    addShroom({  7,0, 20},  4.8f, 3.9f, false);    // tan A - big and friendly (tall enough to walk under)
    addPipe  ({  0,0, 29},  2.4f, 7.0f);           // mid pipe
    addShroom({ -8,0, 40}, 11.5f, 2.8f, false);    // tan B (the sprint-vault crux - small)
    addShroom({-14,0, 51}, 13.5f, 3.4f, false);    // tan C - a breather
    addShroom({-10,0, 62}, 15.5f, 2.9f, false);    // tan D -> wall top (16)
    addShroom({-26,0, 50},  5.0f, 4.5f, true);     // red bail west - a fat parachute
    addShroom({ -2,0, 64},  4.5f, 2.4f, true);     // red bail near wall - small button
    addShroomPatch(3, 0, 22, 5, 2.6f);             // button family by tan A

    // ---- zone 1 / EAST PIPE YARD: five pipes, small pads, rising ------------
    addPipe({18,0,26}, 2.4f,  4.5f);
    addPipe({24,0,34}, 2.4f,  8.0f);
    addPipe({19,0,43}, 2.4f, 11.5f);
    addPipe({25,0,52}, 2.4f, 14.0f);
    addPipe({20,0,62}, 2.7f, 15.5f);               // fat final pipe -> wall top
    addShroom({28,0,44}, 5.0f, 3.9f, true);        // red bail east

    // ---- zone 1 / CENTER WATCHTOWER: two PERFECT run-vaults skip the lot ----
    addBox({2.5f,0,57.0f},{6.0f,8.0f,60.5f}, S_BRICK);
    addDecor(D_CUBE,  {4.25f,4.8f,56.9f}, {1.2f,1.8f,0.25f}, {40,34,30,255});  // window
    addDecor(D_SPHERE,{2.9f,8.32f,57.4f}, {0.28f,0.28f,0.28f}, C_GOLD);        // finial
    addShroom({11,0,52}, 4.0f, 2.6f, true);        // red bail center - small

    // bonus meadow ?-blocks (side hops for coins-and-grins energy)
    addQBlock({-30, 7, 24}, 2.6f);
    addQBlock({ 36, 9, 40}, 2.6f);
    addCoin(-30, 8.3f, 24); addCoin(36, 10.3f, 40);

    // zone-1 coins: an arc over the first wall teaches the vault's shape
    addCoin(0, 3.4f, 12.5f); addCoin(0, 4.3f, 15); addCoin(0, 3.4f, 17.5f);
    addCoin(  7, 5.3f, 20); addCoin( 0, 8.3f, 29); addCoin( -8, 12.8f, 40);
    addCoin(-14, 14.8f, 51); addCoin(-10, 16.8f, 62);
    addCoin( 18, 5.8f, 26); addCoin(24, 9.3f, 34); addCoin( 19, 12.8f, 43);
    addCoin( 25, 15.3f, 52); addCoin(20, 16.8f, 62);
    addCoin(4.25f, 9.3f, 58.8f); addCoin(4, 13.5f, 63.5f); addCoin(4, 15.5f, 67);

    // decorative meadow shrooms (non-path flavor, far from routes) - a real
    // spread now: towering landmarks down to knee-high buttons
    addShroom({ 45,0, 55},  9.0f, 5.5f, false);
    addShroom({-55,0, 30},  5.0f, 2.6f, false);
    addShroom({-70,0, 75}, 11.0f, 6.4f, true);     // giant red sentinel
    addShroom({ 70,0, 90}, 14.0f, 7.8f, false);    // the meadow colossus
    addShroom({ 52,0, 48},  2.6f, 1.5f, false);    // its little companion
    addShroom({-60,0, 36},  3.2f, 1.8f, true);
    addShroomPatch(-48, 0,  22, 6, 3.2f);          // button families in the grass
    addShroomPatch( 62, 0,  84, 7, 3.8f);
    addShroomPatch(-14, 0, -16, 5, 2.8f);          // right by spawn: scale is set early

    // flowers + bushes
    for (int i=0;i<70;i++){
        float x = frnd(-90,90), z = frnd(-60,120);
        if (fabsf(x)<32 && z>8 && z<80) continue;            // keep the lanes clear
        Color heads[4] = {{255,90,90,255},{255,200,60,255},{255,255,255,255},{255,120,200,255}};
        addDecor(D_CYL,   {x,0,z},        {0.06f,0.55f,0.06f}, {60,150,50,255});
        addDecor(D_SPHERE,{x,0.62f,z},    {0.2f,0.2f,0.2f},  heads[GetRandomValue(0,3)]);
    }
    for (int i=0;i<24;i++){
        float x = frnd(-120,120), z = frnd(-70,60);
        if (fabsf(x)<20 && z>5) continue;
        float s = frnd(1.2f,2.6f);
        addDecor(D_SPHERE,{x,0.2f,z},{s,s*0.62f,s},{58,168,64,255});
        addDecor(D_SPHERE,{x+s*0.7f,0.15f,z+s*0.3f},{s*0.7f,s*0.45f,s*0.7f},{52,158,58,255});
    }

    // ---- castle outer wall (front z=70..74, gate at x in [-3,3]) -----------
    addBox({-45,0,70},{-3,16,74}, S_STONE);
    addBox({  3,0,70},{45,16,74}, S_STONE);
    addBox({ -3,8,70},{ 3,16,74}, S_STONE);                       // lintel over gate
    addDecor(D_CUBE, {0,4.0f,70.4f}, {6.0f,8.0f,0.3f}, {35,28,24,255}); // dark gate inset
    addDecor(D_CUBE, {0,8.6f,69.85f}, {7.0f,1.2f,0.4f}, {242,236,218,255});    // gate arch band
    addTorch({-3.7f,0,69.3f}); addTorch({3.7f,0,69.3f});
    // cornice + plinth trim on the front face (silhouette detail)
    addDecor(D_CUBE, {-24,16.06f,69.72f}, {42,0.42f,0.75f}, {242,236,218,255});
    addDecor(D_CUBE, { 24,16.06f,69.72f}, {42,0.42f,0.75f}, {242,236,218,255});
    addDecor(D_CUBE, {-24, 0.70f,69.72f}, {42,1.4f, 0.75f}, {186,180,166,255});
    addDecor(D_CUBE, { 24, 0.70f,69.72f}, {42,1.4f, 0.75f}, {186,180,166,255});
    // hanging pennant banners between the merlons
    for (int i=0;i<8;i++){
        float bx[8] = {-41,-33,-25,-17, 17, 25, 33, 41};
        addBanner({bx[i], 14.8f, 69.85f}, (i%2)? (Color){80,110,230,255} : C_RED);
    }
    addIvy({-36,0,0}, 69.75f, 7); addIvy({22,0,0}, 69.75f, 6);
    addBox({-45,0,74},{-41,16,170}, S_STONE);                     // west wall
    addBox({ 41,0,74},{ 45,16,170}, S_STONE);                     // east wall
    addBox({-45,0,166},{45,16,170}, S_STONE);                     // back wall
    // merlons along the front battlement (tap-vault hurdles)
    for (float x=-43; x<=43; x+=4.0f){
        if (x>-13 && x<13) continue;                              // clear near gate towers
        addBox({x-0.75f,16,70},{x+0.75f,16.9f,71.1f}, S_STONE);
        addBox({x-0.75f,16,72.9f},{x+0.75f,16.9f,74}, S_STONE);
    }
    // corner towers (visual anchors; also solid)
    Vector3 corners[4] = {{-45,0,70},{45,0,70},{-45,0,170},{45,0,170}};
    for (int i=0;i<4;i++){
        addCyl(corners[i], 6, 24, S_STONE);
        addTowerRoof({corners[i].x, 24, corners[i].z}, 6);
    }
    // gate towers
    addBox({-10,0,69},{-4,23.5f,75}, S_STONE);                    // west gate tower
    addBox({  4,0,69},{10,23.5f,75}, S_STONE);                    // east gate tower
    addDecor(D_CUBE, {-7,24.1f,72}, {6.4f,0.3f,6.4f}, C_STONE_B, TX_STONE); // tower top trim
    addDecor(D_CUBE, { 7,24.1f,72}, {6.4f,0.3f,6.4f}, C_STONE_B, TX_STONE);
    addTowerRoof({7,26.0f,72}, 3.4f);                             // east tower gets a roof (west is the path)
    // arrow slits on the gate towers
    addDecor(D_CUBE, {-8.2f, 9,68.88f}, {0.5f,1.7f,0.22f}, {40,34,30,255});
    addDecor(D_CUBE, {-5.8f,15,68.88f}, {0.5f,1.7f,0.22f}, {40,34,30,255});
    addDecor(D_CUBE, { 5.8f, 9,68.88f}, {0.5f,1.7f,0.22f}, {40,34,30,255});
    addDecor(D_CUBE, { 8.2f,15,68.88f}, {0.5f,1.7f,0.22f}, {40,34,30,255});
    // wall-walk coins
    addCoin(-12, 17.4f, 72); addCoin(0, 17.4f, 72); addCoin(12, 17.4f, 72);
    addCoin(-7, 24.9f, 72);

    // ---- zone 2 / ROUTE A: gate-tower slabs -> ?-block bridge -> balconies --
    addBox({-15.0f,19.7f,72.6f},{-11.8f,20.0f,75.4f}, S_WOOD);    // slab 1 (from wall top)
    addBox({-15.0f,22.9f,68.6f},{-11.8f,23.2f,71.4f}, S_WOOD);    // slab 2 -> tower top 23.5
    addCoin(-13.4f, 21.2f, 74.0f); addCoin(-13.4f, 24.4f, 70.0f);
    addQBlock({ 3.0f, 25.0f,  80}, 3.0f);                         // ?-block bridge
    addQBlock({-2.0f, 27.5f,  88}, 3.0f);
    addQBlock({ 3.0f, 30.5f,  97}, 3.0f);
    addQBlock({-1.0f, 33.0f, 106}, 3.0f);
    addCoin(3, 26.3f, 80); addCoin(-2, 28.8f, 88); addCoin(3, 31.8f, 97); addCoin(-1, 34.3f, 106);

    // ---- zone 2 / ROUTE B: giant shroom stack -> east keep beams ------------
    // the stack shrinks as it rises: broad base, nervy top
    addShroom({22,0, 86}, 21.0f, 5.4f, false);                    // G1 (vault on from wall top)
    addShroom({26,0,100}, 26.0f, 4.0f, false);                    // G2
    addShroom({19,0,112}, 31.0f, 3.1f, false);                    // G3 - the tight one
    addBox({18,35.2f,120.6f},{20.8f,35.7f,123.6f}, S_WOOD);       // beams bolted to keep's east face
    addBox({18,39.7f,129.6f},{20.8f,40.2f,132.6f}, S_WOOD);
    addBox({18,44.0f,138.6f},{20.8f,44.5f,141.6f}, S_WOOD);       // -> keep roof 46
    addCoin(22, 22.3f, 86); addCoin(26, 27.3f, 100); addCoin(19, 32.3f, 112);
    addCoin(19, 36.9f, 122.1f); addCoin(19, 41.4f, 131.1f); addCoin(19, 45.7f, 140.1f);

    // ---- zone 2 / ROUTE C: bold gap-bricks across the courtyard -------------
    addBox({11.8f,20.7f,83.8f},{16.2f,21.0f,88.2f}, S_BRICK);     // run-vault from wall top
    addBox({ 5.8f,25.7f,94.8f},{10.2f,26.0f,99.2f}, S_BRICK);
    addBox({-0.2f,30.7f,104.8f},{ 4.2f,31.0f,109.2f}, S_BRICK);   // -> first balcony
    addCoin(14, 22.3f, 86); addCoin(8, 27.3f, 97); addCoin(2, 32.3f, 107);

    // courtyard red bails + flavor + fountain (bails vary: fat, button, mid)
    addShroom({-10,0, 85}, 5.0f, 4.3f, true);
    addShroom({  4,0, 95}, 4.2f, 2.5f, true);
    addShroom({ -2,0,108}, 5.5f, 3.6f, true);
    addShroom({-20,0,120}, 7.5f, 5.6f, false);     // big courtyard shade-cap
    addShroomPatch(-24, 0, 128, 5, 2.6f);
    addCyl({-15,0,98}, 2.6f, 0.9f, S_STONE);                      // fountain rim (vaultable)
    addDecor(D_CYL, {-15,0.04f,98}, {2.25f,0.84f,2.25f}, {96,192,240,255});   // water
    addCyl({-15,0.9f,98}, 0.35f, 1.3f, S_STONE);                  // center column
    addDecor(D_CYL, {-15,2.2f,98}, {0.85f,0.18f,0.85f}, C_STONE_B, TX_STONE); // basin
    addDecor(D_CYL, {-15,2.35f,98}, {0.13f,0.95f,0.13f}, {160,220,250,255});  // jet
    addDecor(D_SPHERE, {-15,3.45f,98}, {0.3f,0.22f,0.3f}, {200,235,252,255});
    for (int i=0;i<16;i++){                       // courtyard flowers
        float x=frnd(-36,36), z=frnd(78,160);
        if (fabsf(x)<12 && z<118) continue;
        addDecor(D_CYL,{x,0,z},{0.06f,0.5f,0.06f},{60,150,50,255});
        addDecor(D_SPHERE,{x,0.56f,z},{0.18f,0.18f,0.18f},{255,220,80,255});
    }

    // ---- the keep -----------------------------------------------------------
    addBox({-18,0,118},{18,46,154}, S_STONE);
    // fake windows on the front + side faces
    for (int wy=0; wy<3; wy++) for (int wx=-1; wx<=1; wx++)
        addDecor(D_CUBE, {wx*10.0f, 12.0f+wy*11.0f, 117.8f}, {1.6f,2.6f,0.3f}, {40,34,30,255});
    for (int wy=0; wy<2; wy++) for (int wz=0; wz<3; wz++){
        addDecor(D_CUBE, { 18.1f, 14.0f+wy*12.0f, 126.0f+wz*10.0f}, {0.3f,2.4f,1.5f}, {40,34,30,255});
        addDecor(D_CUBE, {-18.1f, 14.0f+wy*12.0f, 126.0f+wz*10.0f}, {0.3f,2.4f,1.5f}, {40,34,30,255});
    }
    // cornice + plinth + banners on the keep's face
    addDecor(D_CUBE, {0,46.05f,117.75f}, {36,0.5f,0.8f}, {242,236,218,255});
    addDecor(D_CUBE, {0, 0.90f,117.70f}, {36,1.8f,0.8f}, {186,180,166,255});
    addBanner({-10,43.0f,117.85f}, C_RED, 2.4f, 4.2f);
    addBanner({ 10,43.0f,117.85f}, C_RED, 2.4f, 4.2f);
    addIvy({-15,0,0}, 117.72f, 6);
    // decorative battlement teeth around the keep roof edge (no collision)
    for (float x=-16; x<=16; x+=4.5f){
        addDecor(D_CUBE, {x,46.45f,118.6f}, {1.4f,0.9f,1.2f}, C_STONE_B, TX_STONE);
        addDecor(D_CUBE, {x,46.45f,153.4f}, {1.4f,0.9f,1.2f}, C_STONE_B, TX_STONE);
    }
    for (float z=122; z<=150; z+=4.5f){
        addDecor(D_CUBE, { 17.4f,46.45f,z}, {1.2f,0.9f,1.4f}, C_STONE_B, TX_STONE);
        addDecor(D_CUBE, {-17.4f,46.45f,z}, {1.2f,0.9f,1.4f}, C_STONE_B, TX_STONE);
    }
    // balcony ladder up the keep's front face (routes A & C converge here)
    addBox({ -2.0f,33.2f,115.2f},{ 2.0f,34.0f,118}, S_WOOD);      // b0
    addBox({-10.0f,37.2f,115.2f},{-6.0f,38.0f,118}, S_WOOD);      // b1
    addBox({-16.4f,41.2f,115.2f},{-12.8f,42.0f,118}, S_WOOD);     // b2 -> keep roof 46
    addCoin(0, 35.2f, 117); addCoin(-8, 39.2f, 117); addCoin(-14.5f, 43.2f, 117);
    addCoin(0, 47.4f, 128); addCoin(6, 47.4f, 144);
    // mini-towers on keep roof corners
    Vector3 kc[4] = {{-13.5f,46,122.5f},{13.5f,46,122.5f},{-13.5f,46,149.5f},{13.5f,46,149.5f}};
    for (int i=0;i<4;i++){
        addCyl(kc[i], 3.5f, 14, S_STONE);
        if (i!=3) addTowerRoof({kc[i].x, 60, kc[i].z}, 3.5f);
    }
    addDecor(D_CYL, {13.5f,60.02f,149.5f},{3.9f,0.25f,3.9f}, C_GOLD);  // gold trim: the peg tower

    // ---- zone 3 / ROUTE A: peg spiral -> cloudwalk -> balcony ring ----------
    for (int k=0;k<6;k++){
        float a = k*60.0f*DEG2RAD;
        float px = 13.5f + cosf(a)*4.6f, pz = 149.5f + sinf(a)*4.6f;
        float py = 48.2f + 2.2f*k;                                 // tap-hop rises
        addBox({px-1.4f,py-0.3f,pz-1.4f},{px+1.4f,py,pz+1.4f}, S_WOOD);
        if (k==2 || k==4) addCoin(px, py+1.3f, pz);
    }
    addCoin(13.5f, 61.3f, 149.5f);
    struct { float x,z,top; } clouds[6] = {
        {  8, 143.0f, 64.5f}, { 10, 134.0f, 69.0f}, {  3, 126.0f, 73.5f},
        { -6, 127.0f, 78.0f}, {-10, 138.0f, 82.5f}, { -7, 147.5f, 87.5f} };
    for (int i=0;i<6;i++){
        addCyl({clouds[i].x, clouds[i].top-0.9f, clouds[i].z}, 3.1f, 0.9f, S_CLOUD, false, false);
        addCoin(clouds[i].x, clouds[i].top+1.3f, clouds[i].z);
        // fluffy visual: 3 overlapping squashed spheres
        Vector3 c = {clouds[i].x, clouds[i].top-0.55f, clouds[i].z};
        addDecor(D_SPHERE, c, {3.2f,0.95f,3.2f}, C_CLOUD);
        addDecor(D_SPHERE, {c.x-1.7f,c.y-0.1f,c.z+0.5f}, {1.9f,0.75f,1.9f}, C_CLOUD);
        addDecor(D_SPHERE, {c.x+1.6f,c.y-0.1f,c.z-0.4f}, {2.0f,0.8f,2.0f}, C_CLOUD);
    }

    // ---- zone 3 / ROUTE B: broken brick stair spiraling the spire -----------
    struct { float x,z,top; } spiral[7] = {
        {-11, 127.0f, 51.0f}, {-13, 139.0f, 56.0f}, { -5, 147.0f, 61.0f},
        {  5, 145.0f, 66.0f}, { 10, 136.0f, 71.0f}, {  6, 127.0f, 76.0f},
        { -3, 124.0f, 80.5f} };                                    // -> balcony ring 84
    for (int i=0;i<7;i++){
        addBox({spiral[i].x-2.1f, spiral[i].top-0.5f, spiral[i].z-2.1f},
               {spiral[i].x+2.1f, spiral[i].top,      spiral[i].z+2.1f}, S_BRICK);
        if (i==0 || i==2 || i==4 || i==6) addCoin(spiral[i].x, spiral[i].top+1.3f, spiral[i].z);
    }

    // ---- spire ---------------------------------------------------------------
    addCyl({0,46,136}, 5, 44, S_STONE);                             // spire shaft: 46..90
    addCyl({0,83.2f,136}, 7.8f, 0.8f, S_STONE);                     // balcony ring at 84
    addDecor(D_CYL, {0,88.55f,136}, {5.35f,0.45f,5.35f}, C_GOLD);   // gold collar under the cap
    // THE FINALE: a giant red trampoline cap seals the spire top - there is
    // nowhere to stand up there. Vault onto it and the bounce carries you
    // through the star. Reds are not optional anymore.
    addCyl({0,90,136}, 5.2f, 0.9f, S_SHROOM_RED, true, false);    // overhangs: no standable rim
    addDecor(D_SPHERE, {0,88.8f,136}, {6.1f,0.85f,6.1f}, ctint(C_RED,0.90f), TX_STREAK);   // rim lip
    addDecor(D_SPHERE, {0,89.6f,136}, {5.4f,1.6f,5.4f}, C_RED, TX_STREAK);
    addDecor(D_CYL, {0,88.15f,136}, {4.6f,0.22f,4.6f}, {250,242,222,255});
    for (int k=0;k<7;k++){
        float a = k*0.898f, rr = frnd(1.2f,4.0f);
        float dy = 89.6f + 1.6f*sqrtf(fmaxf(0.0f, 1.0f - (rr*rr)/(5.6f*5.6f))) - 0.10f;
        addDecor(D_SPHERE, {sinf(a)*rr, dy, 136+cosf(a)*rr}, {0.70f,0.30f,0.70f}, C_CREAM);
    }
    for (int k=0;k<3;k++){                                          // slit windows up the shaft
        float a = (20.0f + k*120.0f)*DEG2RAD;
        addDecor(D_CUBE, {sinf(a)*4.95f, 55.0f+k*11.0f, 136+cosf(a)*4.95f},
                 {1.0f,1.8f,0.35f}, {40,34,30,255}, TX_WHITE, 20.0f+k*120.0f);
    }
    addCoin(0, 85.4f, 129.4f); addCoin(4.7f, 85.4f, 141);           // on the balcony ring
    addCoin(0, 92.4f, 133.2f);                                      // grabbed mid-bounce

    // ---- background hills (pure decor, softened by fog) -----------------------
    for (int i=0;i<9;i++){
        float ang = -0.6f + i*0.35f;
        float d = frnd(300,380);
        float x = sinf(ang)*d, z = 100 + cosf(ang)*d;
        float s = frnd(60,130);
        addDecor(D_SPHERE, {x,-s*0.55f,z}, {s, s*0.8f, s}, {70,172,88,255});
    }
}

// -------------------------------------------- level 1: THE MEGASHROOM -----
// One colossal mushroom. Shelf-caps spiral tightly around its stalk: the
// whole climb lives inside a ~10 m column, so every fall drops back down
// your own route - reds you passed are your parachutes. Rises are taller,
// caps are smaller, three gaps are blocked by the stalk itself and must be
// bounce-hopped off a red, and one gate demands a PERFECT.
static void BuildMegashroom(void){
    const float CX = 0, CZ = 40;              // stalk axis
    gSpawn = { 0, 0.91f, 16.0f };
    gStarP = { CX, 150.7f, CZ };

    if (!gMega) addBox({-220,-2,-80},{220,0,280}, S_GRASS);   // ascent: grows on the crag plateau
    addWarpPipe({10,0,12}, {90,160,255,255}); // onward to THE SPOREWAY


    // the stalk (collision) + segmented cream visual with gentle bulges
    addCyl({CX,0,CZ}, 3.0f, 146.0f, S_DARK, false, false);
    for (int s=0;s<10;s++){
        float y0 = s*14.6f;
        float r  = 3.15f + 0.25f*sinf(s*1.7f);
        addDecor(D_CYL, {CX, y0, CZ}, {r, 14.8f, r}, (s%2)? C_CREAM : (Color){247,236,210,255}, TX_FIBER);
    }
    addDecor(D_CYL, {CX, -0.02f, CZ}, {4.6f, 1.2f, 4.6f}, (Color){240,228,198,255}, TX_FIBER);  // root flare
    // leaf pads (decor only)
    float leafY[5] = {22, 48, 77, 104, 131};
    for (int i=0;i<5;i++){
        float a = (55 + i*140)*DEG2RAD;
        addDecor(D_SPHERE, {CX+sinf(a)*4.9f, leafY[i], CZ+cosf(a)*4.9f},
                 {3.1f, 0.32f, 1.45f}, {92,190,84,255}, TX_WHITE, 90.0f - (55+i*140));
    }
    // tiny bracket fungi freckling the stalk between shelves (pure decor)
    for (int k=0;k<16;k++){
        float a = k*2.4f;                          // golden-angle scatter
        float y = 9.0f + k*8.4f;
        Color fc = (k%3==0)? ctint(C_RED,0.95f) : (k%2)? C_TAN : (Color){236,220,190,255};
        addDecor(D_SPHERE, {CX+sinf(a)*3.15f, y, CZ+cosf(a)*3.15f}, {0.85f,0.24f,0.85f}, fc, TX_STREAK);
        addDecor(D_SPHERE, {CX+sinf(a)*3.05f, y-0.14f, CZ+cosf(a)*3.05f}, {0.6f,0.12f,0.6f}, C_CREAM);
    }
    // glow-shrooms ring the roots; spore motes drift up the stalk
    for (int k=0;k<6;k++){
        float a = k*1.047f + 0.4f;
        float gx = CX+sinf(a)*5.6f, gz = CZ+cosf(a)*5.6f;
        addDecor(D_CYL, {gx, 0, gz}, {0.12f, 0.75f, 0.12f}, C_CREAM, TX_FIBER);
        addDecor(D_SPHERE, {gx, 0.85f, gz}, {0.42f,0.30f,0.42f}, (Color){150,232,255,255});
    }
    addMote({CX+5, 12, CZ+4}, {255,240,180,255}, 2.8f, 0.6f);
    addMote({CX-5, 42, CZ-3}, {235,250,255,255}, 3.2f, 0.5f);
    addMote({CX+4, 74, CZ-5}, {255,240,180,255}, 3.0f, 0.7f);
    addMote({CX-4, 105, CZ+5}, {235,250,255,255}, 3.4f, 0.55f);
    addMote({CX, 136, CZ+6}, {255,240,180,255}, 3.0f, 0.65f);
    // a halo of clouds keeps the crown company
    for (int k=0;k<5;k++){
        float a = k*1.257f;
        Vector3 c = {CX+sinf(a)*14.5f, 140.5f+sinf(k*2.1f)*2.0f, CZ+cosf(a)*14.5f};
        addDecor(D_SPHERE, c, {3.4f,1.0f,3.4f}, C_CLOUD);
        addDecor(D_SPHERE, {c.x+1.8f,c.y-0.15f,c.z-0.5f}, {2.1f,0.8f,2.1f}, C_CLOUD);
    }
    if (!gMega){
        // an ELDER megashroom broods on the horizon - you are climbing its child
        addDecor(D_CYL, {-135, 0, 190}, {9.5f, 62.0f, 9.5f}, (Color){238,226,200,255}, TX_FIBER);
        addCap(-135, 190, 68.0f, 26.0f, false);
        addDecor(D_CYL, {148, 0, 148}, {6.0f, 38.0f, 6.0f}, (Color){241,230,206,255}, TX_FIBER);
        addCap(148, 148, 42.0f, 16.0f, true);
        // a forest of lesser kin rings the clearing
        float fx[9] = {-62, -84, -48,  58,  80,  66, -20,  30, -70};
        float fz[9] = { -8,  60, 118, -18,  52, 118, -52, -58, -44};
        float fh[9] = { 11,  22,  6,  13,  26,  10,  4,  16,  14};
        float fr[9] = {5.2f,9.4f,3.2f,5.8f,11.0f,4.4f,2.2f,6.6f,7.6f};
        for (int i=0;i<9;i++) addShroom({fx[i],0,fz[i]}, fh[i], fr[i], (i%3)==1);
        addShroomPatch(-56, 0,  -2, 5, 3.4f);      // undergrowth at the giants' feet
        addShroomPatch( 72, 0,  46, 6, 4.0f);
    }
    // drifting cloud banks at three heights
    for (int k=0;k<6;k++){
        float a = k*1.047f + 0.5f;
        float d = 46 + (k%3)*22;
        Vector3 c = {CX+sinf(a)*d, 34.0f + (k%3)*36.0f, CZ+cosf(a)*d};
        addDecor(D_SPHERE, c, {7.5f,2.0f,7.5f}, C_CLOUD);
        addDecor(D_SPHERE, {c.x-4.2f,c.y-0.4f,c.z+1.2f}, {4.6f,1.6f,4.6f}, C_CLOUD);
        addDecor(D_SPHERE, {c.x+4.4f,c.y-0.3f,c.z-1.0f}, {5.0f,1.7f,5.0f}, C_CLOUD);
    }
    // ferns and mossy stones around the roots
    for (int k=0;k<10;k++){
        float a = k*0.628f + 0.3f;
        float d = 9.5f + (k%3)*3.0f;
        float px = CX+sinf(a)*d, pz = CZ+cosf(a)*d;
        for (int c2=0;c2<3;c2++)
            addDecor(D_CONE, {px, 0.1f+c2*0.45f, pz}, {1.0f-c2*0.26f, 0.75f, 1.0f-c2*0.26f},
                     (Color){64,155,78,255});
    }
    for (int k=0;k<7;k++){
        float a = k*0.9f + 1.1f;
        float d = 13.0f + (k%2)*5.0f;
        addDecor(D_SPHERE, {CX+sinf(a)*d, 0.25f, CZ+cosf(a)*d},
                 {frnd(0.8f,1.6f), frnd(0.5f,0.9f), frnd(0.8f,1.6f)}, (Color){168,166,158,255}, TX_STONE);
    }

    // shelf caps sprouting from the stalk (angle°, topY, radius, red?, dist).
    // The climb is a STRICT one-way spiral - every step turns exactly 68°, so
    // a straight vault from one shelf to the next always clears the central
    // stalk (you never have to curve around it in the air). Difficulty comes
    // from small pads, tall/varied rises, a PERFECT gate, the web crossing,
    // and the long fall a miss earns - not from impossible geometry.
    // radii breathe now: broad resting shelves, tight nervy pads, fat reds -
    // the climb reads as a rhythm of big and small instead of a uniform stair
    struct ShelfDef { float ang, top, r; int red; float dist; };
    static const ShelfDef S[] = {
        // ---- Part A: the ground up to the Weaver's shelf (13 jumps, 68° each) ----
        {356,   5.0f, 3.4f, 0, 0},                              // 1  up from the ground - generous
        { 64,   9.4f, 2.6f, 0, 0},                              // 2  +4.4
        {132,  15.8f, 2.1f, 0, 0},                              // 3  +6.4 F - tight
        {190,  12.5f, 3.3f, 1, 0},                              //    red - fat parachute
        {200,  18.4f, 2.9f, 0, 0},                              // 4  +2.6 S - breather
        {268,  24.8f, 2.0f, 0, 0},                              // 5  +6.4 F - tight
        {336,  29.1f, 2.5f, 0, 0},                              // 6  +4.3 M
        { 20,  26.0f, 2.7f, 1, 0},                              //    red
        { 44,  35.5f, 2.1f, 0, 0},                              // 7  +6.4 F
        {112,  38.1f, 3.0f, 0, 0},                              // 8  +2.6 S - broad rest
        {180,  44.5f, 1.9f, 0, 0},                              // 9  +6.4 F - the pinch
        {210,  41.0f, 3.2f, 1, 0},                              //    red
        {248,  48.8f, 2.4f, 0, 0},                              // 10 +4.3 M
        {316,  55.2f, 2.0f, 0, 0},                              // 11 +6.4 F
        { 60,  51.0f, 2.6f, 1, 0},                              //    red
        { 24,  57.8f, 2.8f, 0, 0},                              // 12 +2.6 S
        { 92,  62.1f, 2.2f, 0, 0},                              // 13 +4.3 M
        {160,  68.6f, 3.2f, 0, 0},                              // 14 THE WEAVER'S SHELF - a big landmark
        // ---- the web crossing (shrine unlocks the swing; two blooms are the
        //      only road on - swinging can steer where a vault cannot) ----
        {160,  77.1f, 2.4f, 0, 17.0f},                          // web gap I - a pad flung far off the stalk
        {240,  86.0f, 2.3f, 0, 0},                              // web gap II - swing home, higher
        // ---- Part B: above the web up to the crown (same 68° spiral) ----
        {308,  92.4f, 2.2f, 0, 0},                              // 16 +6.4 F
        {340,  89.0f, 3.0f, 1, 0},                              //    red by the web exit
        { 16,  95.0f, 1.9f, 0, 0},                              // 17 +2.6 S - button
        { 84, 101.4f, 2.5f, 0, 0},                              // 18 +6.4 F
        {152, 105.7f, 1.9f, 0, 0},                              // 19 +4.3 M
        {120,  98.0f, 2.7f, 1, 0},                              //    red
        {220, 112.1f, 2.3f, 0, 0},                              // 20 +6.4 F
        {288, 120.7f, 2.6f, 0, 0},                              // 21 THE PERFECT GATE (+8.6, sprint+PERFECT)
        {250, 108.0f, 3.1f, 1, 0},                              //    red to catch a botched PERFECT
        {356, 123.3f, 1.9f, 0, 0},                              // 22 +2.6 S
        { 64, 129.7f, 2.4f, 0, 0},                              // 23 +6.4 F
        { 30, 119.0f, 2.6f, 1, 0},                              //    red
        {132, 132.3f, 1.9f, 0, 0},                              // 24 +2.6 S - the last pinch
        {200, 138.7f, 2.1f, 0, 0},                              // 25 +6.4 F
        {170, 129.0f, 3.3f, 1, 0},                              //    the last red - fat, the crown run is dry
        {268, 141.5f, 2.4f, 0, 6.0f},                           // 26 +2.8 - swings OUT past the crown rim
        {320, 144.5f, 2.4f, 0, 6.6f},                           // 27 outrigger just under the rim: hop up & in
    };
    for (auto& sd : S){
        float a = (sd.ang + 180.0f)*DEG2RAD;                    // route starts facing the spawn
        float d = (sd.dist > 0)? sd.dist : 3.0f + sd.r*0.72f;
        float x = CX + sinf(a)*d, z = CZ + cosf(a)*d;
        addCap(x, z, sd.top, sd.r, sd.red != 0);
        if (!sd.red) addCoin(x, sd.top + 1.3f, z);
    }

    // THE CROWN: the summit cap. Collision is only a THIN disc at the very top
    // (you land ON it) - no thick overhang for the final jumps to bonk on.
    {
        float ctop = 148.0f, cr = 6.7f;
        addCyl({CX, ctop - 0.8f, CZ}, cr*0.88f, 0.85f, S_SHROOM_TAN, false, false);   // thin walk-top
        Color cc = C_TAN;
        addDecor(D_SPHERE, {CX, ctop - 0.85f, CZ}, {cr*1.14f, 1.25f, cr*1.14f}, ctint(cc,0.90f), TX_STREAK); // rim lip
        addDecor(D_SPHERE, {CX, ctop + 0.10f, CZ}, {cr*0.90f, 1.85f, cr*0.90f}, cc, TX_STREAK);              // dome
        addDecor(D_CYL, {CX, ctop - 1.6f, CZ}, {cr*0.80f, 0.24f, cr*0.80f}, {250,242,222,255});              // pale underside
        for (int i=0;i<8;i++){
            float a = i*0.785f, rr = frnd(cr*0.26f, cr*0.56f);
            float dy = ctop + 0.55f + 1.85f*sqrtf(fmaxf(0.0f, 1.0f-(rr*rr)/(cr*0.9f*cr*0.9f))) - 0.5f;
            addDecor(D_SPHERE, {CX+sinf(a)*rr, dy, CZ+cosf(a)*rr}, {0.95f,0.4f,0.95f}, C_CREAM);             // spots
        }
    }
    addCoin(CX, 149.4f, CZ);

    // base parachutes: four reds ringing the stalk catch long falls -
    // different heights AND sizes, a proper mushroom family around the roots
    {
        float pr[4] = {4.8f, 3.2f, 5.6f, 2.8f};
        float ph[4] = {5.0f, 7.0f, 4.2f, 6.2f};
        for (int k=0;k<4;k++){
            float a = (45 + k*90)*DEG2RAD;
            addShroom({CX+sinf(a)*12.0f, 0, CZ+cosf(a)*12.0f}, ph[k], pr[k], true);
        }
        addShroomPatch(CX+9, 0, CZ-10, 6, 3.0f);   // buttons among the giants
    }

    // THE WEAVER'S BLOOM: it wakes on the shelf right below the web gaps -
    // touch it, and the two blooms ahead are the only road onward
    addShrine({-1.67f, 68.6f, 44.58f}, 0);
    addWebAnchor({-3.76f, 80.0f, 50.34f});             // bloom over web gap I
    addWebAnchor({-0.90f, 89.0f, 49.20f});             // bloom over web gap II
    // the flung pad hangs in the void on dangling roots
    for (int k=0;k<4;k++){
        float a = k*1.7f;
        addDecor(D_CYL, {-5.81f+sinf(a)*1.2f, 71.6f-k*0.6f, 55.98f+cosf(a)*1.2f},
                 {0.07f, 2.2f+k*0.6f, 0.07f}, {112,78,46,255});
    }

    // meadow dressing: dirt ring at the base, flowers, far flavor shrooms
    for (int i=0;i<10;i++){
        float a = i*0.628f;
        addDecor(D_CYL, {CX+sinf(a)*8.2f, 0.03f, CZ+cosf(a)*8.2f},
                 {frnd(1.2f,1.6f), 0.025f, frnd(1.0f,1.3f)},
                 (i%2)? (Color){203,172,112,255} : (Color){193,161,103,255});
    }
    for (int i=0;i<50;i++){
        float x = frnd(-80,80), z = frnd(-40,120);
        float dx = x-CX, dz = z-CZ;
        if (dx*dx+dz*dz < 15*15) continue;
        if (gMega && dx*dx+dz*dz > 34*34) continue;      // stay on the crag plateau
        Color heads[4] = {{255,90,90,255},{255,200,60,255},{255,255,255,255},{255,120,200,255}};
        addDecor(D_CYL,   {x,0,z},     {0.06f,0.55f,0.06f}, {60,150,50,255});
        addDecor(D_SPHERE,{x,0.62f,z}, {0.2f,0.2f,0.2f},  heads[GetRandomValue(0,3)]);
    }
    if (!gMega){
        addShroom({-45,0, 20}, 10.0f, 6.0f, false);
        addShroom({ 48,0, 70},  4.5f, 2.4f, false);
        addShroom({-38,0, 90},  8.0f, 5.2f, true);
        for (int i=0;i<9;i++){
            float ang = -0.6f + i*0.35f;
            float d = frnd(300,380);
            addDecor(D_SPHERE, {sinf(ang)*d, -frnd(60,130)*0.55f, 100+cosf(ang)*d},
                     {frnd(60,130), frnd(48,104), frnd(60,130)}, {70,172,88,255});
        }
    }
}

// ---------------------------------------------- level 2: THE SPOREWAY -----
// Floating islands strung across the sky. The blooms ARE the bridges: vault
// off an edge, grab mid-air, ride the pendulum, release on the upswing.
// Golden spores turbo-charge the pole for a few seconds - the double-height
// hops they unlock must be chained before the glow fades.
static void BuildSporeway(void){
    gSpawn = { -98, 0.91f, 40 };
    gStarP = { 2, 78.6f, 39 };

    if (!gMega) addBox({-220,-2,-80},{220,0,280}, S_GRASS);   // ascent: open sky below
    addWarpPipe({-106,0,40}, {255,150,60,255});    // onward to THE GORGE

    // floating island: grass disc + dirt belly + lip
    auto island = [&](float x, float z, float top, float r){ addIsle(x, z, top, r); };

    // ---- section A: the BOUNCE run. The islands sit far apart - you must
    //      vault off one, DROP onto the red trampolines floating in the gap,
    //      and ride the boings across to the next. No standing on the reds.
    island(-90, 40,  3.0f, 5.0f);                  // mound off the meadow
    island(-77, 40,  6.0f, 4.5f);                  // I0
    island(-55, 40, 10.0f, 4.0f);                  // I1
    island(-32, 44, 15.0f, 3.6f);                  // I2
    addCap(-70, 40,  4.0f, 4.8f, true);            // gap I0->I1: fat teaching trampoline
    addCap(-62, 41,  4.0f, 3.6f, true);            //   ...then a tighter one
    addCap(-47, 42,  5.0f, 4.2f, true);            // gap I1->I2: two more, tightening
    addCap(-40, 43,  5.0f, 3.0f, true);            //   the small exit cap
    addCoin(-66, 8.0f, 40); addCoin(-44, 9.0f, 42);   // prizes on the bounce line
    // ---- section B: NOW the blooms - two swings, one flight ----------------
    addWebAnchor({-21, 26.0f, 45});
    addWebAnchor({ -8, 30.0f, 43});                //   let go, fall, grab again
    island(  4, 42, 22.0f, 3.6f);                  // I3
    // ---- section C: spore chain I -----------------------------------------
    addSpore(4, 23.2f, 42);
    island( 10, 47, 32.5f, 3.2f);                  // +10.5: boosted only
    island(  3, 52, 43.0f, 3.2f);                  // +10.5 again - beat the timer
    // ---- section D: high swing --------------------------------------------
    addWebAnchor({-9.5f, 54.5f, 52});
    island(-24, 52, 50.0f, 3.4f);                  // I6
    // ---- section E: spore chain II (small pads, hard turn) ------------------
    addSpore(-24, 51.2f, 52);
    island(-31, 46, 60.5f, 2.6f);
    island(-25, 39, 71.0f, 2.6f);                  // I8
    // ---- finale: the long swing to the star --------------------------------
    addWebAnchor({-12, 83.0f, 39});
    island(  2, 39, 76.0f, 4.2f);                  // I9 - the star island
    addCoin(-14.5f, 26.0f, 44); addCoin(-12, 72.0f, 39);

    // a couple more red trampolines under the later web lines (bounce bails)
    if (!gMega){
        addShroom({  7,0,47}, 6.0f, 4.6f, true);   // the big net
        addShroom({-14,0,44}, 7.0f, 3.0f, true);   // the small net
    } else {
        // no ground out here - the nets FLOAT, dangling drift-roots
        addCap(  7, 47, 16.0f, 4.6f, true);
        addCap(-14, 44, 13.0f, 3.0f, true);
        for (int k=0;k<3;k++){
            addDecor(D_CYL, {7+sinf(k*2.1f)*1.5f, 16.0f-4.4f-k*0.8f, 47+cosf(k*2.1f)*1.5f},
                     {0.08f, 2.6f+k*0.8f, 0.08f}, {112,78,46,255});
            addDecor(D_CYL, {-14+sinf(k*2.1f)*1.0f, 13.0f-3.6f-k*0.7f, 44+cosf(k*2.1f)*1.0f},
                     {0.07f, 2.2f+k*0.7f, 0.07f}, {112,78,46,255});
        }
    }

    // island life: waterfalls spilling into the void, roots, tufted flowers
    {
        struct IslePos { float x, z, top, r; };
        static const IslePos isles[] = {
            {-90,40,3,5},{-77,40,6,4.5f},{-55,40,10,4},{-32,44,15,3.6f},{4,42,22,3.6f},
            {10,47,32.5f,3.2f},{3,52,43,3.2f},{-24,52,50,3.4f},{-31,46,60.5f,2.6f},
            {-25,39,71,2.6f},{2,39,76,4.2f} };
        for (int i=0;i<11;i++){
            const IslePos& I = isles[i];
            // hanging roots under the belly
            for (int k=0;k<3;k++){
                float a = i*1.3f + k*2.1f;
                addDecor(D_CYL, {I.x+sinf(a)*I.r*0.4f, I.top-5.4f-k*0.7f, I.z+cosf(a)*I.r*0.4f},
                         {0.07f, 2.4f+k*0.8f, 0.07f}, {112,78,46,255});
            }
            // flower tufts on the lip
            for (int k=0;k<3;k++){
                float a = i*0.8f + k*2.09f;
                Color heads[3] = {{255,120,200,255},{255,255,255,255},{255,200,60,255}};
                addDecor(D_CYL, {I.x+sinf(a)*I.r*0.6f, I.top, I.z+cosf(a)*I.r*0.6f},
                         {0.05f,0.5f,0.05f}, {60,150,50,255});
                addDecor(D_SPHERE, {I.x+sinf(a)*I.r*0.6f, I.top+0.58f, I.z+cosf(a)*I.r*0.6f},
                         {0.17f,0.17f,0.17f}, heads[(i+k)%3]);
            }
        }
        // three islands weep thin waterfalls into the sky
        float wf[3][4] = {{-32,44,15,3.6f},{3,52,43,3.2f},{2,39,76,4.2f}};
        for (int i=0;i<3;i++){
            addDecor(D_CUBE, {wf[i][0]+wf[i][3]*0.55f, wf[i][2]-9.0f, wf[i][1]},
                     {0.35f, 13.0f, 1.1f}, (Color){160,210,250,210});
            addDecor(D_SPHERE, {wf[i][0]+wf[i][3]*0.55f, wf[i][2]-15.6f, wf[i][1]},
                     {0.9f,0.5f,0.9f}, (Color){230,244,252,120});
        }
        // drifting petals
        addMote({-66, 14, 40}, {255,170,215,255}, 3.4f, 0.5f);
        addMote({-21, 30, 45}, {255,205,230,255}, 3.8f, 0.45f);
        addMote({  6, 38, 47}, {255,170,215,255}, 3.2f, 0.6f);
        addMote({-24, 56, 50}, {255,205,230,255}, 3.6f, 0.5f);
        addMote({ -8, 80, 39}, {255,170,215,255}, 3.4f, 0.55f);
    }
    // a distant archipelago drifts beyond the route (pure scenery)
    {
        float bx[10] = {-140, -110,  -60,   20,  60,  90, -150,  -30,  70, -90};
        float bz[10] = { -10,  110,  130,  110, 100,  30,   60,  -40, -20, -40};
        float by[10] = {  26,   48,   70,   58,  34,  62,   80,   44,  76,  18};
        float br[10] = { 9.0f, 6.5f, 5.0f, 7.5f, 8.0f, 4.5f, 6.0f, 5.5f, 4.0f, 10.0f};
        for (int i=0;i<10;i++){
            addDecor(D_CYL, {bx[i], by[i]-1.8f, bz[i]}, {br[i], 1.8f, br[i]}, C_GRASS_A, TX_GRASS);
            addDecor(D_SPHERE, {bx[i], by[i]-2.6f, bz[i]}, {br[i]*0.92f, 3.2f, br[i]*0.92f}, (Color){118,84,50,255});
            if (i%3==0){                              // some weep their own falls
                addDecor(D_CUBE, {bx[i]+br[i]*0.4f, by[i]-9.0f, bz[i]}, {0.4f, 12.0f, 1.0f}, (Color){160,210,250,180});
            }
        }
        // colossal seed-balloons riding the wind
        for (int i=0;i<4;i++){
            float sx[4] = {-45, 25, -80, 45}, sz[4] = {75, 20, 20, 65}, sy[4] = {55, 40, 66, 80};
            addDecor(D_SPHERE, {sx[i], sy[i], sz[i]}, {4.2f, 5.0f, 4.2f}, (Color){245,240,225,120});
            addDecor(D_SPHERE, {sx[i], sy[i]-4.6f, sz[i]}, {0.7f, 0.7f, 0.7f}, (Color){170,130,80,255});
            addDecor(D_CYL, {sx[i], sy[i]-4.4f, sz[i]}, {0.05f, 4.4f, 0.05f}, (Color){200,190,170,200});
        }
    }
    // meadow dressing (standalone only - the ascent has no ground here)
    if (!gMega){
        for (int i=0;i<46;i++){
            float x = frnd(-110,40), z = frnd(0,90);
            Color heads[4] = {{255,90,90,255},{255,200,60,255},{255,255,255,255},{255,120,200,255}};
            addDecor(D_CYL,   {x,0,z},     {0.06f,0.55f,0.06f}, {60,150,50,255});
            addDecor(D_SPHERE,{x,0.62f,z}, {0.2f,0.2f,0.2f},  heads[GetRandomValue(0,3)]);
        }
        addShroom({-70,0, 75}, 11.0f, 6.2f, false);    // meadow giant
        addShroom({ 25,0, 70},  5.5f, 3.0f, false);
        addShroom({ 30,0, 12}, 13.0f, 7.2f, false);    // the far colossus
        addShroom({ 36,0, 18},  2.8f, 1.6f, true);     // its red button friend
        addShroomPatch(-80, 0, 52, 6, 3.4f);
        addShroomPatch( 20, 0, 64, 5, 2.8f);
        for (int i=0;i<9;i++){
            float ang = -0.6f + i*0.35f;
            float d = frnd(300,380);
            addDecor(D_SPHERE, {sinf(ang)*d, -frnd(60,130)*0.55f, 100+cosf(ang)*d},
                     {frnd(60,130), frnd(48,104), frnd(60,130)}, {70,172,88,255});
        }
    }
}

// -------------------------------------------------- level 3: THE GORGE ----
// A long quarried canyon built around THE SLAM. Giant red caps rise from the
// floor as bounce shafts; exit ledges wait at precise height windows under
// stone arches that bite overshoots. Slam late to climb, pick your tier to
// hit the window, and don't kiss the rock. Longest level; hardest level.
static void BuildGorge(void){
    gSpawn = { 0, 0.91f, -30 };
    gStarP = { 0, 77.6f, 236 };

    if (!gMega) addBox({-220,-2,-80},{220,0,280}, S_GRASS);   // ascent: a floating mesa
    addWarpPipe({-8,0,-34}, {80,220,90,255});      // home to the castle
    addBox({-15,0,-12},{15,0.08f,238}, S_SAND);    // sandy canyon floor

    // THE THUNDER SHRINE: touch it and the SLAM is yours, forever...
    addShrine({0,0,-16}, 1);
    // ...but the BOING runs on THUNDERSEEDS. Every diving board grows its own
    // crop - enough for that shaft, none to smuggle out of the canyon.
    addThunder(0, 1.2f, -10, 3);

    // ---- canyon walls: varied skyline + sandstone strata --------------------
    {
        float ze[10] = {-12,16,44,72,98,124,152,180,208,238};
        float hw[9]  = {30,42,36,50,46,58,64,66,58};    // west
        float he[9]  = {34,28,46,38,52,50,60,66,64};    // east
        Color strata[3] = {{196,120,70,255},{214,160,84,255},{228,198,150,255}};
        for (int i=0;i<9;i++){
            addBox({-30,0,ze[i]},{-15,hw[i],ze[i+1]}, S_STONE);
            addBox({ 15,0,ze[i]},{ 30,he[i],ze[i+1]}, S_STONE);
            float zc = (ze[i]+ze[i+1])*0.5f, zl = ze[i+1]-ze[i]-0.6f;
            for (int b=0;b<3;b++){
                float f = 0.28f + b*0.22f;
                addDecor(D_CUBE, {-14.86f, hw[i]*f, zc}, {0.22f, 0.9f, zl}, strata[b]);
                addDecor(D_CUBE, { 14.86f, he[i]*f, zc}, {0.22f, 0.9f, zl}, strata[b]);
            }
        }
    }
    // helpers ------------------------------------------------------------------
    auto ledge = [&](bool east, float top, float z0, float z1){
        GrpScope _g;
        float x0 = east? 12.0f : -15.0f, x1 = east? 15.0f : -12.0f;
        addBox({x0, top-0.7f, z0},{x1, top, z1}, S_STONE);
        addTorch({east? 13.2f : -13.2f, top, z0+0.8f});
        addCoin(east? 13.5f : -13.5f, top+1.3f, (z0+z1)*0.5f);
    };
    auto board = [&](bool east, float top, float z0, float z1){   // diving pier
        addBox({east? 5.0f : -15.0f, top-0.4f, z0},{east? 15.0f : -5.0f, top, z1}, S_WOOD);
    };
    auto roof = [&](bool openEast, float y, float z0, float z1){  // the arch that bites
        GrpScope _g;
        addBox({openEast? -15.0f : -9.0f, y, z0},{openEast? 9.0f : 15.0f, y+1.5f, z1}, S_STONE);
        addDecor(D_CUBE, {openEast? -3.0f : 3.0f, y+1.9f, (z0+z1)*0.5f},
                 {(z1-z0)*0.5f, 0.7f, (z1-z0)*0.5f}, C_STONE_B, TX_STONE);
    };

    // ---- mouth gate ----------------------------------------------------------
    addBox({-15,0,-14},{-10,26,-10}, S_STONE);
    addBox({ 10,0,-14},{ 15,26,-10}, S_STONE);
    addBox({-12,24,-14},{12,27,-10}, S_STONE);
    addFlag({3,0,-6}, C_RED);

    // ---- act 1: the lesson (P1 practice cap -> L1) ---------------------------
    addShroom({0,0,-2}, 4.5f, 4.2f, true);         // fat and forgiving - it's the lesson
    addShroomPatch(-8, 0, -8, 5, 2.6f);            // buttons at the canyon mouth
    ledge(false, 10, 2, 24);                        // L1 west + long walk shelf
    // ---- act 2: THE STEPS (four shafts, windows tighten) ---------------------
    board(false, 10, 24, 28);
    addThunder(-10, 11.3f, 26, 3);
    addShroom({0,0,28}, 5.0f, 3.3f, true);          // C1: drop 5, exit +13, open sky
    ledge(true, 18, 30, 44);                        // E1
    addCoin(0, 19.5f, 28);
    board(true, 18, 44, 48);
    addThunder(10, 19.3f, 46, 3);
    addShroom({0,0,48}, 6.0f, 2.8f, true);          // C2: drop 12, window 18..24 (GOOD)
    roof(false, 30, 45, 51);
    ledge(false, 24, 52, 64);                       // E2
    addCoin(0, 26.5f, 48);
    board(false, 24, 64, 68);
    addThunder(-10, 25.3f, 66, 2);
    addShroom({0,0,68}, 7.0f, 2.4f, true);          // C3: drop 17, window 26..31 (PERFECT, small)
    roof(true, 38, 65, 71);
    ledge(true, 33, 70, 82);                        // E3
    addCoin(0, 35.5f, 68);
    board(true, 33, 86, 90);
    addThunder(10, 34.3f, 88, 2);
    addShroom({0,0,90}, 20.0f, 2.9f, true);         // C4 giant: drop 13, window 23..27 (mixed)
    roof(false, 47, 87, 93);
    ledge(false, 43, 92, 100);                      // E4 = the crossing porch
    addCoin(0, 45.0f, 90);
    // ---- act 3: THE CROSSING (two blooms over the river, no nets) ------------
    addWebAnchor({-4, 53, 106});
    addWebAnchor({ 2, 55, 122});
    addCoin(-4, 45, 106); addCoin(2, 47, 122);
    ledge(true, 40, 126, 134);                      // E5
    // ---- act 4: THE ORGAN PIPES (three shafts, alternating gates) ------------
    board(true, 40, 138, 142);
    addThunder(10, 41.3f, 140, 2);
    addShroom({0,0,142}, 26.0f, 2.9f, true);        // O1: drop 14, window 20..26 (PERFECT-ish)
    roof(true, 52, 139, 145);
    ledge(true, 46, 146, 154);                      // E6
    addCoin(0, 49.0f, 142);
    board(true, 46, 156, 160);
    addThunder(10, 47.3f, 158, 2);
    addShroom({0,0,160}, 30.0f, 2.4f, true);        // O2: drop 16, window 28..33 (PERFECT)
    roof(false, 63, 157, 163);
    ledge(false, 58, 162, 170);                     // E7
    addCoin(0, 60.5f, 160);
    board(false, 58, 172, 176);
    addThunder(-10, 59.3f, 174, 2);
    addShroom({0,0,176}, 40.0f, 2.1f, true);        // O3: drop 18, window 22..26 (GOOD only!) - the pin
    roof(true, 66, 173, 179);
    ledge(true, 62, 180, 188);                      // E8
    addCoin(0, 64.0f, 176);
    // ---- act 5: THE THROAT ----------------------------------------------------
    ledge(true, 54, 188, 194);                      // stairs down to the plank
    ledge(true, 48, 194, 200);
    board(true, 42, 202, 206);
    addFlag({13.0f, 42, 203}, C_GOLD);
    addThunder(9, 43.3f, 204, 2);
    addShroom({0,0,206}, 30.0f, 3.0f, true);        // the Throat cap: drop 12
    roof(false, 55, 203, 209);                      // true window 21..25
    ledge(true, 38, 202, 206);                      // fake ledge (coins, no road)
    ledge(true, 44, 208, 212);                      // fake ledge II
    ledge(false, 51, 208, 216);                     // TRUE exit - the rim
    addCoin(0, 53.0f, 206);
    // ---- crown: spore pinnacles, then vault-into-slam through the star -------
    addSpore(-13.5f, 52.2f, 212);
    addBox({-8,0,216},{-4,61,220}, S_STONE);        // pinnacle A (+10 boosted)
    addBox({-4,0,222},{ 4,64,230}, S_STONE);        // plateau (+3 hop)
    addThunder(0, 65.3f, 226, 2);                   // the crown boing
    addCoin(-6, 63.0f, 218);
    addCyl({0,0,236}, 2.4f, 64.2f, S_STONE);        // the star column
    addCap(0, 236, 66.0f, 3.0f, true);              // vault over it, SLAM it, fly through the star
    addCoin(0, 72.5f, 236);

    // ---- scenery: the gorge lives ---------------------------------------------
    auto pine = [&](float x, float y, float z, float s){
        GrpScope _g;
        addDecor(D_CYL, {x,y,z}, {0.22f*s, 1.1f*s, 0.22f*s}, {110,74,40,255}, TX_FIBER);
        for (int k=0;k<3;k++)
            addDecor(D_CONE, {x, y+(0.8f+k*0.72f)*s, z},
                     {(1.5f-k*0.38f)*s, 1.15f*s, (1.5f-k*0.38f)*s},
                     ctint((Color){52,140,70,255}, 1.0f+k*0.09f));
    };
    auto crystal = [&](float x, float y, float z){
        GrpScope _g;
        for (int k=0;k<3;k++)
            addDecor(D_CONE, {x+frnd(-0.5f,0.5f), y, z+frnd(-0.5f,0.5f)},
                     {frnd(0.24f,0.4f), frnd(0.9f,1.9f), frnd(0.24f,0.4f)}, {120,225,255,255});
        addDecor(D_SPHERE, {x,y+0.5f,z}, {1.1f,0.7f,1.1f}, (Color){150,235,255,70});
    };
    auto vine = [&](float x, float ytop, float z, float len){
        GrpScope _g;
        addDecor(D_CYL, {x, ytop-len, z}, {0.06f, len, 0.06f}, {70,150,60,255});
        for (int k=0;k<4;k++)
            addDecor(D_SPHERE, {x+frnd(-0.2f,0.2f), ytop-len*frnd(0.15f,0.95f), z+frnd(-0.2f,0.2f)},
                     {0.30f,0.20f,0.30f}, {84,170,72,255});
    };
    // the river + waterfall + a little stone footbridge
    for (int i=0;i<9;i++){
        float z = 90 + i*5.8f;
        float x = sinf(i*0.9f)*2.6f;
        addDecor(D_CUBE, {x, 0.12f, z}, {8.5f+sinf(i*1.7f)*1.5f, 0.07f, 6.2f}, {64,140,228,255});
        addDecor(D_SPHERE, {x+frnd(-3,3), 0.17f, z+frnd(-2,2)}, {0.3f,0.08f,0.3f}, {225,240,252,255});
    }
    addDecor(D_CUBE, {-14.55f, 20, 132}, {0.5f, 40, 2.4f}, {150,205,250,235});
    addDecor(D_CUBE, {-14.35f, 24, 132}, {0.3f, 32, 1.2f}, {225,242,252,255});
    for (int i=0;i<5;i++)
        addDecor(D_SPHERE, {-13.6f+frnd(-0.5f,0.8f), 0.6f+frnd(0,0.8f), 132+frnd(-1.5f,1.5f)},
                 {frnd(0.7f,1.3f), frnd(0.4f,0.7f), frnd(0.7f,1.3f)}, (Color){235,245,252,150});
    addBox({-3.5f,0,117},{-2.2f,3,119}, S_STONE);
    addBox({ 2.2f,0,117},{ 3.5f,3,119}, S_STONE);
    addBox({-4.5f,3,116.5f},{4.5f,3.8f,119.5f}, S_STONE);
    // hoodoos outside the mouth
    for (int i=0;i<3;i++){
        float hx[3] = {-24, 26, 21}, hz[3] = {-22, -16, -34}, hh[3] = {14, 18, 10};
        addCyl({hx[i],0,hz[i]}, 2.2f, hh[i], S_STONE);
        addBox({hx[i]-3.2f, hh[i], hz[i]-3.2f},{hx[i]+3.2f, hh[i]+1.6f, hz[i]+3.2f}, S_STONE);
    }
    // pines on the rims and in the mouth meadow
    pine(-21, 30, 4, 2.2f);  pine(-24, 42, 30, 2.6f); pine(-20, 46, 110, 2.4f);
    pine(-23, 64, 166, 2.8f); pine(21, 34, 2, 2.4f);  pine(24, 38, 60, 2.2f);
    pine(20, 50, 136, 2.6f);  pine(23, 66, 195, 2.8f);
    pine(-24, 0, -26, 3.0f);  pine(28, 0, -24, 2.6f); pine(-19, 0, -38, 2.2f);
    pine(33, 0, -38, 3.2f);   pine(-31, 0, -14, 2.4f);
    // crystals glinting in wall nooks, vines draping the strata
    crystal(-13.6f, 0.1f, 36); crystal(13.6f, 0.1f, 78); crystal(-13.6f, 0.1f, 148);
    crystal(13.6f, 0.1f, 190); crystal(-13.6f, 0.1f, 226);
    vine(-14.6f, 42, 22, 14);  vine(14.6f, 28, 36, 11);  vine(-14.6f, 50, 58, 17);
    vine(14.6f, 38, 66, 12);   vine(-14.6f, 46, 104, 15); vine(14.6f, 52, 118, 16);
    vine(-14.6f, 58, 158, 18); vine(14.6f, 60, 172, 15);  vine(-14.6f, 58, 220, 13);
    vine(14.6f, 64, 200, 17);
    // distant mesas beyond the rims; fireflies keep the crystals company.
    // In the ascent they FLOAT alongside the great mesa - give them undersides.
    addBox({-120,0,40},{-70,34,110}, S_SAND);
    addBox({70,0,60},{130,42,140}, S_SAND);
    addBox({-110,0,160},{-60,28,220}, S_SAND);
    if (gMega){
        float ux[3] = {-95, 100, -85}, uz[3] = {75, 100, 190}, uw[3] = {32, 38, 32};
        for (int i=0;i<3;i++)
            for (int k=0;k<3;k++)     // eroded rock belly: shrinking strata lobes
                addDecor(D_SPHERE, {ux[i], -6.0f - k*11.0f, uz[i]},
                         {uw[i]*(1.0f-k*0.28f), 9.5f, (uw[i]+8)*(1.0f-k*0.28f)},
                         ctint((Color){178,128,88,255}, 1.0f - k*0.10f), TX_SAND);
    }
    addMote({-13, 2.2f, 36},  {255,220,110,255}, 1.8f, 0.8f);
    addMote({ 13, 2.6f, 78},  {255,220,110,255}, 1.6f, 0.7f);
    addMote({-13, 2.4f, 148}, {255,220,110,255}, 1.9f, 0.9f);
    addMote({ 13, 2.2f, 190}, {255,220,110,255}, 1.7f, 0.75f);
    addMote({-13, 3.0f, 226}, {255,220,110,255}, 1.8f, 0.85f);
    addMote({ 0, 4.0f, 100},  {200,240,255,255}, 2.6f, 0.5f);
    addMote({ 0, 55, 60},  {40,38,44,255}, 7.0f, 0.35f);       // circling birds, far up
    addMote({ 4, 62, 150}, {40,38,44,255}, 8.0f, 0.3f);
    // dark side-canyon mouths yawning in the walls
    addDecor(D_CUBE, {-14.8f, 6.5f, 52},  {0.35f, 13.0f, 7.0f}, {38,32,28,255});
    addDecor(D_CUBE, { 14.8f, 8.0f, 108}, {0.35f, 16.0f, 9.0f}, {38,32,28,255});
    addDecor(D_CUBE, {-14.8f, 7.0f, 186}, {0.35f, 14.0f, 6.0f}, {38,32,28,255});
    // broken pillar ruins along the rims
    {
        float rx[6] = {-20, 22, -24, 19, -21, 24};
        float rz[6] = { 24, 40, 88, 150, 200, 222};
        float ry[6] = { 42, 28, 50, 60, 66, 64};
        for (int i=0;i<6;i++){
            addDecor(D_CYL, {rx[i], ry[i], rz[i]}, {1.1f, 2.0f+ (i%3)*1.4f, 1.1f}, C_STONE_B, TX_STONE);
            if (i%2==0) addDecor(D_CUBE, {rx[i], ry[i]+2.4f+(i%3)*1.4f, rz[i]}, {2.8f, 0.5f, 2.8f}, C_STONE_B, TX_STONE);
        }
    }
    // desert agaves at the wall feet
    for (int i=0;i<7;i++){
        float ax = (i%2)? 12.0f : -12.0f, az = 10 + i*32.0f;
        for (int c2=0;c2<4;c2++){
            float aa = c2*1.57f + i;
            addDecor(D_CONE, {ax+sinf(aa)*0.5f, 0.05f, az+cosf(aa)*0.5f},
                     {0.4f, 1.5f+0.3f*(c2%2), 0.4f}, (Color){110,168,120,255});
        }
    }
    if (!gMega)
        for (int i=0;i<9;i++){
            float ang = -0.6f + i*0.35f;
            float d = frnd(300,380);
            addDecor(D_SPHERE, {sinf(ang)*d, -frnd(60,130)*0.55f, 100+cosf(ang)*d},
                     {frnd(60,130), frnd(48,104), frnd(60,130)}, {70,172,88,255});
        }
    // meadow flowers by the mouth
    for (int i=0;i<26;i++){
        float x = frnd(gMega? -31.0f : -36.0f, gMega? 31.0f : 36.0f), z = frnd(-40,-14);
        Color heads[4] = {{255,90,90,255},{255,200,60,255},{255,255,255,255},{255,120,200,255}};
        addDecor(D_CYL,   {x,0,z},     {0.06f,0.55f,0.06f}, {60,150,50,255});
        addDecor(D_SPHERE,{x,0.62f,z}, {0.2f,0.2f,0.2f},  heads[GetRandomValue(0,3)]);
    }
}

// ------------------------------------------- level 4: SKYHAVEN (the wind) --
// An open sky of colossal windmills. You VAULT off a terrace, hold SHIFT to
// unfurl the SKYSAIL, and ride the invisible weather: catch an UPDRAFT to soar,
// tuck (W) to dive for speed, flare (S) to brake, steer (A/D), and read the
// steady wind that pushes every glide. Miss your line and the wind carries you
// into the blue. The star crowns the great mill.
static void BuildSkyhaven(void){
    gSpawn = { -64, 2.91f, 0 };
    gStarP = { 52, 150.6f, 0 };               // the star now crowns a 150 m tower
    gAirWind = { 4.0f, 0.0f, 1.6f };          // a steady easterly with a nagging crosswind

    // a soft distant cloud-floor far below (pure decor; falling = respawn)
    for (int i=0;i<30;i++){
        float x = frnd(-160,180), z = frnd(-130,130);
        addDecor(D_SPHERE, {x, -34+frnd(-4,4), z}, {frnd(10,22), frnd(4,8), frnd(10,22)}, C_CLOUD);
    }

    // ---- the long rising route: 12 sky-stone terraces, each smaller & farther.
    //      Vault off a terrace, sail, catch the gap's updraft, glide to the next.
    struct SP { float x, z, top, r; };
    SP route[13] = {
        {-64,   0,   2.0f, 6.0f},   // P0 launch terrace (big)
        {-50,  16,  13.0f, 3.4f},   // P1
        {-38,  -8,  25.0f, 3.2f},   // P2
        {-25,  16,  37.0f, 3.0f},   // P3
        {-13,  -6,  49.0f, 3.0f},   // P4
        { -1,  18,  61.0f, 2.9f},   // P5
        { 11,  -4,  73.0f, 2.9f},   // P6
        { 22,  20,  85.0f, 2.8f},   // P7
        { 33,  -2,  97.0f, 2.8f},   // P8
        { 22,  30, 109.0f, 2.9f},   // P9  (route bends north toward the mill)
        { 36,  14, 121.0f, 2.9f},   // P10
        { 46,  34, 132.0f, 3.0f},   // P11
        { 40,   8, 143.0f, 3.2f},   // P12 final approach terrace
    };
    for (int i=0;i<13;i++) addSkyPlat(route[i].x, route[i].z, route[i].top, route[i].r);

    // ---- one updraft per gap: SHORT, gentle columns. Each only carries you to
    //      the next terrace and then tops out - you cannot ride a single column
    //      high enough to skip the level, so every gap must be caught in turn.
    for (int i=0;i<12;i++){
        float mx = (route[i].x + route[i+1].x)*0.5f;
        float mz = (route[i].z + route[i+1].z)*0.5f;
        addUpdraft({mx, route[i].top - 2.0f, mz}, 5.0f, 12.0f, 16.0f);
    }

    // ---- only TWO bail platforms on the whole ascent (far fewer nets) -------
    addSkyPlat(-31, -22,  31.0f, 2.6f);
    addSkyPlat( 30,  38, 100.0f, 2.6f);

    // ---- HAZARD mills: five spinning blade-gates straddle the glide path.
    //      A blade sweeping through you rips the sail away and bats you down -
    //      time your pass through the gap between blades, or steer around.
    struct HZ { float x, z, y, rad, spd; int blades; };
    HZ haz[5] = {
        {-31,  4,  40.0f, 7.0f,  58.0f, 3},   // gate guarding P3
        { -7,  6,  64.0f, 7.0f, -66.0f, 3},   // gate guarding P5
        { 16,  8,  88.0f, 7.5f,  70.0f, 4},   // gate guarding P7
        { 27, 14, 112.0f, 7.5f, -74.0f, 3},   // gate guarding P9
        { 41, 24, 135.0f, 8.0f,  78.0f, 4},   // gate guarding P11
    };
    for (int i=0;i<5;i++){
        addWindmill({haz[i].x, haz[i].y, haz[i].z}, haz[i].rad, frnd(0,6.2f),
                    haz[i].spd, haz[i].blades, (Color){225,60,45,255}, 1);
        addDecor(D_CYL, {haz[i].x, haz[i].y-haz[i].rad-6.0f, haz[i].z},
                 {0.6f, 6.0f, 0.6f}, (Color){120,110,120,255}, TX_STONE);   // stub pylon
    }

    // ---- the goal: the colossal Skymill; the star crowns its hub -----------
    addCyl({52,0,0}, 4.5f, 149.0f, S_SKYSTONE);          // the great tower
    for (int b=0;b<4;b++)                                 // buttress fins
        addDecor(D_CUBE, {52+cosf(b*1.571f)*4.6f, 74, sinf(b*1.571f)*4.6f}, {1.1f,146,1.1f},
                 (Color){214,224,240,255}, TX_SKY);
    addSkyPlat(52, 0, 149.0f, 5.4f);                     // the crown terrace (walk to the star)
    addUpdraft({47, 130.0f, 4}, 7.0f, 24.0f, 18.0f);     // the mill's final gust (P12 -> the crown)
    addWindmill({52, 140, 0}, 24.0f, 0.0f, 14.0f, 4, (Color){236,74,84,255});   // the great blades
    addStreamer({52, 148, 0}, 6.0f, 0.7f, C_GOLD);       // pennant at the very top

    // ---- decorative turbine mills: the landmarks that "make" the wind ------
    addWindmill({-58,  26, -8},  9.0f, 0.0f, 40.0f, 5, (Color){120,200,120,255});
    addWindmill({-20,  58, 24},  8.0f, 0.0f, 34.0f, 4, (Color){250,200,90,255});
    addWindmill({ 30,  92,-10},  8.5f, 0.0f, 46.0f, 6, (Color){150,190,250,255});
    addWindmill({ 10, 128, 30},  7.5f, 0.0f, 44.0f, 5, (Color){240,150,210,255});
    float dmx[4]={-58,-20,30,10}, dmy[4]={26,58,92,128}, dmz[4]={-8,24,-10,30};
    for (int i=0;i<4;i++)   // short masts hanging just below each hub (not full-height poles)
        addDecor(D_CYL, {dmx[i], dmy[i]-14.0f, dmz[i]}, {0.5f, 14.0f, 0.5f}, (Color){214,224,240,255}, TX_SKY);

    // ---- streamers & pinwheels: the wind kingdom flutters on every terrace --
    Color banC[4] = {{236,74,84,255},{250,200,90,255},{120,200,120,255},{150,190,250,255}};
    for (int i=0;i<13;i++){
        addStreamer({route[i].x+route[i].r*0.6f, route[i].top+3.0f, route[i].z}, frnd(2.2f,3.4f), 0.5f, banC[i%4]);
        addPinwheel({route[i].x-route[i].r*0.6f, route[i].top+1.5f, route[i].z}, 0.7f, frnd(30,60), banC[i%4], C_CREAM);
    }

    // ---- dandelion seeds & leaves streaming on the wind (ambient motes) ----
    for (int i=0;i<14;i++)
        addMote({frnd(-70,55), frnd(6,150), frnd(-30,40)},
                (i%2)? (Color){250,250,240,255} : (Color){200,230,180,255}, frnd(3,6), frnd(0.4f,0.9f));

    // ---- distant giant windmills on the horizon (silhouette landmarks) -----
    addWindmill({-170, 60, 100}, 36.0f, 0.0f, 10.0f, 4, (Color){150,170,200,255});
    addWindmill({ 190, 90, -50}, 42.0f, 0.0f,  8.0f, 5, (Color){150,170,200,255});
    if (!gMega){ addCyl({-170,0,100}, 6, 60, S_SKYSTONE); addCyl({190,0,-50}, 7, 90, S_SKYSTONE); }
    else {       // in the ascent they hang from sky-stone shards, not the ground
        addDecor(D_CYL, {-170, 46, 100}, {6, 14, 6}, (Color){222,232,246,255}, TX_SKY);
        addDecor(D_CYL, { 190, 74, -50}, {7, 16, 7}, (Color){222,232,246,255}, TX_SKY);
    }

    // warp pipe home, on the launch terrace
    addWarpPipe({-64, 2.0f, -5}, {150,190,250,255});
}

// -------------------------------------------- level 5: THE BONEWOOD -------
// A twilight grove of petrified giants, and the hardest climb in the game.
// The new power is the WALLSPRING (tap LMB mid-air against a wall to kick off
// it) - and every old power is a GATE here, not a garnish:
//   act 1  THE SPLIT TRUNK   - 34 m chimney, wallspring chains only
//   act 2  THE SILK CROSSING - 26 m void, two blooms, web swing only
//   act 3  THE DRUM          - a red cap 24 m below the only ledge: PERFECT
//                              slam or nothing
//   act 4  THE LONG GLIDE    - 42 m gap, skysail + one spore-geyser updraft
//   act 5  THE EMBER SHELVES - +9.5 m rises: golden-spore turbo chains only
//   act 6  THE HOLLOW BONE   - 40 m chimney to the crown, then a bounce
//                              through the star
// Falls go all the way to the grove floor. One red bail on the whole climb.
static void BuildBonewood(void){
    gSpawn = { 0, 0.91f, -20 };
    gStarP = { 8, 132.6f, 168 };

    if (!gMega) addBox({-220,-2,-80},{220,0,280}, S_GRASS);   // ascent: the bone plateau
    addWarpPipe({12,0,-24}, {228,222,205,255});    // bone-white flag home

    // colors of the dead wood
    const Color BONE   = {235,228,210,255};
    const Color BONE_D = {205,196,176,255};
    const Color GLOW   = {120,235,215,255};        // teal grave-lights

    // in the ascent the grove has NO floor - every trunk hangs in the twilight
    // on its own dead roots, and the realm waits far below
    auto roots = [&](float x, float z, float r){
        if (!gMega) return;
        GrpScope _g;
        addDecor(D_SPHERE, {x, -0.7f, z}, {r*1.3f, 1.2f, r*1.3f}, BONE_D, TX_FIBER);
        for (int k=0;k<4;k++){
            float a = k*1.57f + x*0.7f;
            addDecor(D_CYL, {x+cosf(a)*r*0.7f, -4.4f-(k%2)*1.2f, z+sinf(a)*r*0.7f},
                     {0.14f, 3.8f+(k%2)*1.4f, 0.14f}, {96,74,46,255}, TX_FIBER);
        }
    };

    // THE SPRINGHEART: the shrine that wakes the wallspring, right at the
    // mouth of the first chimney (type 3)
    addShrine({0,0,0}, 3);

    // ---- act 1: THE SPLIT TRUNK (ground -> 34) -----------------------------
    // two bark walls 4.5 m apart; spring wall to wall all the way up
    addBox({-4.25f,0,6},{-2.25f,34,12}, S_WOOD);
    addBox({ 2.25f,0,6},{ 4.25f,34,12}, S_WOOD);
    roots(-3.25f, 9, 2.4f); roots(3.25f, 9, 2.4f);
    addBox({-2.25f,33.3f,12},{2.25f,34,16}, S_WOOD);          // L1 exit lip
    addCoin(0,  9, 9); addCoin(0, 17, 9); addCoin(0, 25, 9); addCoin(0, 32, 9);
    addCoin(0, 35.3f, 14);
    // bark ribs + a shattered crown on each wall stump
    for (int k=0;k<4;k++){
        addDecor(D_CUBE, {-3.25f, 4.0f+k*8.5f, 9}, {2.3f, 0.5f, 6.4f}, (Color){118,78,42,255}, TX_WOOD);
        addDecor(D_CUBE, { 3.25f, 8.0f+k*8.5f, 9}, {2.3f, 0.5f, 6.4f}, (Color){118,78,42,255}, TX_WOOD);
    }
    addDecor(D_CONE, {-3.25f, 34, 7.2f}, {1.2f, 2.6f, 1.2f}, BONE_D, TX_FIBER);
    addDecor(D_CONE, { 3.25f, 34, 10.8f}, {1.0f, 3.2f, 1.0f}, BONE_D, TX_FIBER);

    // ---- act 2: THE SILK CROSSING (34 -> pillar top 38) --------------------
    addWebAnchor({0, 45, 24});
    addWebAnchor({2, 48, 38});
    addCoin(0, 41, 24); addCoin(2, 44, 38);
    if (!gMega) addShroom({-6,0,30}, 6.0f, 4.2f, true);   // the ONE red bail of the grove
    else {                                          // ...which floats out here
        addCap(-6, 30, 10.0f, 4.2f, true);
        roots(-6, 30, 3.2f);
    }
    // P2: a snapped bone pillar (collision cyl + bone visual)
    addCyl({0,0,44}, 2.6f, 38, S_DARK, false, false);
    addDecor(D_CYL, {0,0,44}, {2.75f, 38.2f, 2.75f}, BONE, TX_FIBER);
    addDecor(D_CYL, {0,37.6f,44}, {3.0f, 0.5f, 3.0f}, BONE_D, TX_FIBER);
    roots(0, 44, 2.9f);
    addCoin(0, 39.3f, 44);
    addThunder(1.2f, 39.6f, 44.8f, 2);             // thunder for THE DRUM, and only the drum

    // ---- act 3: THE DRUM (38 -> 54, PERFECT slam only) ---------------------
    // vault off P2 over the red drum, dive, ring it at the last blink
    addCyl({0,0,55}, 2.0f, 28, S_DARK, false, false);         // drum stalk
    addDecor(D_CYL, {0,0,55}, {2.15f, 27.2f, 2.15f}, BONE, TX_FIBER);
    roots(0, 55, 2.3f);
    addCap(0, 55, 30.0f, 2.6f, true);                          // the drum head
    addBox({-2,53.3f,58.5f},{2,54,63}, S_WOOD);                // E3: the only exit
    addCoin(0, 43, 55);                                        // mid-dive prize
    addCoin(0, 55.3f, 61);

    // ---- act 4: THE LONG GLIDE (54 -> 66 -> sail 42 m -> 56) ---------------
    addCap(3, 68, 60.0f, 2.4f, false);                         // tan step
    addBox({-2,65.3f,72},{2,66,76}, S_WOOD);                   // L4 launch lip
    addCoin(3, 61.3f, 68); addCoin(0, 67.3f, 74);
    // the spore geyser: a dead stalk venting lift - catch it under sail.
    // A long glide arrives sinking at SAIL_TERM, so the column must out-muscle
    // that sink hard or it reads as dead air. Its 76 m ceiling still caps the
    // climb well below anything skippable.
    addUpdraft({0, 46, 94}, 8.0f, 30.0f, 34.0f);
    addDecor(D_CYL, {0, 0, 94}, {3.4f, 42.0f, 3.4f}, BONE_D, TX_FIBER);   // the venting stump
    addDecor(D_CYL, {0, 41.6f, 94}, {3.9f, 0.6f, 3.9f}, BONE, TX_FIBER);
    roots(0, 94, 3.7f);
    addCoin(0, 58, 94); addCoin(0, 68, 94);
    // T4: the landing bough
    addCyl({0,54.4f,118}, 3.0f, 1.6f, S_WOOD);
    addDecor(D_CYL, {0,55.7f,118}, {3.2f, 0.4f, 3.2f}, (Color){150,102,52,255}, TX_WOOD);
    addCoin(0, 57.3f, 118);

    // ---- act 5: THE EMBER SHELVES (56 -> 84.5, spore turbo only) -----------
    addSpore(0, 57.4f, 118);
    addCap( 6, 126, 65.5f, 2.2f, false);
    addCap( 0, 134, 75.0f, 2.2f, false);
    addSpore(0, 76.4f, 134);                                   // mid-chain re-light
    addCap(-6, 142, 84.5f, 2.4f, false);
    addCoin(6, 66.8f, 126); addCoin(0, 76.3f, 134); addCoin(-6, 85.8f, 142);

    // ---- act 6: THE HOLLOW BONE (84 -> 124 -> the crown) -------------------
    addBox({-2,83.3f,146},{2,84,150}, S_WOOD);                 // F0 approach lip
    addBox({-4.25f,84,152},{-2.25f,124,162}, S_WOOD);          // the tallest chimney
    addBox({ 2.25f,84,152},{ 4.25f,124,162}, S_WOOD);
    addCoin(0, 95, 157); addCoin(0, 105, 157); addCoin(0, 115, 157); addCoin(0, 122.5f, 157);
    for (int k=0;k<4;k++){
        addDecor(D_CUBE, {-3.25f, 90.0f+k*9.0f, 157}, {2.3f, 0.5f, 10.4f}, (Color){118,78,42,255}, TX_WOOD);
        addDecor(D_CUBE, { 3.25f, 94.5f+k*9.0f, 157}, {2.3f, 0.5f, 10.4f}, (Color){118,78,42,255}, TX_WOOD);
    }
    addBox({6,125.3f,155},{10,126,159}, S_WOOD);               // C: the last perch
    addCoin(8, 127.3f, 157);
    // the crown: a lone red cap on the tallest snapped bone - bounce through
    addCyl({8,0,168}, 2.0f, 126, S_DARK, false, false);
    addDecor(D_CYL, {8,0,168}, {2.2f, 126.2f, 2.2f}, BONE, TX_FIBER);
    roots(8, 168, 2.4f);
    addCap(8, 168, 128.0f, 2.8f, true);
    addCoin(8, 130.4f, 168);

    // ---- scenery: the grove of dead giants ---------------------------------
    // a petrified titan: bleached leaning stalk + cracked grey cap + gill ring
    auto titan = [&](float x, float z, float h, float r, float lean){
        GrpScope _g;
        addDecor(D_CYL, {x, 0, z}, {r, h, r}, BONE, TX_FIBER);
        addDecor(D_CYL, {x, 0, z}, {r*1.35f, h*0.12f, r*1.35f}, BONE_D, TX_FIBER);
        Vector3 c = {x+lean, h+r*0.35f, z};
        addDecor(D_SPHERE, {c.x, c.y-r*0.30f, c.z}, {r*2.9f, r*0.85f, r*2.9f}, (Color){188,180,162,255}, TX_STREAK);
        addDecor(D_SPHERE, c, {r*2.3f, r*1.15f, r*2.3f}, (Color){206,198,180,255}, TX_STREAK);
        addDecor(D_CYL, {c.x, c.y-r*0.62f, c.z}, {r*2.1f, r*0.16f, r*2.1f}, (Color){150,140,124,255});
        for (int k=0;k<5;k++){                                 // shattered rim shards
            float a = k*1.257f + x;
            addDecor(D_CONE, {c.x+cosf(a)*r*2.5f, c.y-r*0.2f, c.z+sinf(a)*r*2.5f},
                     {r*0.35f, r*0.9f, r*0.35f}, BONE_D, TX_FIBER);
        }
    };
    if (!gMega){
        titan(-52,  40, 46, 6.5f,  3.0f);
        titan( 55,  90, 62, 8.0f, -4.0f);
        titan(-48, 140, 38, 5.0f,  2.0f);
        titan( 60, 190, 70, 9.0f, -3.0f);
        titan(-58, 220, 52, 7.0f,  4.0f);
        // the fallen one: a colossal log the road follows (decor mound)
        addDecor(D_SPHERE, {-24, 2.0f, 92}, {9.0f, 4.2f, 34.0f}, (Color){222,214,196,255}, TX_FIBER);
        addDecor(D_SPHERE, {-24, 3.0f, 74}, {6.5f, 5.5f, 7.5f},  BONE_D, TX_FIBER);
        // teal grave-lights along the route bases (pure glow decor)
        float gx[8] = {-3, 4,  -8,  3, -4,  4, -3, 10};
        float gz[8] = { 3, 5,  30, 44, 55, 94, 148, 166};
        for (int i=0;i<8;i++){
            addDecor(D_CYL, {gx[i], 0, gz[i]}, {0.14f, 0.9f, 0.14f}, (Color){186,200,190,255}, TX_FIBER);
            addDecor(D_SPHERE, {gx[i], 1.05f, gz[i]}, {0.5f, 0.36f, 0.5f}, GLOW);
            addDecor(D_SPHERE, {gx[i], 1.05f, gz[i]}, {0.9f, 0.6f, 0.9f}, (Color){120,235,215,60});
        }
    } else {
        // floorless grove: grave-lights ride the perches themselves
        float gx[6] = {0, 2, 0,  0,  8, 8};
        float gy[6] = {34, 38, 54, 56, 126, 84.6f};
        float gz[6] = {14, 44, 61, 118, 157, 148};
        for (int i=0;i<6;i++){
            addDecor(D_CYL, {gx[i], gy[i], gz[i]}, {0.14f, 0.9f, 0.14f}, (Color){186,200,190,255}, TX_FIBER);
            addDecor(D_SPHERE, {gx[i], gy[i]+1.05f, gz[i]}, {0.5f, 0.36f, 0.5f}, GLOW);
            addDecor(D_SPHERE, {gx[i], gy[i]+1.05f, gz[i]}, {0.9f, 0.6f, 0.9f}, (Color){120,235,215,60});
        }
    }
    // spirit motes drifting the whole climb (cool, not warm - this is a grave)
    addMote({ 0, 12,  9}, {150,240,225,255}, 2.6f, 0.5f);
    addMote({ 0, 40, 30}, {190,225,255,255}, 3.2f, 0.45f);
    addMote({ 0, 48, 76}, {150,240,225,255}, 3.0f, 0.55f);
    addMote({ 0, 70, 118}, {190,225,255,255}, 3.4f, 0.5f);
    addMote({ 0, 105, 157}, {150,240,225,255}, 2.8f, 0.6f);
    addMote({ 8, 126, 168}, {255,240,180,255}, 3.0f, 0.65f);
    if (!gMega){
        // pale bone-button families in the grass
        addShroomPatch(-16, 0, 12, 6, 3.2f);
        addShroomPatch( 20, 0, 40, 5, 2.8f);
        addShroomPatch(-30, 0, 60, 6, 3.6f);
        // mist banks pooling between the titans
        for (int k=0;k<7;k++){
            float a = k*0.9f;
            addDecor(D_SPHERE, {sinf(a)*44, 1.4f, 60+cosf(a)*52+60},
                     {frnd(9,15), frnd(1.6f,2.6f), frnd(9,15)}, (Color){222,232,236,90});
        }
    } else {
        // mist rivers drifting BETWEEN the hanging bones
        for (int k=0;k<7;k++){
            float a = k*0.9f;
            addDecor(D_SPHERE, {sinf(a)*26, 26 + k*13.0f, 30+cosf(a)*30+50},
                     {frnd(8,13), frnd(1.4f,2.2f), frnd(8,13)}, (Color){212,222,232,70});
        }
    }
    // grey-green burial hills on the horizon (standalone only)
    if (!gMega)
        for (int i=0;i<9;i++){
            float ang = -0.6f + i*0.35f;
            float d = frnd(300,380);
            addDecor(D_SPHERE, {sinf(ang)*d, -frnd(60,130)*0.55f, 100+cosf(ang)*d},
                     {frnd(60,130), frnd(48,104), frnd(60,130)}, {96,138,104,255});
        }
}

// ===================================================== THE GREAT ASCENT ====
// Level 6: every world, one climb. Zone build offsets (see BLUEPRINT.md):
// castle grounds the tower; the Megashroom grows on a stone crag behind the
// keep; the Sporeway drifts above its crown; the Gorge is a colossal floating
// mesa; Skyhaven's mills ride above its rim; the Bonewood plateau crowns it
// all in twilight. Seams connect them; falls cascade down through everything.
static const Vector3 OFF_SHROOM = {  0,  80, 170};
static const Vector3 OFF_SPORE  = {102, 225, 170};
static const Vector3 OFF_GORGE  = {104, 310, 270};
static const Vector3 OFF_SKY    = {168, 385, 522};
// the Bonewood is built z-FLIPPED at this offset: its climb runs BACK south
// over the canyon, floorless, overlooking everything you've already earned
static const Vector3 OFF_BONE   = {104, 540, 530};
static const Vector3 ASCENT_STAR = {112, 672.6f, 362};

// connective tissue: supports under each zone, the five seams, ground carpet
static void BuildSeams(void){
    // ---- the wider realm: ground carpet + rolling hills under the drift ----
    addBox({ 220,-2,-80},{ 360,0,850}, S_GRASS);
    addBox({-360,-2,-80},{-220,0,850}, S_GRASS);
    addBox({-220,-2,280},{ 220,0,850}, S_GRASS);
    for (int i=0;i<16;i++){                        // foothill spheres under the tower line
        float t = i/15.0f;
        float hx = lerpf(-40, 300, t) + frnd(-70,70);
        float hz = lerpf(240, 820, t) + frnd(-60,60);
        float s  = frnd(26,64);
        addDecor(D_SPHERE, {hx, -s*0.62f, hz}, {s, s*0.75f, s}, {70,172,88,255});
    }
    for (int i=0;i<10;i++){                        // and a far mountain ring
        float ang = i*0.628f;
        float d = frnd(480,580);
        addDecor(D_SPHERE, {120+sinf(ang)*d, -frnd(60,140)*0.5f, 380+cosf(ang)*d},
                 {frnd(90,170), frnd(70,140), frnd(90,170)}, {84,150,96,255});
    }
    for (int i=0;i<26;i++){                        // meadows live under the whole climb
        float x = frnd(-60, 320), z = frnd(280, 800);
        addShroomPatch(x, 0, z, 4, frnd(2.2f,3.6f));
        if (i%3==0) addShroom({x+frnd(8,20), 0, z+frnd(8,20)}, frnd(4,12), frnd(2.2f,6.0f), i%6==0);
    }

    // ---- THE CRAG: stone spur the Megashroom grows from (falls land here) --
    {
        Vector3 CC = {0, 0, OFF_SHROOM.z + 40};              // plateau center (0,210)
        addCyl({CC.x, 0, CC.z}, 42, 78, S_STONE, false, false);
        addCyl({CC.x, 78, CC.z}, 42, 2, S_GRASS);            // the hanging meadow cap
        Color strata[4] = {{164,150,132,255},{186,172,150,255},{150,138,122,255},{200,188,166,255}};
        for (int b=0;b<4;b++)                                 // banded rock flanks
            addDecor(D_CYL, {CC.x, b*19.0f, CC.z}, {42.8f - b*0.5f, 19.4f, 42.8f - b*0.5f},
                     strata[b], TX_STONE);
        addDecor(D_CYL, {CC.x, 76.6f, CC.z}, {43.4f, 1.6f, 43.4f}, {121,96,62,255});   // soil lip
        for (int k=0;k<10;k++){                               // skirt boulders + roots
            float a = k*0.628f + 0.3f;
            addDecor(D_SPHERE, {CC.x+sinf(a)*43.0f, frnd(1,5), CC.z+cosf(a)*43.0f},
                     {frnd(3,7), frnd(2.5f,5), frnd(3,7)}, strata[k%4], TX_STONE);
            addDecor(D_CYL, {CC.x+sinf(a)*41.5f, 68.0f-k%3*4.0f, CC.z+cosf(a)*41.5f},
                     {0.35f, frnd(6,12), 0.35f}, {96,74,46,255}, TX_FIBER);
        }
        for (int k=0;k<16;k++){                               // rim ring: the edge must READ
            float a = k*0.393f + 0.15f;
            float rr = 39.6f + frnd(-0.8f,0.8f);
            if (k%3 == 0)
                addMiniShroom(CC.x+sinf(a)*rr, 80.0f, CC.z+cosf(a)*rr, frnd(0.5f,1.0f), k%6==0);
            else
                addDecor(D_SPHERE, {CC.x+sinf(a)*rr, 80.2f, CC.z+cosf(a)*rr},
                         {frnd(0.8f,1.7f), frnd(0.5f,1.0f), frnd(0.8f,1.7f)},
                         (Color){168,166,158,255}, TX_STONE);
        }
        // two silver threads fall from the hanging meadow into the castle woods
        addDecor(D_CUBE, {-30.5f, 40, 181.0f}, {0.5f, 76, 1.6f}, (Color){160,210,250,200});
        addDecor(D_CUBE, { 26.0f, 42, 245.0f}, {0.6f, 72, 1.8f}, (Color){160,210,250,180});
        addDecor(D_SPHERE, {-30.5f, 1.2f, 181.0f}, {2.4f,1.0f,2.4f}, (Color){230,244,252,150});
        addDecor(D_SPHERE, { 26.0f, 1.2f, 245.0f}, {2.6f,1.1f,2.6f}, (Color){230,244,252,150});
        addFlag({2, 80, 172}, {80,220,90,255});               // zone flag: the hanging meadow
    }
    // ---- seam 1: spire -> crag. Cloud steps off the balcony ring -----------
    {
        struct { float x,z,top; } cl[3] = {{0,153,82.0f},{4,161,81.0f},{-3,169,80.5f}};
        for (int i=0;i<3;i++){
            addCyl({cl[i].x, cl[i].top-0.9f, cl[i].z}, 3.1f, 0.9f, S_CLOUD, false, false);
            addCoin(cl[i].x, cl[i].top+1.3f, cl[i].z);
            Vector3 c = {cl[i].x, cl[i].top-0.55f, cl[i].z};
            addDecor(D_SPHERE, c, {3.2f,0.95f,3.2f}, C_CLOUD);
            addDecor(D_SPHERE, {c.x-1.7f,c.y-0.1f,c.z+0.5f}, {1.9f,0.75f,1.9f}, C_CLOUD);
            addDecor(D_SPHERE, {c.x+1.6f,c.y-0.1f,c.z-0.4f}, {2.0f,0.8f,2.0f}, C_CLOUD);
        }
    }
    // ---- seam 2: crown -> Sporeway shore mound (they nearly touch) ---------
    addCoin(4, 229.6f, 210); addCoin(8, 230.0f, 210);

    // ---- seam 3: last island -> THE HANGING MESA ---------------------------
    addIsle(104, 215.0f, 304.0f, 3.0f);
    addIsle(104, 221.5f, 308.0f, 2.6f);
    addFlag({101, 310, 228}, {255,150,60,255});               // zone flag: the mesa
    // the mesa itself: slab under the whole canyon + eroded underbelly
    addBox({74, 304, 226},{134, 310, 514}, S_SAND);
    {   // strata lines on its exposed faces (sells "cut rock", not "flat box")
        Color sa = {196,120,70,255}, sb = {228,198,150,255};
        for (int f=0; f<2; f++){
            float fx = f? 134.12f : 73.88f;
            addDecor(D_CUBE, {fx, 305.3f, 370}, {0.12f, 0.8f, 143}, sa);
            addDecor(D_CUBE, {fx, 307.9f, 370}, {0.12f, 1.0f, 143}, sb);
        }
        addDecor(D_CUBE, {104, 305.3f, 226.1f}, {30, 0.8f, 0.1f}, sa);
        addDecor(D_CUBE, {104, 307.9f, 226.1f}, {30, 1.0f, 0.1f}, sb);
    }
    for (int i=0;i<6;i++){
        float uz = 250 + i*45.0f;
        for (int k=0;k<3;k++)
            addDecor(D_SPHERE, {104 + sinf(i*1.7f)*8.0f, 298.0f - k*13.0f, uz},
                     {(34-k*8)*1.0f, 11.0f, (40-k*9)*1.0f},
                     ctint((Color){178,128,88,255}, 1.0f-k*0.09f), TX_SAND);
    }
    for (int k=0;k<9;k++)                                     // dangling vines off the rim
        addDecor(D_CYL, {74.5f + (k%2)*59.0f, 292.0f - (k%3)*4.0f, 240 + k*30.0f},
                 {0.09f, frnd(9,16), 0.09f}, {70,150,60,255});
    // the mouth stream pours off the rim into open sky
    addDecor(D_CUBE, {98, 288, 226.4f}, {1.6f, 44, 0.5f}, (Color){160,210,250,190});
    addDecor(D_CUBE, {98, 258, 226.8f}, {2.2f, 22, 0.5f}, (Color){200,230,252,120});
    addDecor(D_SPHERE, {98, 246, 227}, {3.2f,1.6f,3.2f}, (Color){235,245,252,110});
    for (int i=0;i<5;i++)                                     // floating shards keep the mesa company
        addDecor(D_SPHERE, {74 + frnd(-40,-14), frnd(280,330), 240 + i*52.0f},
                 {frnd(3,6), frnd(2.4f,4.6f), frnd(3,6)}, (Color){186,138,96,255}, TX_SAND);

    // ---- seam 4: gorge crown boing -> Skyhaven P0 (the Skysail shrine) -----
    addSkyPlat(104, 512, 381.0f, 2.8f);                       // one mercy plat under the leap
    addShrine({101, 387.0f, 519}, 2);                         // THE SKYSAIL wakes here
    addCoin(104, 381.5f, 495); addCoin(104, 385.0f, 505); addCoin(107, 388.0f, 515);

    // ---- seam 5: THE LAST WIND. Off the crown mill the easterly dies and a
    // westward breath takes you: sail toward the hanging grove, catching two
    // dying gusts on the way. The Bonewood has NO floor - it hangs over the
    // whole realm, and so will you.
    addUpdraft({192, 500, 514}, 6.5f, 48.0f, 26.0f);          // first dying gust
    addUpdraft({152, 508, 524}, 6.5f, 48.0f, 26.0f);          // second, higher
    addSkyPlat(166, 519, 504.0f, 2.4f);                       // one breath of stone between them
    addCoin(196, 540, 516); addCoin(172, 543, 520); addCoin(140, 546, 526); addCoin(118, 543, 534);
    {   // THE GATE OF BONES: a floating fossil disc holds the Springheart and
        // the mouth of the Split Trunk. Beyond it, nothing but the climb.
        addCyl({104, 538, 540}, 13, 2, S_SKYSTONE);
        addDecor(D_CYL, {104, 539.9f, 540}, {13.2f, 0.16f, 13.2f}, (Color){205,199,215,255});   // slate cast
        addDecor(D_CYL, {104, 537.4f, 540}, {13.5f, 0.7f, 13.5f}, (Color){205,196,176,255}, TX_FIBER);
        for (int k=0;k<3;k++)                                  // under-lobes + hanging roots
            addDecor(D_SPHERE, {104, 534.0f - k*7.0f, 540}, {(12.0f-k*3.4f), 6.0f, (12.5f-k*3.4f)},
                     ctint((Color){212,206,190,255}, 1.0f-k*0.08f), TX_FIBER);
        for (int k=0;k<7;k++){
            float a = k*0.9f;
            addDecor(D_CYL, {104+sinf(a)*11.0f, 528.0f-(k%3)*3.0f, 540+cosf(a)*11.0f},
                     {0.16f, frnd(5,9), 0.16f}, {96,74,46,255}, TX_FIBER);
        }
        for (int k=0;k<5;k++){                                 // rim shards
            float a = k*1.257f + 0.4f;
            addDecor(D_CONE, {104+sinf(a)*12.2f, 540, 540+cosf(a)*12.2f},
                     {0.5f, frnd(1.2f,2.4f), 0.5f}, (Color){205,196,176,255}, TX_FIBER);
        }
        addFlag({110, 540, 548}, {228,222,205,255});           // zone flag: the grove
    }
    // petrified titans adrift on their own torn-out roots, flanking the climb
    {
        float tx[3] = {70, 134, 66}, ty[3] = {540, 575, 610}, tz[3] = {480, 430, 395};
        float th[3] = {26, 32, 22},  tr[3] = {4.2f, 5.0f, 3.6f};
        for (int i=0;i<3;i++){
            addDecor(D_CYL, {tx[i], ty[i]-2.2f, tz[i]}, {tr[i]*2.0f, 2.2f, tr[i]*2.0f}, (Color){205,196,176,255}, TX_FIBER);
            addDecor(D_SPHERE, {tx[i], ty[i]-4.6f, tz[i]}, {tr[i]*1.5f, 3.4f, tr[i]*1.5f}, (Color){186,178,166,255}, TX_FIBER);
            addDecor(D_CYL, {tx[i], ty[i], tz[i]}, {tr[i], th[i], tr[i]}, (Color){235,228,210,255}, TX_FIBER);
            addDecor(D_SPHERE, {tx[i]+2, ty[i]+th[i]+tr[i]*0.3f, tz[i]},
                     {tr[i]*2.7f, tr[i]*1.0f, tr[i]*2.7f}, (Color){206,198,180,255}, TX_STREAK);
            addDecor(D_SPHERE, {tx[i], ty[i]+th[i]+1.0f, tz[i]}, {0.5f,0.36f,0.5f}, (Color){120,235,215,255});
            for (int k=0;k<4;k++){
                float a = k*1.57f + i;
                addDecor(D_CYL, {tx[i]+cosf(a)*tr[i]*1.4f, ty[i]-9.0f-(k%2)*2.0f, tz[i]+sinf(a)*tr[i]*1.4f},
                         {0.16f, 4.5f+(k%2)*1.5f, 0.16f}, {96,74,46,255}, TX_FIBER);
            }
        }
    }
    // deep mercy: two lone red caps far below the grove's fall lines - catch
    // one and you've only lost the zone, not the realm
    addCap(104, 494, 468.0f, 3.6f, true);
    for (int k=0;k<3;k++)
        addDecor(D_CYL, {104+sinf(k*2.1f)*1.6f, 461.0f-k*0.8f, 494+cosf(k*2.1f)*1.6f},
                 {0.09f, 2.6f+k*0.8f, 0.09f}, {112,78,46,255});
    addCap(104, 430, 436.0f, 3.6f, true);
    for (int k=0;k<3;k++)
        addDecor(D_CYL, {104+sinf(k*2.1f)*1.6f, 429.0f-k*0.8f, 430+cosf(k*2.1f)*1.6f},
                 {0.09f, 2.6f+k*0.8f, 0.09f}, {112,78,46,255});

    // ---- drifting cloud banks stitch the bands together --------------------
    struct { float x,y,z; } cb[10] = {
        {-55,175,250},{60,195,230},{-30,300,205},{150,338,240},{60,452,420},
        {160,470,540},{100,562,560},{290,585,620},{40,120,120},{190,250,330} };
    for (int i=0;i<10;i++){
        Vector3 c = {cb[i].x, cb[i].y, cb[i].z};
        Color cc = (cb[i].y > 520)? (Color){225,214,238,255} : C_CLOUD;   // twilight-kissed up high
        addDecor(D_SPHERE, c, {8.0f,2.2f,8.0f}, cc);
        addDecor(D_SPHERE, {c.x-4.6f,c.y-0.4f,c.z+1.2f}, {5.0f,1.7f,5.0f}, cc);
        addDecor(D_SPHERE, {c.x+4.8f,c.y-0.3f,c.z-1.0f}, {5.4f,1.8f,5.4f}, cc);
    }
    // far floating shard-isles for depth at altitude
    for (int i=0;i<6;i++){
        float sx = (i%2)? -160.0f+i*18 : 320.0f+i*14;
        float sy = 340 + i*52.0f, sz = 180 + i*90.0f;
        addDecor(D_CYL, {sx, sy-2.0f, sz}, {frnd(7,12), 2.2f, frnd(7,12)}, (Color){222,232,246,255}, TX_SKY);
        addDecor(D_SPHERE, {sx, sy-4.4f, sz}, {frnd(4,7), 3.6f, frnd(4,7)}, (Color){206,216,234,255}, TX_SKY);
    }
}

static void BuildAscent(void){
    gMega = true;
    gBOff = {0,0,0};      BuildCastle();
    gBOff = OFF_SHROOM;   BuildMegashroom();
    gBOff = OFF_SPORE;    BuildSporeway();
    gBOff = OFF_GORGE;    BuildGorge();
    gBOff = OFF_SKY;      BuildSkyhaven();
    gBOff = OFF_BONE;     gBFlip = true;  BuildBonewood();  gBFlip = false;
    gBOff = {0,0,0};
    BuildSeams();
    gMega = false;
    gSpawn = SPAWN_POS;                    // one meadow. one star. 670 m apart.
    gStarP = ASCENT_STAR;
    gAirWind = {0,0,0};                    // banded wind is applied live per-frame
}

// world bounds (player clamp + save sanity), set per level by BuildWorld
static float gBndX = 215, gBndZ0 = -75, gBndZ1 = 275;

// ------------------------------------------------------------ dispatcher --
// LEVEL EDITOR: if levels/levelN.txt exists (saved from the F4 editor), it
// REPLACES the code builder for that level. Delete the file to get the code
// version back. LoadLevelFile lives in level_io.h (same TU).
static bool LoadLevelFile(int level);
static void BuildWorld(int level = 0, bool forceCode = false){
    gLevel = level;
    solids.clear(); decor.clear(); gWebAnchors.clear(); gCoins.clear(); gSpores.clear();
    gShrines.clear(); gMotes.clear(); gThunders.clear();   // else they pile up every warp
    gUpdrafts.clear(); gWindmills.clear(); gBanners.clear(); gPinwheels.clear();
    gAirWind = {0,0,0};
    gGrp = 0; gGrpDepth = 0; gWarpGrp = -1;
    gBOff = {0,0,0}; gMega = false; gBFlip = false; gBakeDirty = true;
    if (level == 6){ gBndX = 340; gBndZ0 = -80; gBndZ1 = 850; }
    else           { gBndX = 215; gBndZ0 = -75; gBndZ1 = 275; }
    solids.reserve(level == 6? 2400 : 600); decor.reserve(level == 6? 8000 : 1400);
    if (!forceCode && LoadLevelFile(level)) return;
    if      (level == 1) BuildMegashroom();
    else if (level == 2) BuildSporeway();
    else if (level == 3) BuildGorge();
    else if (level == 4) BuildSkyhaven();
    else if (level == 5) BuildBonewood();
    else if (level == 6) BuildAscent();
    else                 BuildCastle();
}
