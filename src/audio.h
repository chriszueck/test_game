// ============================================================================
// audio.h : all sounds synthesized at startup (no asset files)
// ============================================================================
#pragma once
#include "game.h"

static const int SR = 44100;
static bool gMuted = false;
static bool gAudioOK = false;

static Sound sBoing, sPlant, sWhoosh, sWhiff, sFoul, sTick, sDing,
             sLand, sStep, sPop, sWin, sPerfect, sThud;

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
// musical voice: fundamental + a couple of harmonics, exponential decay
static float mfreq(int m){ return 440.0f*powf(2.0f,(m-69)/12.0f); }
static void addTone(std::vector<float>& b, float t0, float dur, float f,
                    float amp, float bright=0.35f, float attack=0.012f){
    size_t i0=(size_t)(t0*SR), n=(size_t)(dur*SR);
    float ph=0;
    for (size_t i=0;i<n && i0+i<b.size();i++){
        float t=(float)i/n;
        ph += 2*PI*f/SR;
        float env = (t*dur<attack? t*dur/attack : powf(1.0f-t,2.2f));
        float s = sinf(ph) + bright*0.5f*sinf(ph*2) + bright*0.3f*sinf(ph*3);
        b[i0+i] += s*amp*env;
    }
}
static void addKick(std::vector<float>& b, float t0, float amp=0.30f){
    addSweep(b,t0,0.10f,95,42,amp);
}
static void addHat(std::vector<float>& b, float t0, float amp=0.07f){
    size_t i0=(size_t)(t0*SR), n=(size_t)(0.03f*SR);
    float py=0;
    for (size_t i=0;i<n && i0+i<b.size();i++){
        float w=frnd(-1,1); float hp=w-py; py=w;      // crude highpass
        b[i0+i] += hp*amp*powf(1.0f-(float)i/n,2.0f);
    }
}
static void addSnare(std::vector<float>& b, float t0, float amp=0.16f){
    addNoise(b,t0,0.09f,amp,0.55f);
    addSweep(b,t0,0.06f,220,150,amp*0.5f);
}

// ------------------------------------------------------- zone music --------
// five 16 s loops baked at startup, crossfaded by altitude:
//   [0] meadow stroll  [1] castle march  [2] sky drift
//   [3] canyon thunder (the mesa)        [4] twilight grove (the Bonewood)
static Sound gMusic[5];
static float gMusVol[5] = {0,0,0,0,0};
static float gMusT = 0; static bool gMusOn = false;
static const float MUSIC_LEN = 16.0f;          // 32 beats at 120 bpm

static void BakeMusic(void){
    const float B = 0.5f;                       // one beat
    { // MEADOW: bouncy I-IV-I-V tune, walking bass, light kick+hat
      auto b=mkbuf(MUSIC_LEN);
      int bass[32] = {48,55,48,55, 53,60,53,60, 48,55,48,55, 55,62,55,59,
                      48,55,48,55, 53,60,53,60, 43,50,55,59, 48,55,48, 0};
      for (int i=0;i<32;i++) if (bass[i]) addTone(b, i*B, 0.42f, mfreq(bass[i]-12), 0.17f, 0.2f);
      int mel[64] = {
        72,0,76,0, 79,0,76,0,  77,0,76,0, 74,0,72,0,
        72,0,76,0, 79,0,84,0,  83,0,79,0, 74,0, 0,0,
        72,0,76,0, 79,0,76,0,  81,0,79,0, 77,0,76,0,
        74,0,76,0, 79,0,83,0,  84,0, 0,0, 79,0,72,0 };
      for (int i=0;i<64;i++) if (mel[i]) addTone(b, i*B*0.5f, 0.34f, mfreq(mel[i]), 0.13f, 0.45f);
      for (int i=0;i<32;i++){ if (i%2==0) addKick(b, i*B, 0.22f); addHat(b, i*B + B*0.5f, 0.05f); }
      gMusic[0]=bakeSound(b); }
    { // CASTLE: minor-key march - firmer bass, snare on 2 & 4
      auto b=mkbuf(MUSIC_LEN);
      int roots[8] = {45,45,53,55, 45,53,52,45};
      for (int bar=0;bar<8;bar++) for (int q=0;q<4;q++)
          addTone(b, (bar*4+q)*B, 0.40f, mfreq(roots[bar]-12 + ((q%2)?7:0)), 0.18f, 0.25f);
      int mel[64] = {
        76,0, 0,76, 74,76,81,0,   0,79,76,0, 74,0,72,0,
        76,0, 0,76, 74,76,84,0,  83,0,81,0, 79,0,76,0,
        77,0,76,77, 81,0,77,0,   79,0,76,79, 83,0,79,0,
        81,0,80,0, 81,83,84,0,   81,0,76,0, 69,0, 0,0 };
      for (int i=0;i<64;i++) if (mel[i]) addTone(b, i*B*0.5f, 0.30f, mfreq(mel[i]), 0.12f, 0.55f);
      for (int i=0;i<32;i++){
          if (i%4==0) addKick(b, i*B, 0.26f);
          if (i%4==2) addSnare(b, i*B, 0.13f);
          addHat(b, i*B, 0.04f);
      }
      gMusic[1]=bakeSound(b); }
    { // SKY: slow dreamy arpeggios, long high tones, no drums
      auto b=mkbuf(MUSIC_LEN);
      int chords[4][4] = { {77,81,84,88}, {76,79,83,86}, {74,77,81,84}, {76,79,83,88} };
      for (int i=0;i<64;i++){
          int ch = (i/16)%4;
          int note = chords[ch][ i%2==0 ? (i/2)%4 : 3-((i/2)%4) ];
          addTone(b, i*B*0.5f, 0.55f, mfreq(note), 0.065f, 0.15f, 0.03f);
      }
      int longs[8] = {84, 0, 83, 0, 81, 0, 79, 88};
      for (int i=0;i<8;i++) if (longs[i]) addTone(b, i*4*B, 2.4f, mfreq(longs[i]), 0.085f, 0.1f, 0.25f);
      int pads[4] = {53, 48, 50, 52};
      for (int i=0;i<4;i++) addTone(b, i*8*B, 3.6f, mfreq(pads[i]-12), 0.09f, 0.08f, 0.5f);
      gMusic[2]=bakeSound(b); }
    { // CANYON: slow thunder - deep toms, an open-fifth drone, desert pentatonic
      auto b=mkbuf(MUSIC_LEN);
      int dro[4] = {45, 45, 43, 41};
      for (int i=0;i<4;i++){
          addTone(b, i*8*B, 3.8f, mfreq(dro[i]-24), 0.16f, 0.12f, 0.4f);
          addTone(b, i*8*B, 3.8f, mfreq(dro[i]-17), 0.10f, 0.10f, 0.5f);   // bare fifth above
      }
      int rif[32] = {69,0,72,0, 74,0,76,0,  74,72,0,69, 0,0,0,0,
                     69,0,72,0, 76,0,79,0,  76,0,74,72, 0,0,69,0};
      for (int i=0;i<32;i++) if (rif[i]) addTone(b, i*B, 0.5f, mfreq(rif[i]), 0.10f, 0.3f);
      for (int i=0;i<32;i++){
          if (i%4==0) addKick(b, i*B, 0.30f);
          if (i%8==6) addKick(b, i*B, 0.20f);
          if (i%8==7) addSnare(b, i*B, 0.08f);
      }
      gMusic[3]=bakeSound(b); }
    { // TWILIGHT GROVE: a music box among the graves - sparse, minor, no drums
      auto b=mkbuf(MUSIC_LEN);
      int arp[32] = {81,0,84,0, 88,0,84,0,  81,0,86,0, 84,0,81,0,
                     79,0,84,0, 88,0,91,0,  88,0,86,0, 84,0,0,0};
      for (int i=0;i<32;i++) if (arp[i]) addTone(b, i*B, 0.9f, mfreq(arp[i]), 0.075f, 0.55f, 0.004f);
      int pads[4] = {45, 41, 43, 45};
      for (int i=0;i<4;i++){
          addTone(b, i*8*B, 4.2f, mfreq(pads[i]-12), 0.075f, 0.06f, 0.8f);
          addTone(b, i*8*B+2*B, 3.6f, mfreq(pads[i]), 0.05f, 0.06f, 0.8f);
      }
      addTone(b, 14*B, 2.2f, mfreq(57), 0.09f, 0.08f, 0.02f);   // a far bell, twice a loop
      addTone(b, 30*B, 2.2f, mfreq(52), 0.09f, 0.08f, 0.02f);
      gMusic[4]=bakeSound(b); }
}
static void StartMusic(void){
    if (!gAudioOK) return;
    for (int i=0;i<5;i++){ SetSoundVolume(gMusic[i], 0); PlaySound(gMusic[i]); }
    gMusOn = true; gMusT = 0;
}
static void UpdateMusic(float dt, float alt, bool won){
    if (!gMusOn) return;
    gMusT += dt;
    if (gMusT >= MUSIC_LEN){                    // seamless-enough loop, phase-locked
        gMusT -= MUSIC_LEN;
        for (int i=0;i<5;i++) PlaySound(gMusic[i]);
    }
    int zone;
    if (gLevel == 6)        // THE ASCENT: the score climbs with you
        zone = (alt < 14)? 0 : (alt < 88)? 1 : (alt < 302)? 2 : (alt < 388)? 3
             : (alt < 542)? 2 : 4;
    else if (gLevel == 3) zone = (alt < 6)? 0 : 3;
    else if (gLevel == 5) zone = 4;
    else zone = (alt < 14.0f)? 0 : (alt < 45.0f)? 1 : 2;
    for (int i=0;i<5;i++){
        float tgt = gMuted? 0.0f : won? (i==2? 0.20f : 0.0f) : (i==zone? 0.45f : 0.0f);
        tgt *= gMusicDuck;                            // the world holds its breath as you fall
        gMusVol[i] += (tgt - gMusVol[i])*fminf(1.0f, dt*1.5f);
        SetSoundVolume(gMusic[i], gMusVol[i]);
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
    { // PERFECT: a triumphant little harp burst, unmistakably its own thing
      auto b=mkbuf(0.65f);
      float ns[5]={1046,1318,1568,2093,2637};
      for (int i=0;i<5;i++) addTone(b, i*0.05f, 0.38f, ns[i], 0.20f, 0.5f, 0.004f);
      addSweep(b, 0.22f, 0.35f, 2093, 3520, 0.08f);
      addHat(b, 0.0f, 0.10f); addHat(b, 0.10f, 0.08f);
      sPerfect=bakeSound(b); }
    { // big-impact body thud
      auto b=mkbuf(0.28f);
      addSweep(b,0,0.22f,72,36,0.75f);
      addNoise(b,0,0.10f,0.45f,0.30f);
      sThud=bakeSound(b); }
    BakeMusic();
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
