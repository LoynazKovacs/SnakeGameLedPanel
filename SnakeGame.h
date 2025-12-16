#pragma once
#include <Arduino.h>
#include <vector>
#include "GameBase.h"
#include "config.h"

#define LOGICAL_WIDTH  (PANEL_RES_X / GRID_SIZE)
#define LOGICAL_HEIGHT (PANEL_RES_Y / GRID_SIZE)

enum Direction { UP, DOWN, LEFT, RIGHT, NONE };

struct Point {
    int x;
    int y;
};

class Snake {
public:
    std::vector<Point> body;
    Direction dir;
    Direction nextDir;
    uint16_t color;
    bool alive;
    int score;
    int playerIndex;

    Snake(int idx, int x, int y, uint16_t c) {
        playerIndex = idx;
        color = c;
        reset(x, y);
    }

    void reset(int x, int y) {
        body.clear();
        body.push_back({x, y});
        body.push_back({x, y + 1});
        dir = UP;
        nextDir = UP;
        alive = true;
        score = 0;
    }

    void handleInput(ControllerPtr ctl) {
        if (!ctl || !ctl->isConnected()) return;

        uint8_t d = ctl->dpad();
        if ((d & 0x01) && dir != DOWN) nextDir = UP;
        if ((d & 0x02) && dir != UP) nextDir = DOWN;
        if ((d & 0x04) && dir != LEFT) nextDir = RIGHT;
        if ((d & 0x08) && dir != RIGHT) nextDir = LEFT;
    }

    void move(bool grow) {
        if (!alive || dir == NONE) return;

        dir = nextDir;
        Point head = body.front();

        if (dir == UP) head.y--;
        else if (dir == DOWN) head.y++;
        else if (dir == LEFT) head.x--;
        else if (dir == RIGHT) head.x++;

        if (head.x < 0 || head.x >= LOGICAL_WIDTH ||
            head.y < 0 || head.y >= LOGICAL_HEIGHT) {
            alive = false;
            return;
        }

        body.insert(body.begin(), head);
        if (!grow) body.pop_back();
    }
};

class SnakeGame : public GameBase {
private:
    std::vector<Snake> snakes;
    Point food;
    unsigned long lastMove;
    bool gameOver;

    uint16_t playerColors[4] = {
        COLOR_GREEN, COLOR_CYAN, COLOR_ORANGE, COLOR_PURPLE
    };

    void spawnFood() {
        bool ok;
        do {
            ok = true;
            food.x = random(0, LOGICAL_WIDTH);
            food.y = random(0, LOGICAL_HEIGHT);

            for (auto& s : snakes) {
                for (auto& p : s.body) {
                    if (p.x == food.x && p.y == food.y) {
                        ok = false;
                        break;
                    }
                }
            }
        } while (!ok);
    }

public:
    SnakeGame() {
        lastMove = 0;
        gameOver = false;
    }

    void start() override {
        snakes.clear();
        gameOver = false;
        lastMove = millis();
        spawnFood();

        for (int i = 0; i < MAX_GAMEPADS; i++) {
            if (globalControllerManager->getController(i)) {
                snakes.emplace_back(
                    i,
                    LOGICAL_WIDTH / 2 + i * 2,
                    LOGICAL_HEIGHT / 2,
                    playerColors[i]
                );
            }
        }
    }

    void reset() override {
        start();
    }

    void update(ControllerManager* input) override {
        if (gameOver) return;

        if (millis() - lastMove < SNAKE_SPEED_MS) return;
        lastMove = millis();

        for (auto& s : snakes) {
            if (!s.alive) continue;

            ControllerPtr ctl = input->getController(s.playerIndex);
            if (!ctl) {
                s.alive = false;
                continue;
            }

            s.handleInput(ctl);

            Point& head = s.body.front();
            bool grow = (head.x == food.x && head.y == food.y);

            s.move(grow);
            if (grow) {
                s.score += 10;
                spawnFood();
            }

            for (size_t i = 1; i < s.body.size(); i++) {
                if (s.body[i].x == s.body[0].x &&
                    s.body[i].y == s.body[0].y) {
                    s.alive = false;
                    break;
                }
            }
        }

        bool anyAlive = false;
        for (auto& s : snakes) if (s.alive) anyAlive = true;
        if (!anyAlive && !snakes.empty()) gameOver = true;
    }

    void draw(MatrixPanel_I2S_DMA* display) override {
        display->fillScreen(COLOR_BLACK);

        if (gameOver) {
            display->setCursor(5, 30);
            display->setTextColor(COLOR_RED);
            display->print("GAME OVER");
            return;
        }

        display->fillRect(
            food.x * GRID_SIZE,
            food.y * GRID_SIZE,
            GRID_SIZE,
            GRID_SIZE,
            COLOR_RED
        );

        for (auto& s : snakes) {
            if (!s.alive) continue;
            for (auto& p : s.body) {
                display->fillRect(
                    p.x * GRID_SIZE,
                    p.y * GRID_SIZE,
                    GRID_SIZE,
                    GRID_SIZE,
                    s.color
                );
            }
        }
    }

    bool isGameOver() override {
        return gameOver;
    }
};
