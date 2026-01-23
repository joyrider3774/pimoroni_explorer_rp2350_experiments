// Musical Demo for i2stones library on Adafruit FruitJam
// Features: melodies, chords, arpeggios, sound effects, and classic tunes
#include "mixedtones.h"

#define volume 20

// Note frequencies (Middle C = C4)
#define NOTE_C4  261.63
#define NOTE_CS4 277.18
#define NOTE_D4  293.66
#define NOTE_DS4 311.13
#define NOTE_E4  329.63
#define NOTE_F4  349.23
#define NOTE_FS4 369.99
#define NOTE_G4  392.00
#define NOTE_GS4 415.30
#define NOTE_A4  440.00
#define NOTE_AS4 466.16
#define NOTE_B4  493.88
#define NOTE_C5  523.25
#define NOTE_CS5 554.37
#define NOTE_D5  587.33
#define NOTE_DS5 622.25
#define NOTE_E5  659.25
#define NOTE_F5  698.46
#define NOTE_FS5 739.99
#define NOTE_G5  783.99
#define NOTE_GS5 830.61
#define NOTE_A5  880.00
#define NOTE_AS5 932.33
#define NOTE_B5  987.77
#define NOTE_C6  1046.50
#define NOTE_CS6 1108.73
#define NOTE_D6  1174.66
#define NOTE_DS6 1244.51
#define NOTE_E6  1318.51
#define NOTE_F6  1396.91
#define NOTE_FS6 1479.98
#define NOTE_G6  1567.98
#define NOTE_GS6 1661.22
#define NOTE_A6  1760.00

uint32_t demoStartTime = 0;

void setup() {
    Serial.begin(115200);
    while(!Serial)
      delay(10);
    delay(1000);
    
    Serial.println("\n╔═══════════════════════════════════════╗");
    Serial.println("║   MIXEDTONES MUSICAL DEMO - Pimoroni Explorer   ║");
    Serial.println("╚═══════════════════════════════════════╝\n");
    
    setupAudio(44100,12,13);
    Serial.println("✓ Mixed Tones Audio initialized!");
    Serial.println("\n🎵 Starting musical demo...\n");
    
    demoStartTime = millis();
}

// ═══════════════════════════════════════════════════════════
// SOUND EFFECTS
// ═══════════════════════════════════════════════════════════

void powerUp() {
    Serial.println("🔊 Power Up!");
    for (float f = 200; f <= 800; f += 50) {
        playTone(f, volume, 0.03);
        delay(30);
    }
}

void powerDown() {
    Serial.println("🔊 Power Down!");
    for (float f = 800; f >= 200; f -= 50) {
        playTone(f, volume, 0.03);
        delay(30);
    }
}

void laserShot() {
    Serial.println("🔊 Laser!");
    playTone(1200, volume, 0.05);
    delay(50);
    playTone(800, volume, 0.05);
    delay(50);
    playTone(400, volume, 0.1);
}

void explosion() {
    Serial.println("🔊 Explosion!");
    // White noise simulation with multiple random frequencies
    for (int i = 0; i < 10; i++) {
        playTone(random(100, 500), volume, 0.05);
    }
}

void coin() {
    Serial.println("🔊 Coin!");
    playTone(NOTE_B5, volume, 0.1);
    delay(100);
    playTone(NOTE_E6, volume, 0.3);
}

// ═══════════════════════════════════════════════════════════
// MUSICAL PATTERNS
// ═══════════════════════════════════════════════════════════

void playScale() {
    Serial.println("🎹 Playing C Major Scale");
    const float scale[] = {NOTE_C4, NOTE_D4, NOTE_E4, NOTE_F4, NOTE_G4, NOTE_A4, NOTE_B4, NOTE_C5};
    
    for (int i = 0; i < 8; i++) {
        playTone(scale[i], volume, 0.25);
        delay(280);
    }
}

void playChord(float note1, float note2, float note3, float duration) {
    playTone(note1, volume, duration);
    playTone(note2, volume, duration);
    playTone(note3, volume, duration);
}

void playChordProgression() {
    Serial.println("🎸 Chord Progression: C-Am-F-G");
    
    // C major
    playChord(NOTE_C4, NOTE_E4, NOTE_G4, 0.8);
    delay(900);
    
    // A minor
    playChord(NOTE_A4, NOTE_C5, NOTE_E5, 0.8);
    delay(900);
    
    // F major
    playChord(NOTE_F4, NOTE_A4, NOTE_C5, 0.8);
    delay(900);
    
    // G major
    playChord(NOTE_G4, NOTE_B4, NOTE_D5, 0.8);
    delay(900);
}

void playArpeggio() {
    Serial.println("🎼 C Major Arpeggio");
    const float notes[] = {NOTE_C4, NOTE_E4, NOTE_G4, NOTE_C5, NOTE_G4, NOTE_E4};
    
    for (int i = 0; i < 6; i++) {
        playTone(notes[i], volume, 0.15);
        delay(160);
    }
}

// ═══════════════════════════════════════════════════════════
// CLASSIC MELODIES
// ═══════════════════════════════════════════════════════════

void playMaryHadALittleLamb() {
    Serial.println("🐑 Mary Had a Little Lamb");
    
    struct Note { float freq; int duration; };
    Note melody[] = {
        {NOTE_E4, 300}, {NOTE_D4, 300}, {NOTE_C4, 300}, {NOTE_D4, 300},
        {NOTE_E4, 300}, {NOTE_E4, 300}, {NOTE_E4, 600},
        {NOTE_D4, 300}, {NOTE_D4, 300}, {NOTE_D4, 600},
        {NOTE_E4, 300}, {NOTE_G4, 300}, {NOTE_G4, 600},
        {NOTE_E4, 300}, {NOTE_D4, 300}, {NOTE_C4, 300}, {NOTE_D4, 300},
        {NOTE_E4, 300}, {NOTE_E4, 300}, {NOTE_E4, 300}, {NOTE_E4, 300},
        {NOTE_D4, 300}, {NOTE_D4, 300}, {NOTE_E4, 300}, {NOTE_D4, 300},
        {NOTE_C4, 900}
    };
    
    for (int i = 0; i < sizeof(melody) / sizeof(Note); i++) {
        playTone(melody[i].freq, volume, melody[i].duration / 1000.0);
        delay(melody[i].duration + 50);
    }
}

void playTwinkle() {
    Serial.println("⭐ Twinkle Twinkle Little Star");
    
    struct Note { float freq; int duration; };
    Note melody[] = {
        {NOTE_C4, 400}, {NOTE_C4, 400}, {NOTE_G4, 400}, {NOTE_G4, 400},
        {NOTE_A4, 400}, {NOTE_A4, 400}, {NOTE_G4, 800},
        {NOTE_F4, 400}, {NOTE_F4, 400}, {NOTE_E4, 400}, {NOTE_E4, 400},
        {NOTE_D4, 400}, {NOTE_D4, 400}, {NOTE_C4, 800},
        {NOTE_G4, 400}, {NOTE_G4, 400}, {NOTE_F4, 400}, {NOTE_F4, 400},
        {NOTE_E4, 400}, {NOTE_E4, 400}, {NOTE_D4, 800},
        {NOTE_G4, 400}, {NOTE_G4, 400}, {NOTE_F4, 400}, {NOTE_F4, 400},
        {NOTE_E4, 400}, {NOTE_E4, 400}, {NOTE_D4, 800},
        {NOTE_C4, 400}, {NOTE_C4, 400}, {NOTE_G4, 400}, {NOTE_G4, 400},
        {NOTE_A4, 400}, {NOTE_A4, 400}, {NOTE_G4, 800},
        {NOTE_F4, 400}, {NOTE_F4, 400}, {NOTE_E4, 400}, {NOTE_E4, 400},
        {NOTE_D4, 400}, {NOTE_D4, 400}, {NOTE_C4, 800}
    };
    
    for (int i = 0; i < sizeof(melody) / sizeof(Note); i++) {
        playTone(melody[i].freq, volume, melody[i].duration / 1000.0);
        delay(melody[i].duration + 30);
    }
}

void playHappyBirthday() {
    Serial.println("🎂 Happy Birthday!");
    
    struct Note { float freq; int duration; };
    Note melody[] = {
        {NOTE_C4, 250}, {NOTE_C4, 125}, {NOTE_D4, 375},
        {NOTE_C4, 375}, {NOTE_F4, 375}, {NOTE_E4, 750},
        
        {NOTE_C4, 250}, {NOTE_C4, 125}, {NOTE_D4, 375},
        {NOTE_C4, 375}, {NOTE_G4, 375}, {NOTE_F4, 750},
        
        {NOTE_C4, 250}, {NOTE_C4, 125}, {NOTE_C5, 375},
        {NOTE_A4, 375}, {NOTE_F4, 375}, {NOTE_E4, 375}, {NOTE_D4, 750},
        
        {NOTE_AS4, 250}, {NOTE_AS4, 125}, {NOTE_A4, 375},
        {NOTE_F4, 375}, {NOTE_G4, 375}, {NOTE_F4, 750}
    };
    
    for (int i = 0; i < sizeof(melody) / sizeof(Note); i++) {
        playTone(melody[i].freq, volume, melody[i].duration / 1000.0);
        delay(melody[i].duration + 50);
    }
}

void playMarioTheme() {
    Serial.println("🍄 Super Mario Bros Theme (excerpt)");
    
    struct Note { float freq; int duration; };
    Note melody[] = {
        {NOTE_E5, 150}, {NOTE_E5, 150}, {0, 150}, {NOTE_E5, 150},
        {0, 150}, {NOTE_C5, 150}, {NOTE_E5, 300},
        {NOTE_G5, 600}, {NOTE_G4, 600},
        
        {NOTE_C5, 450}, {NOTE_G4, 300}, {NOTE_E4, 450},
        {NOTE_A4, 300}, {NOTE_B4, 300}, {NOTE_AS4, 150}, {NOTE_A4, 450},
        
        {NOTE_G4, 200}, {NOTE_E5, 200}, {NOTE_G5, 200},
        {NOTE_A5, 300}, {NOTE_F5, 150}, {NOTE_G5, 150},
        {0, 150}, {NOTE_E5, 150}, {NOTE_C5, 150}, {NOTE_D5, 150}, {NOTE_B4, 450}
    };
    
    for (int i = 0; i < sizeof(melody) / sizeof(Note); i++) {
        if (melody[i].freq > 0) {
            playTone(melody[i].freq, volume, melody[i].duration / 1000.0);
        }
        delay(melody[i].duration + 20);
    }
}

// ═══════════════════════════════════════════════════════════
// ADVANCED DEMOS
// ═══════════════════════════════════════════════════════════

void multiChannelDemo() {
    Serial.println("🎛️  Multi-Channel Demo - 4 simultaneous tones");
    
    // Play 4 notes of a C major 7th chord with scheduled start times
    playTone(NOTE_C4, volume, 2.0, 0.0);   // Root
    playTone(NOTE_E4, volume, 2.0, 0.2);   // Third
    playTone(NOTE_G4, volume, 2.0, 0.4);   // Fifth
    playTone(NOTE_B4, volume, 2.0, 0.6);   // Seventh
    
    delay(3000);
}

void sirens() {
    Serial.println("🚨 Police Siren");
    
    for (int i = 0; i < 4; i++) {
        // Up sweep
        for (float f = 400; f <= 800; f += 30) {
            playTone(f, volume, 0.03);
            delay(30);
        }
        // Down sweep
        for (float f = 800; f >= 400; f -= 30) {
            playTone(f, volume, 0.03);
            delay(30);
        }
    }
}

void echoDemo() {
    Serial.println("🔁 Echo Effect");
    
    float note = NOTE_A4;
    uint8_t volumes[] = {255, 180, 120, 80, 50};
    
    for (int i = 0; i < 5; i++) {
        playTone(note, volumes[i], 0.15, i * 0.2);
    }
    
    delay(2000);
}

// ═══════════════════════════════════════════════════════════
// MAIN DEMO SEQUENCE
// ═══════════════════════════════════════════════════════════

void loop() {
    updateAudio();
    
    static int demoStep = 0;
    static uint32_t lastDemo = 0;
    
    // Run demos every 5 seconds
    if (millis() - lastDemo > 5000) {
        lastDemo = millis();
        
        Serial.print("\n▶ Demo "); Serial.print(demoStep + 1); Serial.println("/15");
        Serial.println("────────────────────────────────────");
        
        switch(demoStep) {
            case 0: powerUp(); break;
            case 1: playScale(); break;
            case 2: playArpeggio(); break;
            case 3: coin(); break;
            case 4: playChordProgression(); break;
            case 5: laserShot(); break;
            case 6: playMaryHadALittleLamb(); break;
            case 7: explosion(); break;
            case 8: playTwinkle(); break;
            case 9: sirens(); break;
            case 10: multiChannelDemo(); break;
            case 11: playMarioTheme(); break;
            case 12: echoDemo(); break;
            case 13: playHappyBirthday(); break;
            case 14: powerDown(); break;
        }
        
        demoStep++;
        if (demoStep >= 15) {
            Serial.println("\n\n🎉 Demo sequence complete! Restarting...\n");
            demoStep = 0;
        }
    }
    
}
