// ============================================================================
// level_io.h : LEVEL FILE save/load for the in-game editor (F4)
//
// A level saved from the editor lands in  levels/levelN.txt  (next to the exe)
// and from then on REPLACES the code builder for that level - BuildWorld loads
// the file instead. Delete the file to return to the code-built version.
// The format is plain text, one entity per line, hand-editable.
// ============================================================================
#pragma once
#include "game.h"
#include "world.h"
#include <cstdio>

static void LevelFilePath(char* out, int n, int level){
    snprintf(out, n, "%slevels/level%d.txt", GetApplicationDirectory(), level);
}

// ------------------------------------------------------------------ save ---
static bool SaveLevelFileTo(const char* path){
    FILE* f = fopen(path, "w");
    if (!f) return false;
    fprintf(f, "shroomvault-level 1\n");
    fprintf(f, "grpmax %d\n", gGrp);
    fprintf(f, "spawn %.3f %.3f %.3f\n", gSpawn.x, gSpawn.y, gSpawn.z);
    fprintf(f, "star %.3f %.3f %.3f\n",  gStarP.x, gStarP.y, gStarP.z);
    fprintf(f, "wind %.3f %.3f %.3f\n",  gAirWind.x, gAirWind.y, gAirWind.z);
    fprintf(f, "warptop %d %.3f %.3f %.3f\n", gWarpGrp, gWarpTop.x, gWarpTop.y, gWarpTop.z);
    for (const Solid& s : solids){
        if (s.isCyl)
            fprintf(f, "cyl %d %d %d %d %.3f %.3f %.3f %.3f %.3f\n",
                    s.grp, s.surf, s.bouncy?1:0, s.visible?1:0,
                    s.base.x, s.base.y, s.base.z, s.rad, s.hgt);
        else
            fprintf(f, "box %d %d %d %d %.3f %.3f %.3f %.3f %.3f %.3f\n",
                    s.grp, s.surf, s.bouncy?1:0, s.visible?1:0,
                    s.mn.x, s.mn.y, s.mn.z, s.mx.x, s.mx.y, s.mx.z);
    }
    for (const Decor& d : decor)
        fprintf(f, "dec %d %d %d %.3f %.3f %.3f %.3f %.3f %.3f %.3f %d %d %d %d\n",
                d.grp, d.kind, d.tex, d.rotY,
                d.pos.x, d.pos.y, d.pos.z, d.scale.x, d.scale.y, d.scale.z,
                d.col.r, d.col.g, d.col.b, d.col.a);
    for (const Coin& c : gCoins)
        fprintf(f, "coin %d %.3f %.3f %.3f\n", c.grp, c.p.x, c.p.y, c.p.z);
    for (const WebAnchor& a : gWebAnchors)
        fprintf(f, "web %d %.3f %.3f %.3f %.3f\n", a.grp, a.pos.x, a.pos.y, a.pos.z, a.radius);
    for (const Spore& s : gSpores)
        fprintf(f, "spore %d %.3f %.3f %.3f\n", s.grp, s.p.x, s.p.y, s.p.z);
    for (const Shrine& s : gShrines)
        fprintf(f, "shrine %d %d %.3f %.3f %.3f\n", s.grp, s.type, s.p.x, s.p.y, s.p.z);
    for (const Updraft& u : gUpdrafts)
        fprintf(f, "updraft %d %.3f %.3f %.3f %.3f %.3f %.3f\n",
                u.grp, u.base.x, u.base.y, u.base.z, u.rad, u.hgt, u.str);
    for (const Windmill& w : gWindmills)
        fprintf(f, "mill %d %d %d %.3f %.3f %.3f %.3f %.4f %.3f %d %d %d %d\n",
                w.grp, w.blades, w.hazard, w.pos.x, w.pos.y, w.pos.z,
                w.rad, w.tilt, w.spd, w.col.r, w.col.g, w.col.b, w.col.a);
    for (const Banner& b : gBanners)
        fprintf(f, "banner %d %.3f %.3f %.3f %.3f %.3f %.4f %d %d %d %d\n",
                b.grp, b.top.x, b.top.y, b.top.z, b.len, b.w, b.phase,
                b.col.r, b.col.g, b.col.b, b.col.a);
    for (const Pinwheel& p : gPinwheels)
        fprintf(f, "pin %d %.3f %.3f %.3f %.3f %.3f %d %d %d %d %d %d %d %d\n",
                p.grp, p.pos.x, p.pos.y, p.pos.z, p.rad, p.spd,
                p.a.r, p.a.g, p.a.b, p.a.a, p.b.r, p.b.g, p.b.b, p.b.a);
    for (const Mote& m : gMotes)
        fprintf(f, "mote %d %.3f %.3f %.3f %.3f %.3f %d %d %d %d\n",
                m.grp, m.p.x, m.p.y, m.p.z, m.r, m.spd, m.c.r, m.c.g, m.c.b, m.c.a);
    for (const Thunder& th : gThunders)
        fprintf(f, "thun %d %d %.3f %.3f %.3f\n", th.grp, th.amount, th.p.x, th.p.y, th.p.z);
    fclose(f);
    return true;
}
static bool SaveLevelFile(int level){
    char dir[600]; snprintf(dir, sizeof(dir), "%slevels", GetApplicationDirectory());
    if (!DirectoryExists(dir)) MakeDirectory(dir);
    char path[600]; LevelFilePath(path, sizeof(path), level);
    return SaveLevelFileTo(path);
}

// ------------------------------------------------------------------ load ---
// Assumes the caller (BuildWorld) already cleared every entity vector.
static bool LoadLevelFileFrom(const char* path){
    FILE* f = fopen(path, "r");
    if (!f) return false;
    char line[512], tag[32];
    int grpMax = 0;
    while (fgets(line, sizeof(line), f)){
        if (sscanf(line, "%31s", tag) != 1) continue;
        if (!strcmp(tag, "grpmax")){ sscanf(line, "grpmax %d", &grpMax); }
        else if (!strcmp(tag, "spawn")) sscanf(line, "spawn %f %f %f", &gSpawn.x, &gSpawn.y, &gSpawn.z);
        else if (!strcmp(tag, "star"))  sscanf(line, "star %f %f %f",  &gStarP.x, &gStarP.y, &gStarP.z);
        else if (!strcmp(tag, "wind"))  sscanf(line, "wind %f %f %f",  &gAirWind.x, &gAirWind.y, &gAirWind.z);
        else if (!strcmp(tag, "warptop"))
            sscanf(line, "warptop %d %f %f %f", &gWarpGrp, &gWarpTop.x, &gWarpTop.y, &gWarpTop.z);
        else if (!strcmp(tag, "box")){
            Solid s{}; int b=0, v=1;
            sscanf(line, "box %d %d %d %d %f %f %f %f %f %f",
                   &s.grp, &s.surf, &b, &v, &s.mn.x, &s.mn.y, &s.mn.z, &s.mx.x, &s.mx.y, &s.mx.z);
            s.isCyl=false; s.bouncy=b!=0; s.visible=v!=0;
            solids.push_back(s);
        }
        else if (!strcmp(tag, "cyl")){
            Solid s{}; int b=0, v=1;
            sscanf(line, "cyl %d %d %d %d %f %f %f %f %f",
                   &s.grp, &s.surf, &b, &v, &s.base.x, &s.base.y, &s.base.z, &s.rad, &s.hgt);
            s.isCyl=true; s.bouncy=b!=0; s.visible=v!=0;
            s.mn={s.base.x-s.rad, s.base.y, s.base.z-s.rad};
            s.mx={s.base.x+s.rad, s.base.y+s.hgt, s.base.z+s.rad};
            solids.push_back(s);
        }
        else if (!strcmp(tag, "dec")){
            Decor d{}; int r,g,b,a;
            sscanf(line, "dec %d %d %d %f %f %f %f %f %f %f %d %d %d %d",
                   &d.grp, &d.kind, &d.tex, &d.rotY,
                   &d.pos.x, &d.pos.y, &d.pos.z, &d.scale.x, &d.scale.y, &d.scale.z,
                   &r, &g, &b, &a);
            d.col = {(unsigned char)r,(unsigned char)g,(unsigned char)b,(unsigned char)a};
            decor.push_back(d);
        }
        else if (!strcmp(tag, "coin")){
            Coin c{}; sscanf(line, "coin %d %f %f %f", &c.grp, &c.p.x, &c.p.y, &c.p.z);
            gCoins.push_back(c);
        }
        else if (!strcmp(tag, "web")){
            WebAnchor a{}; sscanf(line, "web %d %f %f %f %f", &a.grp, &a.pos.x, &a.pos.y, &a.pos.z, &a.radius);
            gWebAnchors.push_back(a);
        }
        else if (!strcmp(tag, "spore")){
            Spore s{}; sscanf(line, "spore %d %f %f %f", &s.grp, &s.p.x, &s.p.y, &s.p.z);
            gSpores.push_back(s);
        }
        else if (!strcmp(tag, "shrine")){
            Shrine s{}; sscanf(line, "shrine %d %d %f %f %f", &s.grp, &s.type, &s.p.x, &s.p.y, &s.p.z);
            gShrines.push_back(s);
        }
        else if (!strcmp(tag, "updraft")){
            Updraft u{};
            sscanf(line, "updraft %d %f %f %f %f %f %f",
                   &u.grp, &u.base.x, &u.base.y, &u.base.z, &u.rad, &u.hgt, &u.str);
            gUpdrafts.push_back(u);
        }
        else if (!strcmp(tag, "mill")){
            Windmill w{}; int r,g,b,a;
            sscanf(line, "mill %d %d %d %f %f %f %f %f %f %d %d %d %d",
                   &w.grp, &w.blades, &w.hazard, &w.pos.x, &w.pos.y, &w.pos.z,
                   &w.rad, &w.tilt, &w.spd, &r, &g, &b, &a);
            w.col = {(unsigned char)r,(unsigned char)g,(unsigned char)b,(unsigned char)a};
            gWindmills.push_back(w);
        }
        else if (!strcmp(tag, "banner")){
            Banner bn{}; int r,g,b,a;
            sscanf(line, "banner %d %f %f %f %f %f %f %d %d %d %d",
                   &bn.grp, &bn.top.x, &bn.top.y, &bn.top.z, &bn.len, &bn.w, &bn.phase,
                   &r, &g, &b, &a);
            bn.col = {(unsigned char)r,(unsigned char)g,(unsigned char)b,(unsigned char)a};
            gBanners.push_back(bn);
        }
        else if (!strcmp(tag, "pin")){
            Pinwheel p{}; int r,g,b,a, r2,g2,b2,a2;
            sscanf(line, "pin %d %f %f %f %f %f %d %d %d %d %d %d %d %d",
                   &p.grp, &p.pos.x, &p.pos.y, &p.pos.z, &p.rad, &p.spd,
                   &r, &g, &b, &a, &r2, &g2, &b2, &a2);
            p.a = {(unsigned char)r,(unsigned char)g,(unsigned char)b,(unsigned char)a};
            p.b = {(unsigned char)r2,(unsigned char)g2,(unsigned char)b2,(unsigned char)a2};
            gPinwheels.push_back(p);
        }
        else if (!strcmp(tag, "mote")){
            Mote m{}; int r,g,b,a;
            sscanf(line, "mote %d %f %f %f %f %f %d %d %d %d",
                   &m.grp, &m.p.x, &m.p.y, &m.p.z, &m.r, &m.spd, &r, &g, &b, &a);
            m.c = {(unsigned char)r,(unsigned char)g,(unsigned char)b,(unsigned char)a};
            gMotes.push_back(m);
        }
        else if (!strcmp(tag, "thun")){
            Thunder th{};
            sscanf(line, "thun %d %d %f %f %f", &th.grp, &th.amount, &th.p.x, &th.p.y, &th.p.z);
            gThunders.push_back(th);
        }
    }
    fclose(f);
    gGrp = grpMax;                 // new editor objects continue the id sequence
    return true;
}
static bool LoadLevelFile(int level){
    char path[600]; LevelFilePath(path, sizeof(path), level);
    if (!FileExists(path)) return false;
    return LoadLevelFileFrom(path);
}
