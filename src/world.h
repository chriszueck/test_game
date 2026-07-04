// ============================================================================
// world.h : level geometry — meadow routes, castle, courtyard routes, spire
//
// Route map (foddian, no checkpoints — but many roads up):
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

// texture slots (filled in gfx.h)
enum TexId { TX_WHITE, TX_GRASS, TX_STONE, TX_BRICK, TX_Q, TX_COUNT };

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
static Solid* addBox(Vector3 mn, Vector3 mx, int surf, bool bouncy=false, bool vis=true){
    Solid s{}; s.isCyl=false; s.mn=mn; s.mx=mx; s.surf=surf; s.bouncy=bouncy; s.visible=vis;
    solids.push_back(s); return &solids.back();
}
static Solid* addCyl(Vector3 base, float rad, float hgt, int surf, bool bouncy=false, bool vis=true){
    Solid s{}; s.isCyl=true; s.base=base; s.rad=rad; s.hgt=hgt;
    s.mn={base.x-rad, base.y, base.z-rad}; s.mx={base.x+rad, base.y+hgt, base.z+rad};
    s.surf=surf; s.bouncy=bouncy; s.visible=vis;
    solids.push_back(s); return &solids.back();
}
static void addDecor(int kind, Vector3 pos, Vector3 scale, Color col, int tex=TX_WHITE, float rotY=0){
    decor.push_back({kind, pos, scale, col, tex, rotY});
}

// mushroom: stem + cap (cap solid is a cylinder; drawn as a chunky dome + spots)
static void addShroom(Vector3 groundPos, float capTopY, float capR, bool red){
    float capH   = clampf(capR*0.70f, 1.3f, 3.0f);
    float stemR  = clampf(capR*0.40f, 0.6f, 1.7f);
    float stemH  = capTopY - groundPos.y - capH*0.60f;
    addCyl(groundPos, stemR, stemH + 0.2f, S_DARK, false, false);                  // stem collision
    // stem: fat base flare, straight shaft, collar bulge under the cap
    addDecor(D_CYL, groundPos, {stemR*1.30f, fmaxf(stemH*0.30f,0.5f), stemR*1.30f}, (Color){247,236,210,255});
    addDecor(D_CYL, groundPos, {stemR, stemH + 0.15f, stemR}, C_CREAM);
    addDecor(D_SPHERE, {groundPos.x, groundPos.y+stemH-0.25f, groundPos.z},
             {stemR*1.35f, 0.55f, stemR*1.35f}, (Color){246,233,203,255});
    Vector3 capBase = { groundPos.x, capTopY - capH, groundPos.z };
    addCyl(capBase, capR*0.96f, capH, red ? S_SHROOM_RED : S_SHROOM_TAN, red, false);
    // cap: one big dome spanning the full cap height, tiny poke above the walk surface
    Color cc = red ? C_RED : C_TAN;
    float domeTop = capTopY + 0.28f, domeBot = capTopY - capH - 0.15f;
    Vector3 capC = { groundPos.x, (domeTop+domeBot)*0.5f, groundPos.z };
    float ry = (domeTop-domeBot)*0.5f;
    addDecor(D_SPHERE, capC, {capR, ry, capR}, cc);
    addDecor(D_CYL, {capC.x, domeBot+0.02f, capC.z}, {capR*0.80f, 0.20f, capR*0.80f},
             (Color){250,242,222,255});                                            // pale underside
    // big evenly-spaced spots that sit on the dome surface
    int nd = 3 + (int)(capR*1.1f);
    for (int i=0;i<nd;i++){
        float a  = (i + frnd(-0.18f,0.18f)) * 6.283f/nd;
        float rr = frnd(capR*0.30f, capR*0.62f);
        float dotR = clampf(capR*0.22f, 0.32f, 0.85f)*frnd(0.85f,1.15f);
        float dy = capC.y + ry*sqrtf(fmaxf(0.0f, 1.0f - (rr*rr)/(capR*capR))) - dotR*0.18f;
        addDecor(D_SPHERE, {capC.x+cosf(a)*rr, dy, capC.z+sinf(a)*rr}, {dotR, dotR*0.45f, dotR}, C_CREAM);
    }
}
static void addPipe(Vector3 groundPos, float rad, float hgt){
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
    addDecor(D_CYL,  {topCenter.x, topCenter.y-0.1f, topCenter.z}, {rad+0.5f, 0.5f, rad+0.5f}, C_STONE_B, TX_STONE);
    addDecor(D_CONE, {topCenter.x, topCenter.y+0.4f, topCenter.z}, {rad+0.7f, rad*1.5f, rad+0.7f}, C_ROOF);
    addDecor(D_SPHERE, {topCenter.x, topCenter.y+0.4f+rad*1.5f, topCenter.z}, {0.35f,0.35f,0.35f}, C_GOLD);
}
// route flag: pole + pennant (pure decor — marks where a path starts)
static void addFlag(Vector3 groundPos, Color col){
    addDecor(D_CYL,   groundPos, {0.08f, 3.0f, 0.08f}, C_WOOD);
    addDecor(D_CUBE,  {groundPos.x+0.5f, groundPos.y+2.62f, groundPos.z}, {0.9f, 0.45f, 0.06f}, col);
    addDecor(D_SPHERE,{groundPos.x, groundPos.y+3.08f, groundPos.z}, {0.14f,0.14f,0.14f}, C_GOLD);
}
// spinning collectible coins trace the routes (drawn + collected in main.cpp)
struct Coin { Vector3 p; bool taken; };
static std::vector<Coin> gCoins;
static void addCoin(float x, float y, float z){ gCoins.push_back({{x,y,z}, false}); }
// hanging pennant banner on a wall face (face normal along -Z; rotY to reorient)
static void addBanner(Vector3 topCenter, Color col, float w=1.5f, float h=2.6f){
    addDecor(D_CUBE, {topCenter.x, topCenter.y-h*0.5f, topCenter.z}, {w, h, 0.14f}, col);
    addDecor(D_CUBE, {topCenter.x, topCenter.y+0.06f, topCenter.z}, {w+0.25f, 0.22f, 0.2f}, C_GOLD);
}
static void addTorch(Vector3 groundPos){
    addDecor(D_CYL,    groundPos, {0.09f, 2.7f, 0.09f}, C_WOOD);
    addDecor(D_SPHERE, {groundPos.x, groundPos.y+2.95f, groundPos.z}, {0.30f,0.42f,0.30f}, {255,140,30,255});
    addDecor(D_SPHERE, {groundPos.x, groundPos.y+3.06f, groundPos.z}, {0.16f,0.24f,0.16f}, {255,225,95,255});
}
static void addIvy(Vector3 base, float faceZ, int n){
    for (int i=0;i<n;i++)
        addDecor(D_SPHERE, {base.x+frnd(-1.4f,1.4f), base.y+frnd(0.3f,5.5f), faceZ},
                 {frnd(0.5f,1.0f), frnd(0.5f,0.9f), 0.35f}, {56,150,60,255});
}

// ------------------------------------------------------------- the level ---
static void BuildWorld(void){
    solids.reserve(600); decor.reserve(1400);

    // ---- meadow ------------------------------------------------------------
    addBox({-220,-2,-80},{220,0,280}, S_GRASS);

    // tutorial brick wall (forces the first vault)
    addBox({-15,0,13.4f},{15,2.6f,16.6f}, S_BRICK);

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

    // route flags just past the wall — pick a lane
    addFlag({10.5f,0,17.0f}, { 80,220, 90,255});   // green  : west ladder (steady)
    addFlag({16.0f,0,22.0f}, {255,210, 60,255});   // yellow : pipe yard (precision)
    addFlag({ 4.0f,0,35.0f}, C_RED);               // red    : watchtower skip (expert)

    // ---- zone 1 / WEST LADDER: shroom -> pipe -> two tall tans -> wall ------
    addShroom({  7,0, 20},  4.0f, 3.4f, false);    // tan A
    addPipe  ({  0,0, 29},  2.4f, 7.0f);           // mid pipe
    addShroom({-10,0, 42}, 11.5f, 3.2f, false);    // tan B
    addShroom({-18,0, 56}, 15.0f, 3.2f, false);    // tan C -> wall top (16)
    addShroom({-26,0, 50},  5.0f, 3.4f, true);     // red bail west
    addShroom({ -4,0, 60},  4.5f, 3.0f, true);     // red bail near wall

    // ---- zone 1 / EAST PIPE YARD: five pipes, small pads, rising ------------
    addPipe({18,0,26}, 2.4f,  4.5f);
    addPipe({24,0,34}, 2.4f,  8.0f);
    addPipe({19,0,43}, 2.4f, 11.5f);
    addPipe({25,0,52}, 2.4f, 14.0f);
    addPipe({20,0,62}, 2.7f, 15.5f);               // fat final pipe -> wall top
    addShroom({28,0,44}, 5.0f, 3.4f, true);        // red bail east

    // ---- zone 1 / CENTER WATCHTOWER: two PERFECT run-vaults skip the lot ----
    addBox({2.5f,0,54.0f},{6.0f,8.0f,57.5f}, S_BRICK);
    addDecor(D_CUBE,  {4.25f,4.8f,53.9f}, {1.2f,1.8f,0.25f}, {40,34,30,255});  // window
    addDecor(D_SPHERE,{2.9f,8.32f,54.4f}, {0.28f,0.28f,0.28f}, C_GOLD);        // finial
    addShroom({11,0,50}, 4.0f, 3.0f, true);        // red bail center

    // bonus meadow ?-blocks (side hops for coins-and-grins energy)
    addQBlock({-30, 7, 24}, 2.6f);
    addQBlock({ 36, 9, 40}, 2.6f);
    addCoin(-30, 8.3f, 24); addCoin(36, 10.3f, 40);

    // zone-1 coins: an arc over the first wall teaches the vault's shape
    addCoin(0, 3.4f, 12.5f); addCoin(0, 4.3f, 15); addCoin(0, 3.4f, 17.5f);
    addCoin(  7, 5.3f, 20); addCoin( 0, 8.3f, 29); addCoin(-10, 12.8f, 42); addCoin(-18, 16.3f, 56);
    addCoin( 18, 5.8f, 26); addCoin(24, 9.3f, 34); addCoin( 19, 12.8f, 43);
    addCoin( 25, 15.3f, 52); addCoin(20, 16.8f, 62);
    addCoin(4.25f, 9.3f, 55.8f); addCoin(4, 13.5f, 61); addCoin(4, 15.5f, 66);

    // decorative meadow shrooms (non-path flavor, far from routes)
    addShroom({ 45,0, 55}, 7.0f, 4.2f, false);
    addShroom({-55,0, 30}, 8.0f, 4.6f, false);
    addShroom({-70,0, 75}, 6.0f, 4.0f, true);
    addShroom({ 70,0, 90}, 9.0f, 5.0f, false);

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
    addShroom({22,0, 88}, 21.0f, 4.2f, false);                    // G1 (vault on from wall top)
    addShroom({26,0,102}, 26.0f, 4.2f, false);                    // G2
    addShroom({19,0,112}, 31.0f, 4.0f, false);                    // G3
    addBox({18,35.2f,120.6f},{20.8f,35.7f,123.6f}, S_WOOD);       // beams bolted to keep's east face
    addBox({18,39.7f,129.6f},{20.8f,40.2f,132.6f}, S_WOOD);
    addBox({18,44.0f,138.6f},{20.8f,44.5f,141.6f}, S_WOOD);       // -> keep roof 46
    addCoin(22, 22.3f, 88); addCoin(26, 27.3f, 102); addCoin(19, 32.3f, 112);
    addCoin(19, 36.9f, 122.1f); addCoin(19, 41.4f, 131.1f); addCoin(19, 45.7f, 140.1f);

    // ---- zone 2 / ROUTE C: bold gap-bricks across the courtyard -------------
    addBox({11.8f,20.7f,83.8f},{16.2f,21.0f,88.2f}, S_BRICK);     // run-vault from wall top
    addBox({ 5.8f,25.7f,94.8f},{10.2f,26.0f,99.2f}, S_BRICK);
    addBox({-0.2f,30.7f,104.8f},{ 4.2f,31.0f,109.2f}, S_BRICK);   // -> first balcony
    addCoin(14, 22.3f, 86); addCoin(8, 27.3f, 97); addCoin(2, 32.3f, 107);

    // courtyard red bails + flavor + fountain
    addShroom({-10,0, 85}, 5.0f, 3.4f, true);
    addShroom({  4,0, 95}, 5.0f, 3.4f, true);
    addShroom({ -2,0,108}, 5.0f, 3.4f, true);
    addShroom({-20,0,120}, 6.0f, 3.6f, false);
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
    // THE FINALE: a giant red trampoline cap seals the spire top — there is
    // nowhere to stand up there. Vault onto it and the bounce carries you
    // through the star. Reds are not optional anymore.
    addCyl({0,90,136}, 5.2f, 0.9f, S_SHROOM_RED, true, false);    // overhangs: no standable rim
    addDecor(D_SPHERE, {0,89.6f,136}, {5.6f,1.6f,5.6f}, C_RED);
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
