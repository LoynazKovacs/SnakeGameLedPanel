#pragma once
#include <Arduino.h>
#include <math.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include "ControllerManager.h"
#include "SmallFont.h"
#include "Settings.h"

// Forward declaration
extern MatrixPanel_I2S_DMA* dma_display;

/**
 * SettingsMenu - Menu for adjusting system settings
 * Allows changing brightness and other persistent settings
 */
class SettingsMenu {
public:
    enum SettingType {
        SETTING_BRIGHTNESS,
        SETTING_GAME_SPEED,
        SETTING_SOUND,
        SETTING_RESET,
        SETTING_BACK,
        NUM_SETTINGS
    };
    
    int selected = 0;
    bool isActive = false;

    // HUD layout
    static constexpr int HUD_H = 8;

    // Analog tuning
    static constexpr float STICK_DEADZONE = 0.22f;
    static constexpr int16_t AXIS_DIVISOR = 512;

    // Repeat behavior (prevents double steps when not "fast enough")
    static constexpr uint16_t DPAD_REPEAT_DELAY_MS = 450;
    static constexpr uint16_t DPAD_REPEAT_INTERVAL_MS = 180;
    
    // Bluepad32 analog helper (SFINAE) so we don't hard-depend on a single API surface.
    struct InputDetail {
        template <typename T>
        static auto axisX(T* c, int) -> decltype(c->axisX(), int16_t()) { return (int16_t)c->axisX(); }
        template <typename T>
        static int16_t axisX(T*, ...) { return 0; }

        template <typename T>
        static auto axisY(T* c, int) -> decltype(c->axisY(), int16_t()) { return (int16_t)c->axisY(); }
        template <typename T>
        static int16_t axisY(T*, ...) { return 0; }
    };

    static inline float clampf(float v, float lo, float hi) { return (v < lo) ? lo : (v > hi) ? hi : v; }
    static inline float deadzone01(float v, float dz) {
        const float a = fabsf(v);
        if (a <= dz) return 0.0f;
        const float s = (a - dz) / (1.0f - dz);
        return (v < 0) ? -s : s;
    }

    void draw(MatrixPanel_I2S_DMA* display, ControllerManager* input) {
        (void)input; // Settings screen doesn't need to show player icons.
        display->fillScreen(0);
        
        // ----------------------
        // HUD
        // ----------------------
        SmallFont::drawString(display, 2, 6, "SETTINGS", COLOR_CYAN);
        for (int x = 0; x < PANEL_RES_X; x += 2) display->drawPixel(x, HUD_H - 1, COLOR_BLUE);
        
        // Draw settings options
        const char* settingNames[] = {
            "Brightness",
            "Game Speed",
            "Sound",
            "Reset",
            "Back"
        };
        
        for (int i = 0; i < NUM_SETTINGS; i++) {
            int yPos = HUD_H + 6 + (i * 8);
            
            // Selection indicator
            if (i == selected) {
                SmallFont::drawChar(display, 2, yPos, '>', COLOR_GREEN);
            } else {
                SmallFont::drawChar(display, 2, yPos, ' ', COLOR_WHITE);
            }
            
            // Setting name
            SmallFont::drawString(display, 8, yPos, settingNames[i], 
                i == selected ? COLOR_GREEN : COLOR_WHITE);
            
            // Draw value
            if (i == SETTING_BRIGHTNESS) {
                char val[8];
                snprintf(val, sizeof(val), "%d", globalSettings.getBrightness());
                SmallFont::drawString(display, 50, yPos, val, COLOR_YELLOW);
            } else if (i == SETTING_GAME_SPEED) {
                char val[4];
                snprintf(val, sizeof(val), "%d", globalSettings.getGameSpeed());
                SmallFont::drawString(display, 50, yPos, val, COLOR_YELLOW);
            } else if (i == SETTING_SOUND) {
                const char* val = globalSettings.isSoundEnabled() ? "ON" : "OFF";
                SmallFont::drawString(display, 50, yPos, val, COLOR_YELLOW);
            }
        }
    }
    
    /**
     * Update settings menu and handle input
     * Returns true if user wants to go back
     */
    bool update(ControllerManager* input) {
        ControllerPtr ctl = input->getController(0);
        if (!ctl) return false;
        
        const uint8_t dpad = ctl->dpad();
        const unsigned long now = millis();

        // ----------------------
        // Navigation (analog + D-pad)
        // ----------------------
        static uint8_t prevDpad = 0;
        static uint32_t dpadNavHoldStartMs = 0;
        static uint32_t dpadNavLastRepeatMs = 0;

        // D-pad Up/Down: press once, repeat only after hold delay.
        const bool up = (dpad & 0x01) != 0;
        const bool down = (dpad & 0x02) != 0;
        const bool prevUp = (prevDpad & 0x01) != 0;
        const bool prevDown = (prevDpad & 0x02) != 0;

        int navDir = 0;
        if (up || down) {
            if ((up && !prevUp) || (down && !prevDown)) {
                navDir = up ? -1 : 1;
                dpadNavHoldStartMs = (uint32_t)now;
                dpadNavLastRepeatMs = (uint32_t)now;
            } else if (dpadNavHoldStartMs != 0 &&
                       (uint32_t)(now - dpadNavHoldStartMs) >= DPAD_REPEAT_DELAY_MS &&
                       (uint32_t)(now - dpadNavLastRepeatMs) >= DPAD_REPEAT_INTERVAL_MS) {
                navDir = up ? -1 : 1;
                dpadNavLastRepeatMs = (uint32_t)now;
            }
        } else {
            dpadNavHoldStartMs = 0;
        }

        // Analog Up/Down if D-pad isn't used.
        static uint32_t lastAnalogNavMs = 0;
        if (navDir == 0 && !(up || down)) {
            const float rawY = clampf((float)InputDetail::axisY(ctl, 0) / (float)AXIS_DIVISOR, -1.0f, 1.0f);
            const float sy = deadzone01(rawY, STICK_DEADZONE);
            if (sy < 0) navDir = -1;
            else if (sy > 0) navDir = 1;

            if (navDir != 0) {
                const float mag = fabsf(sy);
                const uint32_t interval = (uint32_t)(320.0f - 160.0f * mag); // ~320ms..160ms
                if ((uint32_t)(now - lastAnalogNavMs) > interval) lastAnalogNavMs = (uint32_t)now;
                else navDir = 0;
            } else {
                lastAnalogNavMs = 0;
            }
        }

        if (navDir < 0 && selected > 0) selected--;
        else if (navDir > 0 && selected < NUM_SETTINGS - 1) selected++;

        // ----------------------
        // Adjust (analog X + D-pad Left/Right)
        // ----------------------
        static uint32_t dpadAdjHoldStartMs = 0;
        static uint32_t dpadAdjLastRepeatMs = 0;
        static uint32_t lastAnalogAdjMs = 0;

        const bool left = (dpad & 0x08) != 0;
        const bool right = (dpad & 0x04) != 0;
        const bool prevLeft = (prevDpad & 0x08) != 0;
        const bool prevRight = (prevDpad & 0x04) != 0;

        int adjDir = 0;
        if (left || right) {
            if ((left && !prevLeft) || (right && !prevRight)) {
                adjDir = left ? -1 : 1;
                dpadAdjHoldStartMs = (uint32_t)now;
                dpadAdjLastRepeatMs = (uint32_t)now;
            } else if (dpadAdjHoldStartMs != 0 &&
                       (uint32_t)(now - dpadAdjHoldStartMs) >= DPAD_REPEAT_DELAY_MS &&
                       (uint32_t)(now - dpadAdjLastRepeatMs) >= DPAD_REPEAT_INTERVAL_MS) {
                adjDir = left ? -1 : 1;
                dpadAdjLastRepeatMs = (uint32_t)now;
            }
        } else {
            dpadAdjHoldStartMs = 0;
        }

        if (adjDir == 0 && !(left || right)) {
            const float rawX = clampf((float)InputDetail::axisX(ctl, 0) / (float)AXIS_DIVISOR, -1.0f, 1.0f);
            const float sx = deadzone01(rawX, STICK_DEADZONE);
            if (sx < 0) adjDir = -1;
            else if (sx > 0) adjDir = 1;

            if (adjDir != 0) {
                const float mag = fabsf(sx);
                const uint32_t interval = (uint32_t)(320.0f - 160.0f * mag);
                if ((uint32_t)(now - lastAnalogAdjMs) > interval) lastAnalogAdjMs = (uint32_t)now;
                else adjDir = 0;
            } else {
                lastAnalogAdjMs = 0;
            }
        }

        if (adjDir != 0) adjustSetting(adjDir);

        // Store prev dpad for edge checks.
        prevDpad = dpad;
        
        // Select/Back with A button
        static unsigned long lastSelect = 0;
        if (ctl->a() && (now - lastSelect > 200)) {
            lastSelect = now;
            
            if (selected == SETTING_RESET) {
                // Reset to defaults
                globalSettings.resetToDefaults();
                globalSettings.save();
                delay(300);
                return false;  // Stay in menu
            } else if (selected == SETTING_BACK) {
                // Save all settings before going back
                globalSettings.save();
                delay(200);
                return true;
            }
        }
        
        // Also allow B button to go back
        static unsigned long lastB = 0;
        if (ctl->b() && (now - lastB > 200)) {
            lastB = now;
            globalSettings.save();
            delay(200);
            return true;
        }
        
        return false;
    }
    
private:
    void adjustSetting(int delta) {
        switch (selected) {
            case SETTING_BRIGHTNESS: {
                int newVal = globalSettings.getBrightness() + (delta * 5);
                globalSettings.setBrightness(newVal);
                globalSettings.save();
                // Apply brightness immediately
                if (dma_display != nullptr) {
                    dma_display->setBrightness8(globalSettings.getBrightness());
                }
                break;
            }
            case SETTING_GAME_SPEED: {
                int newVal = globalSettings.getGameSpeed() + delta;
                globalSettings.setGameSpeed(newVal);
                globalSettings.save();
                break;
            }
            case SETTING_SOUND: {
                globalSettings.setSoundEnabled(!globalSettings.isSoundEnabled());
                globalSettings.save();
                break;
            }
        }
    }
};

