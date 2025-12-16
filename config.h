#pragma once
#include <Arduino.h>

// =======================================================
// Display Configuration
// =======================================================
#define PANEL_RES_X 64
#define PANEL_RES_Y 64
#define PANEL_CHAIN 1

// HUB75 Pins
#define R1_PIN 25
#define G1_PIN 26
#define B1_PIN 27
#define R2_PIN 14
#define G2_PIN 12
#define B2_PIN 13

#define A_PIN 23
#define B_PIN 19
#define C_PIN 5
#define D_PIN 17
#define E_PIN 32

#define LAT_PIN 4
#define OE_PIN 15
#define CLK_PIN 16

#define DISPLAY_COLOR_DEPTH 5

// =======================================================
// Game Configuration
// =======================================================
#define MAX_GAMEPADS 4
#define SNAKE_SPEED_MS 100
#define GRID_SIZE 4

// RGB565 Colors
#define COLOR_BLACK   0x0000
#define COLOR_WHITE   0xFFFF
#define COLOR_RED     0xF800
#define COLOR_GREEN   0x07E0
#define COLOR_BLUE    0x001F
#define COLOR_CYAN    0x07FF
#define COLOR_MAGENTA 0xF81F
#define COLOR_YELLOW  0xFFE0
#define COLOR_ORANGE  0xFD20
#define COLOR_PURPLE  0x780F
