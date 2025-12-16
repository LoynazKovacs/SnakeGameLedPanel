#pragma once
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include "ControllerManager.h"

class GameBase {
public:
    virtual void start() = 0;
    virtual void update(ControllerManager* input) = 0;
    virtual void draw(MatrixPanel_I2S_DMA* display) = 0;
    virtual bool isGameOver() = 0;
    virtual void reset() = 0;
    virtual ~GameBase() {}
};
