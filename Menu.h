#pragma once
#include <Arduino.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include "ControllerManager.h"

class Menu {
public:
    int selected = 0;
    const char* options[1] = { "Snake" };

    void draw(MatrixPanel_I2S_DMA* d, int players) {
        d->fillScreen(0);
        d->setCursor(4, 10);
        d->setTextColor(COLOR_CYAN);
        d->print("MAIN MENU");

        d->setCursor(8, 30);
        d->setTextColor(COLOR_GREEN);
        d->print("> Snake");

        d->setCursor(4, 55);
        d->setTextColor(COLOR_YELLOW);
        d->printf("%d Player(s)", players);
    }

    int update(ControllerManager* input) {
        ControllerPtr ctl = input->getController(0);
        if (ctl && ctl->a()) return 0;
        return -1;
    }
};
