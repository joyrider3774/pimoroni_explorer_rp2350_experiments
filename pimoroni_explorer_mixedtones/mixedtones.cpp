//this files has been generated with the help of claude.ai

#include <stdlib.h>
#include <float.h>
#include <stdint.h>
#include <Arduino.h>
#include <hardware/timer.h>
#include <hardware/pwm.h>
#include <pico/time.h>
#include "mixedtones.h"

// Configuration
uint8_t audioPinPiezo = 12;
uint8_t speakerEnablePin = 13;

// Global audio time tracking
uint32_t audioStartTime = 0;
int32_t sampleRate = 11025;
#define MAX_CHANNELS 64
static struct repeating_timer audio_timer;
static volatile bool timer_running = false;

struct Oscillator {
    uint32_t phase;
    uint32_t phase_increment;
    uint16_t amplitude;
    uint32_t duration_samples;
    uint32_t samples_played;
    uint32_t start_time_ms;
    uint16_t envelope;  // 0-256 for fade in/out
    bool active;
    bool scheduled;
};

Oscillator oscillators[MAX_CHANNELS];

// Active channel tracking
static uint8_t active_channel_indices[MAX_CHANNELS];
static uint8_t active_channel_count = 0;

inline int16_t squareWave(uint8_t phase) {
    return (phase < 128) ? 127 : -127;
}

inline int16_t fastSine(uint8_t phase) {
    static const int8_t sine_lut[64] = {
        0, 6, 12, 18, 25, 31, 37, 43, 49, 54, 60, 65, 71, 76, 81, 85,
        90, 94, 98, 102, 106, 109, 112, 115, 117, 120, 122, 123, 125, 126, 126, 127,
        127, 127, 126, 126, 125, 123, 122, 120, 117, 115, 112, 109, 106, 102, 98, 94,
        90, 85, 81, 76, 71, 65, 60, 54, 49, 43, 37, 31, 25, 18, 12, 6
    };
    
    uint8_t quad = phase >> 6;
    uint8_t index = phase & 0x3F;
    
    int8_t val = sine_lut[index];
    
    if (quad == 1) val = sine_lut[63 - index];
    else if (quad == 2) val = -sine_lut[index];
    else if (quad == 3) val = -sine_lut[63 - index];
    
    return val;
}


// Rebuild active channel list (lock-free)
static inline void rebuildActiveChannelList() {
    uint8_t temp_indices[MAX_CHANNELS];
    uint8_t temp_count = 0;
    
    for (int i = 0; i < MAX_CHANNELS; i++) {
        if (oscillators[i].active || oscillators[i].scheduled) {
            temp_indices[temp_count++] = i;
        }
    }
    
    for (int i = 0; i < temp_count; i++) {
        active_channel_indices[i] = temp_indices[i];
    }
    active_channel_count = temp_count;
}


bool timerCallback_Piezo(struct repeating_timer *t) {
    int32_t mixed_sample = 0;
    int active_count = 0;
    
    for (int i = 0; i < MAX_CHANNELS; i++) {
        if (oscillators[i].active) {
            oscillators[i].phase += oscillators[i].phase_increment;
            oscillators[i].samples_played++;
            
            if (oscillators[i].duration_samples > 0 && 
                oscillators[i].samples_played >= oscillators[i].duration_samples) {
                oscillators[i].active = false;
                oscillators[i].scheduled = false;
            } else {
                if (oscillators[i].amplitude > 0) {
                    uint8_t phase_byte = oscillators[i].phase >> 24;
                    // Use square wave directly - better for piezo with PWM
                    int16_t wave = (phase_byte < 128) ? oscillators[i].amplitude : 0;
                    mixed_sample += wave;
                    active_count++;
                }
            }
        }
    }

    if (active_count > 0) {
        // Average the mixed samples
        mixed_sample /= active_count;
        
        mixed_sample = mixed_sample / 4;  
        
        // Clamp to valid range
        if (mixed_sample < 0) mixed_sample = 0;
        if (mixed_sample > 255) mixed_sample = 255;
        
        // Output via PWM - duty cycle represents volume
        pwm_set_gpio_level(audioPinPiezo, (uint16_t)mixed_sample);
    } else {
        pwm_set_gpio_level(audioPinPiezo, 0);  // Complete silence
    }
    
    return true;
}

void setupAudio(int32_t sample_rate, uint8_t pin_piezo, uint8_t pin_speaker_enable) 
{
    audioPinPiezo = pin_piezo;
    sampleRate = sample_rate;
    speakerEnablePin = pin_speaker_enable;
    
    // Enable the amplifier
    pinMode(speakerEnablePin, OUTPUT);
    digitalWrite(speakerEnablePin, HIGH);
    
    // Configure GPIO 12 for PWM (it's PWM slice 6, channel A)
    gpio_set_function(audioPinPiezo, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(audioPinPiezo);
    
    // Configure PWM
    pwm_config config = pwm_get_default_config();
    
    // CRITICAL: PWM frequency must be MUCH higher than sample rate
    // Set PWM to run at a high carrier frequency (e.g., 150 kHz)
    // System clock is 150 MHz
    float clock_div = 150000000.0f / (150000.0f * 256);  // 150kHz carrier, 8-bit resolution
    pwm_config_set_clkdiv(&config, clock_div);
    pwm_config_set_wrap(&config, 255);  // 8-bit PWM (0-255)
    
    pwm_init(slice_num, &config, true);
    pwm_set_gpio_level(audioPinPiezo, 0);  // Start silent
    
    Serial.print("PWM Audio setup - Pin: ");
    Serial.print(audioPinPiezo);
    Serial.print(" (PWM slice ");
    Serial.print(slice_num);
    Serial.print("), Sample rate: ");
    Serial.print(sampleRate);
    Serial.print(" Hz, PWM freq: ~150 kHz");
    Serial.println();
    
    // Initialize oscillators
    for (int i = 0; i < MAX_CHANNELS; i++) {
        oscillators[i].active = false;
        oscillators[i].scheduled = false;
        oscillators[i].phase = 0;
        oscillators[i].amplitude = 0;
        oscillators[i].duration_samples = 0;
        oscillators[i].samples_played = 0;
        oscillators[i].start_time_ms = 0;
        oscillators[i].envelope = 0;
    }

    // Setup timer interrupt
    int64_t interval_us = -1000000 / sampleRate;  
    Serial.print("Attempting to set timer interval: ");
    Serial.print(-interval_us);
    Serial.println(" microseconds");
    
    if (!add_repeating_timer_us(interval_us, timerCallback_Piezo, NULL, &audio_timer)) {
        Serial.println("ERROR: Failed to setup timer!");
        Serial.println("Sample rate may be too high for reliable interrupt timing");
        timer_running = false;
    } else {
        timer_running = true;
        Serial.print("Timer interrupt enabled successfully at ");
        Serial.print(sampleRate);
        Serial.println(" Hz");
    }
    
    audioStartTime = millis();
}
uint8_t getMaxChannels() {
    return MAX_CHANNELS;
}

int8_t findFreeChannel() {
    for (int i = 0; i < MAX_CHANNELS; i++) {
        if (!oscillators[i].active && !oscillators[i].scheduled) {
            return i;
        }
    }
    return -1;
}

void updateAudio() {
    uint32_t now = millis();
    
    for (int i = 0; i < MAX_CHANNELS; i++) {
        if (oscillators[i].scheduled && !oscillators[i].active) {
            if (now >= oscillators[i].start_time_ms) {
                oscillators[i].phase = 0;
                oscillators[i].samples_played = 0;
                oscillators[i].envelope = 0;
                oscillators[i].active = true;
                oscillators[i].scheduled = false;
            }
        }
    }
    
    rebuildActiveChannelList();
}

int8_t playTone(float frequency, uint8_t volume, float duration_sec, float delay_sec) {
    int8_t channel = findFreeChannel();
    if (channel < 0) return -1;
    
    oscillators[channel].phase_increment = (uint32_t)((frequency * 4294967296.0) / sampleRate);
    oscillators[channel].amplitude = volume;
    
    if (duration_sec > 0) {
        oscillators[channel].duration_samples = (uint32_t)(sampleRate * duration_sec);
    } else {
        oscillators[channel].duration_samples = 0;
    }
    
    if (delay_sec > 0) {
        oscillators[channel].start_time_ms = millis() + (uint32_t)(delay_sec * 1000);
        oscillators[channel].scheduled = true;
        oscillators[channel].active = false;
        oscillators[channel].phase = 0;
        oscillators[channel].samples_played = 0;
    } else {
        oscillators[channel].start_time_ms = 0;
        oscillators[channel].scheduled = false;
        oscillators[channel].active = true;
        oscillators[channel].phase = 0;
        oscillators[channel].samples_played = 0;
    }
    
    oscillators[channel].envelope = 0;
    rebuildActiveChannelList();
    
    return channel;
}

int8_t playToneOnChannel(uint8_t channel, float frequency, uint8_t volume, float duration_sec, float delay_sec) {
    if (channel >= MAX_CHANNELS) return -1;
    
    oscillators[channel].phase_increment = (uint32_t)((frequency * 4294967296.0) / sampleRate);
    oscillators[channel].amplitude = volume;
    
    if (duration_sec > 0) {
        oscillators[channel].duration_samples = (uint32_t)(sampleRate * duration_sec);
    } else {
        oscillators[channel].duration_samples = 0;
    }
    
    if (delay_sec > 0) {
        oscillators[channel].start_time_ms = millis() + (uint32_t)(delay_sec * 1000);
        oscillators[channel].scheduled = true;
        oscillators[channel].active = false;
        oscillators[channel].phase = 0;
        oscillators[channel].samples_played = 0;
    } else {
        oscillators[channel].start_time_ms = 0;
        oscillators[channel].scheduled = false;
        oscillators[channel].active = true;
        oscillators[channel].phase = 0;
        oscillators[channel].samples_played = 0;
    }
    
    oscillators[channel].envelope = 0;
    rebuildActiveChannelList();
    
    return channel;
}

void cancelScheduled(int8_t channel) {
    if (channel >= 0 && channel < MAX_CHANNELS) {
        oscillators[channel].scheduled = false;
        oscillators[channel].active = false;
    }
}

void stopChannel(int8_t channel) {
    if (channel >= 0 && channel < MAX_CHANNELS) {
        oscillators[channel].active = false;
        oscillators[channel].scheduled = false;
        oscillators[channel].amplitude = 0;
    }
}

void stopAllTones() {
    for (int i = 0; i < MAX_CHANNELS; i++) {
        oscillators[i].active = false;
        oscillators[i].scheduled = false;
        oscillators[i].amplitude = 0;
    }
}

bool isChannelActive(int8_t channel) {
    if (channel < 0 || channel >= MAX_CHANNELS) return false;
    return oscillators[channel].active || oscillators[channel].scheduled;
}

uint8_t getActiveChannelCount() {
    uint8_t count = 0;
    for (int i = 0; i < MAX_CHANNELS; i++) {
        if (oscillators[i].active || oscillators[i].scheduled) count++;
    }
    return count;
}

uint8_t getPlayingChannelCount() {
    uint8_t count = 0;
    for (int i = 0; i < MAX_CHANNELS; i++) {
        if (oscillators[i].active) count++;
    }
    return count;
}

uint32_t getAudioStartTime() {
    return audioStartTime;
}
