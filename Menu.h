#pragma once
#include <Arduino.h>
#include <math.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include "ControllerManager.h"
#include "SmallFont.h"
#include "Settings.h"

class Menu {
public:
    int selected = 0;
    int scrollOffset = 0;   // Target scroll (in visible rows)
    float scrollPos = 0.0f; // Animated scroll position (mega smooth)
    const char* options[10] = { "Snake", "Tron", "Pong", "Breakout", "Shooter", "Labyrinth", "Tetris", "Emojis", "Asteroids", "Settings" };
    static const int NUM_OPTIONS = 10;
    static const int VISIBLE_ITEMS = 7;  // 7 lines *8px + 8px HUD = 64px

    // HUD layout
    static constexpr int HUD_H = 8;
    static constexpr float SCROLL_SMOOTH = 0.18f; // 0..1 (higher = snappier)
    static constexpr float STICK_DEADZONE = 0.22f;
    static constexpr int16_t AXIS_DIVISOR = 512;
    static constexpr uint16_t DPAD_REPEAT_DELAY_MS = 450;   // must hold this long before repeat
    static constexpr uint16_t DPAD_REPEAT_INTERVAL_MS = 180;

    // Bluepad32 analog helper (SFINAE) so we don't hard-depend on a single API surface.
    struct InputDetail {
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
    
    // Get actual number of visible options based on player count
    int getVisibleOptionsCount(int players) {
        int count = 0;
        for (int i = 0; i < NUM_OPTIONS; i++) {
            if (isOptionVisible(i, players)) count++;
        }
        return count;
    }
    
    // Check if option should be visible
    bool isOptionVisible(int index, int players) {
        if (index == 6) {  // Tetris
            return players == 1;
        }
        if (index == 8) {  // Asteroids
            return players == 1;
        }
        // All others always visible
        return true;  // All other options always visible
    }
    
    // Get actual option index from visible index
    int getActualIndex(int visibleIndex, int players) {
        int actual = 0;
        int visible = 0;
        
        for (int i = 0; i < NUM_OPTIONS; i++) {
            if (isOptionVisible(i, players)) {
                if (visible == visibleIndex) {
                    return i;
                }
                visible++;
            }
        }
        return visibleIndex;  // Fallback
    }
    
    // Get visible index from actual index
    int getVisibleIndex(int actualIndex, int players) {
        int visible = 0;
        for (int i = 0; i < actualIndex; i++) {
            if (isOptionVisible(i, players)) {
                visible++;
            }
        }
        return visible;
    }

    void draw(MatrixPanel_I2S_DMA* d, ControllerManager* input) {
        const int players = (input != nullptr) ? input->getConnectedCount() : 0;
        d->fillScreen(0);
        
        // ----------------------
        // HUD: "MENU" + player icons (P1..P4)
        // ----------------------
        SmallFont::drawString(d, 2, 6, "MENU", COLOR_CYAN);
        for (int x = 0; x < PANEL_RES_X; x += 2) d->drawPixel(x, HUD_H - 1, COLOR_BLUE);

        const uint16_t pColors[MAX_GAMEPADS] = {
            globalSettings.getPlayerColor(),
            COLOR_CYAN,
            COLOR_ORANGE,
            COLOR_PURPLE
        };
        const uint16_t offC = d->color565(90, 90, 90);

        // "P1" is small, but we still keep a 1px gap between tokens for readability.
        static constexpr int TOKEN_W = 7;   // approx width of "P1" in TomThumb
        static constexpr int TOKEN_GAP = 1; // requested 1px separation
        static constexpr int TOKEN_STRIDE = TOKEN_W + TOKEN_GAP;
        int px = PANEL_RES_X - (MAX_GAMEPADS * TOKEN_STRIDE);
        for (int i = 0; i < MAX_GAMEPADS; i++) {
            const bool connected = (input && input->getController(i) != nullptr);
            SmallFont::drawStringF(d, px, 6, connected ? pColors[i] : offC, "P%d", i + 1);
            px += TOKEN_STRIDE;
        }

        // Count visible options
        int visibleCount = 0;
        for (int i = 0; i < NUM_OPTIONS; i++) {
            if (isOptionVisible(i, players)) {
                visibleCount++;
            }
        }
        
        // Ensure selected is valid
        int visibleSelected = getVisibleIndex(selected, players);
        if (visibleSelected >= visibleCount) {
            visibleSelected = visibleCount - 1;
            // Find actual index
            for (int i = 0; i < NUM_OPTIONS; i++) {
                if (isOptionVisible(i, players)) {
                    if (getVisibleIndex(i, players) == visibleSelected) {
                        selected = i;
                        break;
                    }
                }
            }
        }
        
        // Calculate visible range for scrolling (target offset)
        int maxVisible = VISIBLE_ITEMS;
        if (visibleSelected < scrollOffset) {
            scrollOffset = visibleSelected;
        }
        if (visibleSelected >= scrollOffset + maxVisible) {
            scrollOffset = visibleSelected - maxVisible + 1;
        }

        // Smooth scroll animation towards target.
        scrollPos = scrollPos + ((float)scrollOffset - scrollPos) * SCROLL_SMOOTH;
        
        // Draw options with animated scroll (mega smooth)
        // List layout:
        // - TomThumb uses a baseline Y; keep the baseline safely below the HUD and within screen.
        // - With 7 rows at 8px spacing, baselines span [HUD_H+6 .. HUD_H+6+48] => [14..62].
        const float listBaseY = (float)HUD_H + 6.0f;
        const float listTopBaseline = listBaseY;
        const float listBottomBaseline = listBaseY + (float)(VISIBLE_ITEMS - 1) * 8.0f;

        // If the selected row would be clipped due to scroll animation lag, snap scrollPos to the
        // target so selection is always visible.
        {
            const float selY = listBaseY + ((float)visibleSelected - scrollPos) * 8.0f;
            if (selY < listTopBaseline || selY > listBottomBaseline) {
                scrollPos = (float)scrollOffset;
            }
        }
        int visibleIdx = 0;
        for (int i = 0; i < NUM_OPTIONS; i++) {
            if (!isOptionVisible(i, players)) continue;
            
            const float yF = listBaseY + ((float)visibleIdx - scrollPos) * 8.0f;
            const int yPos = (int)yF;
            // Hard clip so we never draw into the HUD, while allowing the full 7 visible rows.
            // (Baselines range 14..62; allow exactly that range.)
            if (yPos < (int)listTopBaseline || yPos > (int)listBottomBaseline) {
                visibleIdx++;
                continue;
            }
                
            // Draw selection indicator
            if (i == selected) {
                SmallFont::drawChar(d, 2, yPos, '>', COLOR_GREEN);
            } else {
                SmallFont::drawChar(d, 2, yPos, ' ', COLOR_WHITE);
            }
            
            // Draw option name
            SmallFont::drawString(d, 6, yPos, options[i], 
                i == selected ? COLOR_GREEN : COLOR_WHITE);
            visibleIdx++;
        }
        
        // Scroll indicators (if needed)
        if (scrollOffset > 0) {
            // Up arrow indicator
            d->drawPixel(60, HUD_H + 1, COLOR_WHITE);
            d->drawPixel(59, HUD_H + 2, COLOR_WHITE);
            d->drawPixel(61, HUD_H + 2, COLOR_WHITE);
        }
        if (scrollOffset + maxVisible < visibleCount) {
            // Down arrow indicator
            d->drawPixel(60, 62, COLOR_WHITE);
            d->drawPixel(59, 61, COLOR_WHITE);
            d->drawPixel(61, 61, COLOR_WHITE);
        }
    }

    int update(ControllerManager* input) {
        ControllerPtr ctl = input->getController(0);
        if (!ctl) return -1;

        int players = input->getConnectedCount();
        const uint8_t dpad = ctl->dpad();
        static unsigned long lastAnalogMove = 0;
        static uint8_t prevDpad = 0;
        static uint32_t dpadHoldStartMs = 0;
        static uint32_t lastDpadRepeatMs = 0;
        unsigned long now = millis();
        
        // ----------------------
        // Navigate menu (analog + D-pad)
        // ----------------------
        // 1) D-pad: single-step on press, and ONLY repeat after a hold delay.
        int navDir = 0;
        const bool dUp = (dpad & 0x01) != 0;
        const bool dDown = (dpad & 0x02) != 0;
        const bool prevUp = (prevDpad & 0x01) != 0;
        const bool prevDown = (prevDpad & 0x02) != 0;
        prevDpad = dpad;

        if (dUp || dDown) {
            if ((dUp && !prevUp) || (dDown && !prevDown)) {
                // Edge press => exactly one step.
                navDir = dUp ? -1 : 1;
                dpadHoldStartMs = (uint32_t)now;
                lastDpadRepeatMs = (uint32_t)now;
            } else {
                // Held => repeat only after delay.
                if (dpadHoldStartMs != 0 &&
                    (uint32_t)(now - dpadHoldStartMs) >= DPAD_REPEAT_DELAY_MS &&
                    (uint32_t)(now - lastDpadRepeatMs) >= DPAD_REPEAT_INTERVAL_MS) {
                    navDir = dUp ? -1 : 1;
                    lastDpadRepeatMs = (uint32_t)now;
                }
            }
        } else {
            dpadHoldStartMs = 0;
        }

        // 2) Analog: variable repeat rate based on magnitude (ignored while D-pad is held).
        if (navDir == 0 && !(dUp || dDown)) {
            const float rawY = clampf((float)InputDetail::axisY(ctl, 0) / (float)AXIS_DIVISOR, -1.0f, 1.0f);
            const float sy = deadzone01(rawY, STICK_DEADZONE);
            if (sy < 0) navDir = -1;
            else if (sy > 0) navDir = 1;

            if (navDir != 0) {
                const float mag = fabsf(sy);
                // Slower analog scrolling (overall).
                const uint32_t interval = (uint32_t)(320.0f - 160.0f * mag); // ~320ms..160ms
                if ((uint32_t)(now - lastAnalogMove) > interval) {
                    lastAnalogMove = now;
                } else {
                    navDir = 0;
                }
            } else {
                // Reset so next analog nudge feels immediate.
                lastAnalogMove = 0;
            }
        }

        if (navDir != 0) {
            {
                const int currentVisible = getVisibleIndex(selected, players);
                const int visibleCount = getVisibleOptionsCount(players);

                if (navDir < 0 && currentVisible > 0) {
                    for (int i = selected - 1; i >= 0; i--) {
                        if (isOptionVisible(i, players)) { selected = i; break; }
                    }
                } else if (navDir > 0 && currentVisible < visibleCount - 1) {
                    for (int i = selected + 1; i < NUM_OPTIONS; i++) {
                        if (isOptionVisible(i, players)) { selected = i; break; }
                    }
                }
            }
        }
        
        // Select with A button
        static unsigned long lastSelect = 0;
        if (ctl->a() && (now - lastSelect > 200)) {
            lastSelect = now;
            return selected;
        }

        // Cycle player color with Y button (debounced)
        static unsigned long lastColorChange = 0;
        // Bluepad32 exposes ABXY on most pads; if a controller doesn't have Y, this stays false.
        if (ctl->y() && (now - lastColorChange > 200)) {
            lastColorChange = now;
            globalSettings.cyclePlayerColor(1);
            globalSettings.save();
        }
        
        return -1;
    }
};
