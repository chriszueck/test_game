// ============================================================================
// audio.h : all sounds synthesized at startup (no asset files)
// ============================================================================
#pragma once
#include "game.h"

static const int SR = 44100;
static bool gMuted = false;
static bool gAudioOK = false;

static Sound sBoing, sPlant, sWhoosh, sWhiff, sFoul, sTick, sDing,
             sLand, sStep, sPop, sWin;

// build a Sound from a float buffer (-1..1)
static Sound bakeSound(std::vector<float>& buf){
    std::vector<short> pcm(buf.size());
    for (size_t i=0;i<buf.size();i++) pcm[i] = (short)(clampf(buf[i],-1,1)*32000);
    Wave w{}; w.frameCount=(unsigned)buf.size(); w.sampleRate=SR; w.sampleSize=16; w.channels=1;
    w.data = pcm.data();
    return LoadSoundFromWave(w);   // copies data
}
static std::vector<float> mkbuf(float dur){ return std::vector<float>((size_t)(dur*SR), 0.0f); }

// sine sweep with exponential-ish decay envelope
static void addSweep(std::vector<float>& b, float t0, float dur, float f0, float f1,
                     float amp, float attack=0.008f){
    size_t i0=(size_t)(t0*SR), n=(size_t)(dur*SR);
    float ph=0;
    for (size_t i=0;i<n && i0+i<b.size();i++){
        float t=(float)i/n;
        float f=lerpf(f0,f1,t);
        ph += 2*PI*f/SR;
        float env = (t*dur<attack? (t*dur/attack) : powf(1.0f-t, 1.6f));
        b[i0+i] += sinf(ph)*amp*env;
    }
}
static void addNoise(std::vector<float>& b, float t0, float dur, float amp,
                     float lp=0.2f, bool swell=false){
    size_t i0=(size_t)(t0*SR), n=(size_t)(dur*SR);
    float y=0;
    for (size_t i=0;i<n && i0+i<b.size();i++){
        float t=(float)i/n;
        float w=frnd(-1,1);
        y += lp*(w-y);
        float env = swell? sinf(t*PI) : powf(1.0f-t,2.0f);
        b[i0+i] += y*amp*env;
    }
}

static void LoadAudioAll(void){
    InitAudioDevice();
    gAudioOK = IsAudioDeviceReady();
    if (!gAudioOK) return;
    { auto b=mkbuf(0.30f); addSweep(b,0,0.30f,150,430,0.55f); addSweep(b,0,0.22f,300,860,0.18f); sBoing=bakeSound(b); }
    { auto b=mkbuf(0.14f); addSweep(b,0,0.12f,110,60,0.7f);  addNoise(b,0,0.05f,0.5f,0.5f); sPlant=bakeSound(b); }
    { auto b=mkbuf(0.38f); addNoise(b,0,0.38f,0.65f,0.12f,true); addSweep(b,0,0.3f,180,520,0.10f); sWhoosh=bakeSound(b); }
    { auto b=mkbuf(0.16f); addNoise(b,0,0.16f,0.30f,0.25f,true); sWhiff=bakeSound(b); }
    { auto b=mkbuf(0.42f); addSweep(b,0,0.34f,330,140,0.5f); addSweep(b,0.05f,0.3f,166,70,0.3f);
      addNoise(b,0.3f,0.1f,0.4f,0.4f); sFoul=bakeSound(b); }
    { auto b=mkbuf(0.05f); addSweep(b,0,0.05f,950,900,0.35f); sTick=bakeSound(b); }
    { auto b=mkbuf(0.16f); addSweep(b,0,0.16f,1568,1568,0.30f); addSweep(b,0,0.12f,2093,2093,0.12f); sDing=bakeSound(b); }
    { auto b=mkbuf(0.09f); addSweep(b,0,0.09f,120,70,0.5f); addNoise(b,0,0.05f,0.35f,0.4f); sLand=bakeSound(b); }
    { auto b=mkbuf(0.05f); addNoise(b,0,0.05f,0.22f,0.35f); addSweep(b,0,0.04f,170,120,0.15f); sStep=bakeSound(b); }
    { auto b=mkbuf(0.07f); addSweep(b,0,0.07f,660,880,0.22f); sPop=bakeSound(b); }
    { auto b=mkbuf(1.5f);
      float notes[6]={523,659,784,1046,784,1318};
      for (int i=0;i<6;i++){ float t0=i*0.14f;
          addSweep(b,t0,0.5f,notes[i],notes[i],0.30f);
          addSweep(b,t0,0.35f,notes[i]*2,notes[i]*2,0.10f); }
      sWin=bakeSound(b); }
}
static void SND(Sound& s, float pitch=1.0f, float vol=1.0f){
    if (!gAudioOK || gMuted) return;
    SetSoundPitch(s, pitch); SetSoundVolume(s, vol); PlaySound(s);
}

// ------------------------------------------------------------- wind loop ---
static AudioStream gWind{};
static bool gWindOn=false;
static void InitWind(void){
    if (!gAudioOK) return;
    SetAudioStreamBufferSizeDefault(4096);
    gWind = LoadAudioStream(SR,16,1);
    PlayAudioStream(gWind);
    gWindOn = true;
}
static void UpdateWind(float speed){
    if (!gWindOn) return;
    static float lpy=0;
    static short buf[4096];
    while (IsAudioStreamProcessed(gWind)){
        for (int i=0;i<4096;i++){
            float w=frnd(-1,1); lpy += 0.08f*(w-lpy);
            buf[i]=(short)(lpy*24000);
        }
        UpdateAudioStream(gWind, buf, 4096);
    }
    float vol = gMuted? 0.0f : clampf((speed-13.0f)/38.0f, 0.0f, 0.85f);
    SetAudioStreamVolume(gWind, vol);
}
