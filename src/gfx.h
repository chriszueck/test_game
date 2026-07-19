// ============================================================================
// gfx.h : toon shader, procedural textures, models, world/star/pole rendering
// ============================================================================
#pragma once
#include "game.h"
#include "world.h"

static const char* TOON_VS =
"#version 330\n"
"in vec3 vertexPosition; in vec2 vertexTexCoord; in vec3 vertexNormal; in vec4 vertexColor;\n"
"uniform mat4 mvp; uniform mat4 matModel; uniform mat4 matNormal;\n"
"out vec2 fragTexCoord; out vec4 fragColor; out vec3 fragNormal; out vec3 fragPos;\n"
"void main(){\n"
"  fragTexCoord = vertexTexCoord; fragColor = vertexColor;\n"
"  fragNormal = normalize(vec3(matNormal*vec4(vertexNormal,0.0)));\n"
"  fragPos = vec3(matModel*vec4(vertexPosition,1.0));\n"
"  gl_Position = mvp*vec4(vertexPosition,1.0);\n"
"}\n";

static const char* TOON_FS =
"#version 330\n"
"in vec2 fragTexCoord; in vec4 fragColor; in vec3 fragNormal; in vec3 fragPos;\n"
"uniform sampler2D texture0; uniform vec4 colDiffuse;\n"
"uniform vec3 sunDir; uniform vec3 camPos; uniform vec3 fogColor; uniform float fogDensity;\n"
"out vec4 finalColor;\n"
"void main(){\n"
"  vec4 tex = texture(texture0, fragTexCoord);\n"
"  vec3 base = tex.rgb * colDiffuse.rgb * fragColor.rgb;\n"
"  vec3 N = normalize(fragNormal);\n"
"  float ndl = max(dot(N, -sunDir), 0.0);\n"
"  float band = ndl > 0.62 ? 1.0 : (ndl > 0.34 ? 0.82 : (ndl > 0.12 ? 0.66 : 0.52));\n"  // 4 cel bands
"  vec3 col = base * band * mix(vec3(0.90,0.95,1.10), vec3(1.05,1.00,0.93), band);\n"    // warm light, cool shade
"  vec3 V = normalize(camPos - fragPos);\n"
"  float rim = 1.0 - max(dot(V, N), 0.0);\n"                                             // cartoon edge darkening
"  col *= 1.0 - 0.20*smoothstep(0.60, 0.95, rim);\n"
"  float d = length(fragPos - camPos);\n"
"  float f = clamp(1.0 - exp(-fogDensity*fogDensity*d*d), 0.0, 1.0);\n"
"  col = mix(col, fogColor, f);\n"
"  finalColor = vec4(col, tex.a * colDiffuse.a);\n"
"}\n";

static Shader  gToon;
static int     gLocSun, gLocCam, gLocFogC, gLocFogD;
static Texture gTex[TX_COUNT];
static Model   gCube, gCyl, gSphere, gCone, gPlane;
// static-decor bake state (see BakeWorldDecor below)
struct BakedChunk { Mesh mesh; int tex; Vector3 c; float rad; };
static std::vector<BakedChunk> gBaked;
static Material gBakeMat;
static bool gEditMode = false;                // toggled in editor.h (same TU)

// altitude-graded sky: day at the meadow, thin and pale by the island band,
// peach dusk at the mesa rim, deep star-pricked twilight in the Bonewood.
// RenderAll samples these by camera height every frame; fog follows.
static Color gSkyTop = {  92, 148, 252, 255 };
static Color gSkyFog = { 178, 214, 255, 255 };
static Color lerpC(Color a, Color b, float t){
    return { (unsigned char)lerpf(a.r,b.r,t), (unsigned char)lerpf(a.g,b.g,t),
             (unsigned char)lerpf(a.b,b.b,t), 255 };
}
static void SkyAt(float y){
    struct K { float y; Color top, fog; };
    static const K ks[5] = {
        {   0, { 92,148,252,255}, {178,214,255,255} },   // meadow day
        { 270, {118,168,250,255}, {204,226,252,255} },   // the island band: thinner air
        { 440, {154,134,212,255}, {244,190,168,255} },   // mesa rim: peach dusk
        { 565, { 76, 66,150,255}, {160,124,166,255} },   // violet hour
        { 665, { 34, 32, 90,255}, {104, 90,144,255} },   // the Bonewood: deep twilight
    };
    int i = 0;
    while (i < 3 && y > ks[i+1].y) i++;
    float t = clampf((y - ks[i].y)/(ks[i+1].y - ks[i].y), 0, 1);
    gSkyTop = lerpC(ks[i].top, ks[i+1].top, t);
    gSkyFog = lerpC(ks[i].fog, ks[i+1].fog, t);
}

// -------------------------------------------------------------- textures ---
static Texture makeTex(Image img, bool mips){
    Texture t = LoadTextureFromImage(img);
    UnloadImage(img);
    if (mips){ GenTextureMipmaps(&t); SetTextureFilter(t, TEXTURE_FILTER_TRILINEAR); }
    else SetTextureFilter(t, TEXTURE_FILTER_BILINEAR);
    return t;
}
static void LoadTextures(void){
    gTex[TX_WHITE] = makeTex(GenImageColor(4,4,WHITE), false);
    { // meadow: soft checker + mottled patches + sparse far-away flower dots
        Image im = GenImageChecked(512,512,8,8,C_GRASS_A,C_GRASS_B);
        for (int i=0;i<650;i++){
            Color p = (i%3)? (Color){86,186,60,255} : (Color){106,210,80,255};
            ImageDrawRectangle(&im, GetRandomValue(0,508), GetRandomValue(0,508), 4, 4, p);
        }
        for (int i=0;i<90;i++){
            Color f = (i%4==0)? (Color){255,230,110,255} : (Color){250,250,250,255};
            ImageDrawRectangle(&im, GetRandomValue(0,510), GetRandomValue(0,510), 2, 2, f);
        }
        gTex[TX_GRASS] = makeTex(im, true);
    }
    { // stone: staggered masonry blocks with mortar grooves + per-block tint
        Image im = GenImageColor(256,256,(Color){196,190,176,255});     // mortar
        for (int row=0; row<8; row++){
            int off = (row%2)*32;
            for (int cx=-64; cx<256; cx+=64){
                int v = GetRandomValue(0,16);
                Color blk = { (unsigned char)(230-v), (unsigned char)(226-v), (unsigned char)(213-v), 255 };
                ImageDrawRectangle(&im, cx+off+2, row*32+2, 60, 28, blk);
                // subtle top-edge highlight per block
                ImageDrawRectangle(&im, cx+off+2, row*32+2, 60, 3, (Color){241,238,227,255});
            }
        }
        gTex[TX_STONE] = makeTex(im, true);
    }
    { // bricks: base + mortar lines, staggered
        Image im = GenImageColor(256,256,C_BRICK);
        for (int row=0; row<8; row++){
            ImageDrawRectangle(&im, 0, row*32, 256, 4, C_BRICK_D);
            int off = (row%2)*32;
            for (int cx=off; cx<256+64; cx+=64) ImageDrawRectangle(&im, cx-2, row*32, 4, 32, C_BRICK_D);
        }
        gTex[TX_BRICK] = makeTex(im, true);
    }
    { // wood planks: 4 vertical boards, grain streaks, nail dots
        Image im = GenImageColor(128,128,(Color){176,120,58,255});
        for (int p=0;p<4;p++){
            int x0 = p*32;
            ImageDrawRectangle(&im, x0, 0, 2, 128, (Color){118,72,30,255});      // board seam
            for (int g=0;g<5;g++)                                                // grain
                ImageDrawRectangle(&im, x0+4+GetRandomValue(0,24), GetRandomValue(0,96),
                                   1, GetRandomValue(18,42), (Color){150,98,44,255});
            ImageDrawRectangle(&im, x0+14, 6,   3, 3, (Color){96,58,24,255});     // nails
            ImageDrawRectangle(&im, x0+14, 119, 3, 3, (Color){96,58,24,255});
        }
        gTex[TX_WOOD] = makeTex(im, true);
    }
    { // stem fiber: white base, soft vertical streaks (tinted per use)
        Image im = GenImageColor(64,128,WHITE);
        for (int i=0;i<26;i++){
            int x = GetRandomValue(0,63);
            ImageDrawRectangle(&im, x, GetRandomValue(0,60), GetRandomValue(1,2),
                               GetRandomValue(30,68), (Color){233,224,206,255});
        }
        gTex[TX_FIBER] = makeTex(im, true);
    }
    { // cap streaks: white base, faint radial streaking + top sheen (tinted per cap)
        Image im = GenImageColor(128,128,WHITE);
        for (int i=0;i<34;i++){
            int x = GetRandomValue(0,127);
            ImageDrawRectangle(&im, x, GetRandomValue(20,70), GetRandomValue(1,3),
                               GetRandomValue(30,58), (Color){229,221,211,255});
        }
        ImageDrawRectangle(&im, 0, 0, 128, 14, (Color){255,255,255,255});         // polar sheen
        ImageDrawRectangle(&im, 0, 14, 128, 8, (Color){246,242,236,255});
        gTex[TX_STREAK] = makeTex(im, true);
    }
    { // pipe gloss: vertical brightness bands (tinted pipe-green)
        Image im = GenImageColor(64,64,WHITE);
        for (int x=0;x<64;x++){
            float k = sinf(x/64.0f*2.0f*PI);
            unsigned char v = (unsigned char)clampf(226 + 28*k, 200, 255);
            ImageDrawRectangle(&im, x, 0, 1, 64, (Color){v,v,v,255});
        }
        ImageDrawRectangle(&im, 10, 0, 5, 64, WHITE);                             // hot highlight
        gTex[TX_PIPEG] = makeTex(im, true);
    }
    { // sandstone: warm speckle + faint wind-ripple lines
        Image im = GenImageColor(128,128,(Color){214,190,150,255});
        for (int i=0;i<420;i++){
            Color p = (i%3)? (Color){202,176,134,255} : (Color){226,204,166,255};
            ImageDrawRectangle(&im, GetRandomValue(0,126), GetRandomValue(0,126), 2, 2, p);
        }
        for (int r=0;r<6;r++)
            ImageDrawRectangle(&im, 0, 12+r*20+GetRandomValue(-3,3), 128, 2, (Color){206,180,140,255});
        gTex[TX_SAND] = makeTex(im, true);
    }
    { // sky-stone: pale cloud-marble, cool blue-white blocks with soft veins
        Image im = GenImageColor(128,128,(Color){222,232,244,255});
        for (int row=0; row<6; row++){
            int off = (row%2)*42;
            for (int cx=-42; cx<128; cx+=42){
                int v = GetRandomValue(0,14);
                Color blk = { (unsigned char)(236-v),(unsigned char)(242-v),(unsigned char)(252-v),255 };
                ImageDrawRectangle(&im, cx+off+2, row*22+2, 40, 20, blk);
                ImageDrawRectangle(&im, cx+off+2, row*22+2, 40, 3, (Color){250,253,255,255});
            }
        }
        for (int i=0;i<40;i++)                               // faint cool veins
            ImageDrawRectangle(&im, GetRandomValue(0,120), GetRandomValue(0,124),
                               GetRandomValue(4,14), 1, (Color){198,214,236,255});
        gTex[TX_SKY] = makeTex(im, true);
    }
    { // canvas cloth: bold vertical stripes for banners & the sail
        Image im = GenImageColor(64,64,(Color){245,245,248,255});
        for (int s=0;s<4;s++)
            ImageDrawRectangle(&im, s*16, 0, 8, 64, (Color){228,228,236,255});
        for (int y=0;y<64;y+=6)                              // horizontal weave
            ImageDrawRectangle(&im, 0, y, 64, 1, (Color){214,214,224,255});
        gTex[TX_CLOTH] = makeTex(im, true);
    }
    { // ? block
        Image im = GenImageColor(128,128,C_GOLD);
        Color edge = {182,120,20,255}, riv = {120,74,12,255};
        ImageDrawRectangleLines(&im, (Rectangle){2,2,124,124}, 5, edge);
        ImageDrawText(&im, "?", 47, 24, 80, (Color){140,84,10,255});
        int p = 12;
        ImageDrawRectangle(&im, p,p,7,7, riv); ImageDrawRectangle(&im, 128-p-7,p,7,7, riv);
        ImageDrawRectangle(&im, p,128-p-7,7,7, riv); ImageDrawRectangle(&im, 128-p-7,128-p-7,7,7, riv);
        gTex[TX_Q] = makeTex(im, true);
    }
}

// ---------------------------------------------------------------- models ---
static Model makeModel(Mesh m){
    Model mdl = LoadModelFromMesh(m);
    mdl.materials[0].shader = gToon;
    return mdl;
}
static void LoadGfx(void){
    gToon = LoadShaderFromMemory(TOON_VS, TOON_FS);
    gLocSun  = GetShaderLocation(gToon, "sunDir");
    gLocCam  = GetShaderLocation(gToon, "camPos");
    gLocFogC = GetShaderLocation(gToon, "fogColor");
    gLocFogD = GetShaderLocation(gToon, "fogDensity");
    Vector3 sun = Vector3Normalize((Vector3){-0.45f,-1.0f,-0.35f});
    SetShaderValue(gToon, gLocSun, &sun, SHADER_UNIFORM_VEC3);
    float fogd = 0.0022f;                          // the tower must read from the meadow
    SetShaderValue(gToon, gLocFogD, &fogd, SHADER_UNIFORM_FLOAT);
    LoadTextures();
    gBakeMat = LoadMaterialDefault();
    gBakeMat.shader = gToon;
    gCube   = makeModel(GenMeshCube(1,1,1));
    gCyl    = makeModel(GenMeshCylinder(1,1,28));
    gSphere = makeModel(GenMeshSphere(1,14,22));
    gCone   = makeModel(GenMeshCone(1,1,24));
    gPlane  = makeModel(GenMeshPlane(1,1,1,1));
}
static void GfxFrame(Vector3 camPos){
    SetShaderValue(gToon, gLocCam, &camPos, SHADER_UNIFORM_VEC3);
    SkyAt(camPos.y);                              // the sky climbs with you
    float fogc[3] = { gSkyFog.r/255.0f, gSkyFog.g/255.0f, gSkyFog.b/255.0f };
    SetShaderValue(gToon, gLocFogC, fogc, SHADER_UNIFORM_VEC3);
}

// one generic toon draw: model, position, scale, single-axis rotation, tex+tint
static void drawM(Model& m, Vector3 pos, Vector3 scale, Color col, int tex,
                  Vector3 rotAxis = {0,1,0}, float rotDeg = 0){
    m.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = gTex[tex];
    DrawModelEx(m, pos, rotAxis, rotDeg, scale, col);
}
// cylinder between two arbitrary points (for the pole)
static void drawCylBetween(Vector3 a, Vector3 b, float rad, Color col){
    Vector3 d = b - a; float len = Vector3Length(d);
    if (len < 0.0001f) return;
    Vector3 dir = d * (1.0f/len);
    Vector3 axis = Vector3CrossProduct((Vector3){0,1,0}, dir);
    float ang = RAD2DEG*acosf(clampf(Vector3DotProduct((Vector3){0,1,0}, dir), -1, 1));
    if (Vector3Length(axis) < 0.0001f) axis = (Vector3){1,0,0};
    drawM(gCyl, a, {rad,len,rad}, col, TX_WHITE, axis, ang);
}
// a cone from `base` pointing along `dir` (petals, spikes)
static void drawConeTo(Vector3 base, Vector3 dir, float len, float rad, Color col, int tex=TX_WHITE){
    float dl = Vector3Length(dir);
    if (dl < 0.0001f) return;
    Vector3 d = dir*(1.0f/dl);
    Vector3 axis = Vector3CrossProduct((Vector3){0,1,0}, d);
    float ang = RAD2DEG*acosf(clampf(d.y, -1, 1));
    if (Vector3Length(axis) < 0.0001f) axis = (Vector3){1,0,0};
    drawM(gCone, base, {rad, len, rad}, col, tex, axis, ang);
}

// ------------------------------------------------------------ world draw ---
static void DrawSolid(const Solid& s, int idx){
    if (!s.visible) return;
    Color tint = WHITE; int tex = TX_WHITE;
    switch (s.surf){
        case S_GRASS:  tex = TX_GRASS; break;
        case S_STONE:
            if (s.isCyl){ tex = TX_STONE; tint = ctint(WHITE,0.99f); }   // masonry wraps the towers
            else { tex = TX_STONE; tint = (idx&1)? WHITE : ctint(WHITE,0.96f); }
            break;
        case S_BRICK:  tex = TX_BRICK; break;
        case S_QBLOCK:                             // spent blocks go dull brown
            if (s.used){ tex = TX_BRICK; tint = (Color){168,120,74,255}; }
            else tex = TX_Q;
            break;
        case S_WOOD:   tex = TX_WOOD;  break;
        case S_PIPE:   tex = TX_PIPEG; tint = C_PIPE; break;
        case S_SAND:   tex = TX_SAND;  break;
        case S_SKYSTONE: tex = TX_SKY; break;
        case S_GOLD:   tint = C_GOLD;  break;
        case S_DARK:   tint = {70,60,55,255}; break;
        case S_CLOUD:  tint = C_CLOUD; break;
        default: break;
    }
    if (s.isCyl){
        drawM(gCyl, s.base, {s.rad, s.hgt, s.rad}, tint, tex);
    } else {
        Vector3 c = (s.mn + s.mx)*0.5f, sz = s.mx - s.mn;
        drawM(gCube, c, sz, tint, tex);
    }
}
static void DrawDecorItem(const Decor& d){
    switch (d.kind){
        case D_SPHERE: drawM(gSphere, d.pos, d.scale, d.col, d.tex, {0,1,0}, d.rotY); break;
        case D_CUBE:   drawM(gCube,   d.pos, d.scale, d.col, d.tex, {0,1,0}, d.rotY); break;
        case D_CYL:    drawM(gCyl,    d.pos, d.scale, d.col, d.tex, {0,1,0}, d.rotY); break;
        case D_CONE:   drawM(gCone,   d.pos, d.scale, d.col, d.tex, {0,1,0}, d.rotY); break;
    }
}
// a luminous web-orchid: bright petals, a glowing core, dangling silk. Big and
// unmistakable so the player always sees where the next swing point is.
static void DrawWebAnchors(Vector3 camPos){
    float t = (float)GetTime();
    for (const auto& a : gWebAnchors){
        float dist = Vector3Distance(a.pos, camPos);
        if (dist > 260) continue;
        if (a.wilt > 0){                              // spent: a shrivelled grey sac, regrowing
            float k = 1.0f - a.wilt/WEB_WILT;
            drawM(gSphere, a.pos, {0.16f+0.12f*k,0.20f+0.10f*k,0.16f+0.12f*k}, (Color){120,108,108,235}, TX_STREAK);
            for (int s=0;s<3;s++){ float ang=s*2.09f;
                drawM(gSphere, a.pos+(Vector3){cosf(ang)*0.18f,-0.5f,sinf(ang)*0.18f}, {0.06f,0.10f,0.06f},
                      (Color){108,98,98,220}, TX_WHITE); }
            continue;
        }
        bool near = dist < WEB_RANGE && a.pos.y > camPos.y - 4.0f;
        float beat = near? 0.6f+0.4f*sinf(t*8.0f) : 0.8f+0.2f*sinf(t*2.8f);
        float spin = t*0.5f + a.pos.x*0.3f;
        // a fat sticky sap-pod that oozes the red webbing. Long pointed petals
        // give it a real flower shape; opaque saturated crimson, not a glow orb.
        // 8 long pointed petals splaying out and up (the dominant silhouette)
        for (int p=0;p<8;p++){
            float ang = spin + p*0.785f;
            Vector3 dir = { cosf(ang), 0.34f + 0.12f*sinf(t*2.0f+p), sinf(ang) };
            Color pc = (p%2)? (Color){222,28,40,255} : (Color){248,64,118,255};   // crimson / hot magenta
            drawConeTo(a.pos, dir, 0.66f, 0.185f, pc, TX_STREAK);
        }
        // 5 shorter inner petals, more upright, brighter
        for (int p=0;p<5;p++){
            float ang = -spin*0.8f + p*1.257f + 0.4f;
            Vector3 dir = { cosf(ang)*0.62f, 0.9f, sinf(ang)*0.62f };
            drawConeTo(a.pos+(Vector3){0,0.04f,0}, dir, 0.40f, 0.15f, (Color){255,120,95,255}, TX_STREAK);
        }
        // small tight inner glow (not a big washy orb)
        drawM(gSphere, a.pos, {0.34f*beat+0.16f, 0.30f*beat+0.14f, 0.34f*beat+0.16f}, (Color){255,70,80,70}, TX_WHITE);
        // glossy sap bulb + luminous core + a hot specular glint
        drawM(gSphere, a.pos, {0.27f,0.30f,0.27f}, (Color){196,26,40,255}, TX_STREAK);
        drawM(gSphere, a.pos+(Vector3){0,0.10f,0}, {0.14f+0.06f*beat,0.14f+0.06f*beat,0.14f+0.06f*beat},
              (Color){255,225,130,255}, TX_WHITE);
        drawM(gSphere, a.pos+(Vector3){-0.07f,0.13f,-0.05f}, {0.045f,0.045f,0.045f}, WHITE, TX_WHITE);
        // dripping sticky sap tendrils with fat glossy droplets
        for (int s=0;s<3;s++){
            float ang = t*0.3f + s*2.09f;
            float droop = 0.95f + 0.3f*sinf(t*1.5f+s);
            Vector3 tip = a.pos + (Vector3){cosf(ang)*0.28f, -droop, sinf(ang)*0.28f};
            drawCylBetween(a.pos+(Vector3){0,-0.14f,0}, tip, 0.04f, (Color){205,36,46,255});
            drawM(gSphere, tip, {0.10f,0.13f,0.10f}, (Color){225,46,58,255}, TX_STREAK);
            drawM(gSphere, tip+(Vector3){-0.03f,0.03f,0}, {0.03f,0.03f,0.03f}, (Color){255,175,175,220}, TX_WHITE);
        }
        // upward wisp so it reads from far; brighter when grabbable
        DrawCylinder((Vector3){a.pos.x, a.pos.y+5.0f, a.pos.z}, 0.06f, 0.24f, 8, 8,
                     (Color){255,80,80,(unsigned char)(60+90*(near?beat:0.4f))});
        if (near){                                    // grab-ready twin halo
            for (int r=0;r<2;r++)
                DrawCircle3D(a.pos, 0.62f+r*0.15f+0.08f*sinf(t*6.0f), (Vector3){1,0,0}, 90.0f,
                             (Color){255,120,120,175});
        }
        if (gUnlockT > 2.5f)                          // the unlock: every bloom flares awake
            DrawCylinder((Vector3){a.pos.x, a.pos.y+16, a.pos.z}, 0.4f, 0.9f, 30, 10,
                         (Color){255,80,80,(unsigned char)(90*(gUnlockT-2.5f)/2.0f)});
    }
}
// ---- static-decor baking -------------------------------------------------
// The ascent carries ~4000 decor items; per-item DrawModelEx is ~3000 draw
// calls and the frame dies. Everything opaque and non-landmark gets baked
// into merged meshes (bucketed by 90 m height band + primitive + texture) and
// drawn in a handful of DrawMesh calls. Alpha decor, landmarks (big >= 40)
// and everything during editor mode stay on the per-item path.
static bool BakeSkip(const Decor& d){         // stays on the dynamic path?
    float big = fmaxf(d.scale.x, fmaxf(d.scale.y, d.scale.z));
    return d.col.a < 255 || big >= 40.0f;
}
static void FreeBaked(void){
    for (auto& b : gBaked) UnloadMesh(b.mesh);
    gBaked.clear();
}
static void BakeWorldDecor(void){
    FreeBaked();
    // CPU-side templates. NOTE: raylib de-indexes par_shapes meshes, so the
    // sphere/cyl/cone have indices == NULL (triangle soup); only the cube is
    // indexed. The bake emits soup uniformly and handles both.
    Mesh tm[4] = {};
    tm[D_SPHERE] = GenMeshSphere(1, 8, 12);    // blobs; cel shading hides the budget
    tm[D_CUBE]   = GenMeshCube(1,1,1);
    tm[D_CYL]    = GenMeshCylinder(1,1,14);
    tm[D_CONE]   = GenMeshCone(1,1,14);
    int tvc[4];                                 // soup verts per template
    for (int k=0;k<4;k++) tvc[k] = tm[k].triangleCount*3;
    // bucket items by (yband, kind, tex)
    struct Key { int band, kind, tex; };
    std::vector<std::vector<int>> groups;
    std::vector<Key> keys;
    for (int i=0;i<(int)decor.size();i++){
        const Decor& d = decor[i];
        if (BakeSkip(d)) continue;
        Key k = { (int)floorf(d.pos.y/90.0f), d.kind, d.tex };
        int g = -1;
        for (int j=0;j<(int)keys.size();j++)
            if (keys[j].band==k.band && keys[j].kind==k.kind && keys[j].tex==k.tex){ g=j; break; }
        if (g < 0){ keys.push_back(k); groups.push_back({}); g = (int)keys.size()-1; }
        groups[g].push_back(i);
    }
    for (int g=0; g<(int)groups.size(); g++){
        const Mesh& src = tm[keys[g].kind];
        int per = tvc[keys[g].kind];
        size_t at = 0;
        while (at < groups[g].size()){
            int fit = (int)fminf((float)(groups[g].size()-at), fmaxf(1.0f, floorf(90000.0f/per)));
            Mesh m{};
            m.vertexCount   = fit*per;
            m.triangleCount = fit*src.triangleCount;
            m.vertices  = (float*)RL_MALLOC(m.vertexCount*3*sizeof(float));
            m.normals   = (float*)RL_MALLOC(m.vertexCount*3*sizeof(float));
            m.texcoords = (float*)RL_MALLOC(m.vertexCount*2*sizeof(float));
            m.colors    = (unsigned char*)RL_MALLOC(m.vertexCount*4);
            Vector3 mn = { 1e9f, 1e9f, 1e9f}, mx = {-1e9f,-1e9f,-1e9f};
            for (int q=0; q<fit; q++){
                const Decor& d = decor[groups[g][at+q]];
                Matrix M = MatrixMultiply(MatrixMultiply(
                               MatrixScale(d.scale.x, d.scale.y, d.scale.z),
                               MatrixRotate((Vector3){0,1,0}, d.rotY*DEG2RAD)),
                               MatrixTranslate(d.pos.x, d.pos.y, d.pos.z));
                Matrix N = MatrixTranspose(MatrixInvert(M));
                int v0 = q*per;
                for (int j=0; j<per; j++){
                    int sv = src.indices? src.indices[j] : j;
                    Vector3 p = { src.vertices[sv*3], src.vertices[sv*3+1], src.vertices[sv*3+2] };
                    p = Vector3Transform(p, M);
                    m.vertices[(v0+j)*3] = p.x; m.vertices[(v0+j)*3+1] = p.y; m.vertices[(v0+j)*3+2] = p.z;
                    Vector3 nr = { src.normals[sv*3], src.normals[sv*3+1], src.normals[sv*3+2] };
                    nr = Vector3Normalize((Vector3){
                        N.m0*nr.x + N.m4*nr.y + N.m8*nr.z,
                        N.m1*nr.x + N.m5*nr.y + N.m9*nr.z,
                        N.m2*nr.x + N.m6*nr.y + N.m10*nr.z });
                    m.normals[(v0+j)*3] = nr.x; m.normals[(v0+j)*3+1] = nr.y; m.normals[(v0+j)*3+2] = nr.z;
                    m.texcoords[(v0+j)*2] = src.texcoords[sv*2]; m.texcoords[(v0+j)*2+1] = src.texcoords[sv*2+1];
                    m.colors[(v0+j)*4] = d.col.r; m.colors[(v0+j)*4+1] = d.col.g;
                    m.colors[(v0+j)*4+2] = d.col.b; m.colors[(v0+j)*4+3] = 255;
                    mn.x = fminf(mn.x, p.x); mn.y = fminf(mn.y, p.y); mn.z = fminf(mn.z, p.z);
                    mx.x = fmaxf(mx.x, p.x); mx.y = fmaxf(mx.y, p.y); mx.z = fmaxf(mx.z, p.z);
                }
            }
            UploadMesh(&m, false);
            // GPU owns it now - drop the CPU copies (UnloadMesh handles NULLs)
            RL_FREE(m.vertices);  m.vertices  = NULL;
            RL_FREE(m.normals);   m.normals   = NULL;
            RL_FREE(m.texcoords); m.texcoords = NULL;
            RL_FREE(m.colors);    m.colors    = NULL;
            gBaked.push_back({ m, keys[g].tex, (mn+mx)*0.5f, Vector3Length(mx-mn)*0.5f });
            at += fit;
        }
    }
    for (int k=0;k<4;k++) UnloadMesh(tm[k]);
}

static void DrawWorld3D(Vector3 camPos, Vector3 camFwd){
    if (gBakeDirty && !gEditMode){ BakeWorldDecor(); gBakeDirty = false; }
    // the ascent draws six worlds at once - cheap behind-the-camera rejection
    // (angle > ~100 deg off-axis; fov is 74-90) carries most of the load
    for (size_t i=0;i<solids.size();i++){
        const Solid& s = solids[i];
        if (!s.visible) continue;
        Vector3 c = (s.mn+s.mx)*0.5f;
        Vector3 to = c - camPos;
        float d = Vector3Length(to);
        float big = Vector3Length(s.mx - s.mn);
        if (d > 320 && big < 60) continue;
        if (big < 60 && d > 30 &&
            to.x*camFwd.x + to.y*camFwd.y + to.z*camFwd.z < -0.2f*d) continue;
        DrawSolid(s, (int)i);
    }
    if (!gEditMode)
        for (auto& b : gBaked){
            Vector3 to = b.c - camPos;
            float d = Vector3Length(to);
            if (d - b.rad > 430) continue;
            if (d > b.rad*1.4f + 40 &&
                to.x*camFwd.x + to.y*camFwd.y + to.z*camFwd.z < -0.25f*d) continue;
            gBakeMat.maps[MATERIAL_MAP_DIFFUSE].texture = gTex[b.tex];
            DrawMesh(b.mesh, gBakeMat, MatrixIdentity());
        }
    for (const Decor& d : decor){
        if (!gEditMode && !BakeSkip(d)) continue;        // already in a baked chunk
        Vector3 to = d.pos - camPos;
        float dist = Vector3Length(to);
        float big = fmaxf(d.scale.x, fmaxf(d.scale.y, d.scale.z));
        if (big < 1.0f && dist > 150) continue;
        if (big < 6.0f && dist > 270) continue;
        if (dist > 420 && big < 40) continue;
        if (big < 60 && dist > 30 &&
            to.x*camFwd.x + to.y*camFwd.y + to.z*camFwd.z < -0.2f*dist) continue;
        DrawDecorItem(d);
    }
}

// --------------------------------------------------------- sky decoration --
static struct { Vector3 p; float s, spd; } gSkyClouds[20];
static Vector3 gStarDirs[110];                 // fixed star dome (unit directions)
static void InitSky(void){
    for (int i=0;i<20;i++){
        gSkyClouds[i].p = { frnd(-260,340), frnd(40,640), frnd(-40,760) };
        gSkyClouds[i].s = frnd(6,14);
        gSkyClouds[i].spd = frnd(0.6f,1.8f);
    }
    for (int i=0;i<110;i++){
        float az = frnd(0, 6.283f), el = frnd(0.06f, 1.45f);
        gStarDirs[i] = { cosf(az)*cosf(el), sinf(el), sinf(az)*cosf(el) };
    }
}
static void DrawSkyBits(float t, Vector3 camPos){
    float y = camPos.y;
    // the sun rules the lower sky, then sinks away into the dusk
    float sunA = clampf(1.0f - (y-330.0f)/160.0f, 0.0f, 1.0f);
    if (sunA > 0.01f){
        DrawSphere((Vector3){170,210,-120}, 16, (Color){255,244,180,(unsigned char)(255*sunA)});
        DrawSphere((Vector3){170,210,-120}, 13, (Color){255,252,220,(unsigned char)(255*sunA)});
    }
    // the moon owns the heights (camera-anchored: a true celestial)
    float moonA = clampf((y-380.0f)/140.0f, 0.0f, 1.0f);
    if (moonA > 0.01f){
        Vector3 mp = camPos + (Vector3){-330, 240, 260};
        DrawSphere(mp, 26, (Color){236,240,252,(unsigned char)(235*moonA)});
        DrawSphere(mp + (Vector3){9,7,-6}, 23, (Color){178,182,214,(unsigned char)(200*moonA)});   // the shadowed cheek
    }
    // stars prick through as the blue thins; they breathe a little
    float starA = clampf((y-400.0f)/170.0f, 0.0f, 1.0f);
    if (starA > 0.01f)
        for (int i=0;i<110;i++){
            float tw = 0.65f + 0.35f*sinf(t*1.7f + i*2.09f);
            DrawSphereEx(camPos + gStarDirs[i]*760.0f, 1.5f + 0.6f*tw, 4, 6,
                         (Color){255,255,244,(unsigned char)(215*starA*tw)});
        }
    for (auto& c : gSkyClouds){
        c.p.x += c.spd * GetFrameTime();
        if (c.p.x > 360) c.p.x = -360;
        Color cc = (c.p.y > 500)? (Color){224,212,238,255} : WHITE;   // twilight-lit up high
        DrawSphereEx(c.p, c.s*0.5f, 7, 10, cc);
        DrawSphereEx((Vector3){c.p.x-c.s*0.35f, c.p.y-c.s*0.06f, c.p.z+c.s*0.1f}, c.s*0.34f, 7, 10, cc);
        DrawSphereEx((Vector3){c.p.x+c.s*0.36f, c.p.y-c.s*0.05f, c.p.z-c.s*0.1f}, c.s*0.36f, 7, 10, cc);
    }
}

// ------------------------------------------------------------- the star ----
static void DrawStar(float t, bool won){
    Vector3 sp = gStarP; sp.y += sinf(t*1.6f)*0.35f;
    float spin = t*60.0f*DEG2RAD * (won? 6.0f : 1.0f);
    Vector3 u = { cosf(spin), 0, sinf(spin) };          // star plane right-vector
    Vector3 n = { -sinf(spin), 0, cosf(spin) };         // plane normal
    drawM(gSphere, sp, {0.72f,0.72f,0.34f}, C_GOLD, TX_WHITE, {0,1,0}, -spin*RAD2DEG);
    for (int k=0;k<5;k++){
        float a = (90.0f + k*72.0f)*DEG2RAD;            // 5 points, one straight up
        Vector3 dir = u*cosf(a) + (Vector3){0,sinf(a),0};
        Vector3 axis = Vector3CrossProduct((Vector3){0,1,0}, dir);
        float ang = RAD2DEG*acosf(clampf(dir.y,-1,1));
        if (Vector3Length(axis)<0.001f) axis = {1,0,0};
        drawM(gCone, sp + dir*0.42f, {0.44f,1.35f,0.44f}, C_GOLD, TX_WHITE, axis, ang);
    }
    for (int side=-1; side<=1; side+=2){                 // little eyes, both faces
        Vector3 off = n*(0.56f*side);
        drawM(gSphere, sp + off + u*0.22f + (Vector3){0,0.12f,0}, {0.09f,0.16f,0.06f}, BLACK, TX_WHITE);
        drawM(gSphere, sp + off + u*(-0.22f) + (Vector3){0,0.12f,0}, {0.09f,0.16f,0.06f}, BLACK, TX_WHITE);
    }
}
static void DrawGoalBeam(float t){
    float pulse = 0.75f + 0.25f*sinf(t*2.2f);
    float base = gStarP.y - 3.7f;
    DrawCylinder((Vector3){gStarP.x, base, gStarP.z}, 2.0f*pulse, 2.3f*pulse, 320, 24,
                 (Color){255,196,50,(unsigned char)(58*pulse)});
    DrawCylinder((Vector3){gStarP.x, base, gStarP.z}, 0.7f, 0.85f, 320, 16,
                 (Color){255,232,140,(unsigned char)(78*pulse)});
}

// ---------------------------------------------------------- blob shadow ----
static float GroundTopBelow(Vector3 p){
    float feet = p.y - PLAYER_HH;
    float best = -1000;
    for (const Solid& s : solids){
        if (s.isCyl){
            float dx = p.x - s.base.x, dz = p.z - s.base.z;
            if (dx*dx + dz*dz > (s.rad+0.15f)*(s.rad+0.15f)) continue;
            float top = s.base.y + s.hgt;
            if (top <= feet + 0.05f && top > best) best = top;
        } else {
            if (p.x < s.mn.x-0.15f || p.x > s.mx.x+0.15f || p.z < s.mn.z-0.15f || p.z > s.mx.z+0.15f) continue;
            if (s.mx.y <= feet + 0.05f && s.mx.y > best) best = s.mx.y;
        }
    }
    return best;
}
static void DrawBlobShadow(Vector3 p){
    float top = GroundTopBelow(p);
    if (top < -900) return;
    float h = (p.y - PLAYER_HH) - top;
    float a = clampf(1.0f - h/45.0f, 0.15f, 1.0f);
    float r = lerpf(0.55f, 0.3f, clampf(h/45.0f,0,1));
    drawM(gCyl, {p.x, top+0.03f, p.z}, {r, 0.02f, r}, (Color){20,30,15,(unsigned char)(110*a)}, TX_WHITE);
}
