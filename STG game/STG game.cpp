#include <SDL.h>
#include <vector>
#include <cstdlib>

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;



class Player {
public:
    SDL_Rect shape;
    int speed;

    Player() {
        shape = { SCREEN_WIDTH / 2 - 25, SCREEN_HEIGHT - 50, 50, 50 };
        speed = 5;
    }

    void move(int dx, int dy) {
        shape.x += dx * speed;
        shape.y += dy * speed;

        // 确保玩家不会移动出屏幕
        if (shape.x < 0) shape.x = 0;
        if (shape.x + shape.w > SCREEN_WIDTH) shape.x = SCREEN_WIDTH - shape.w;
    }

    void draw(SDL_Renderer* renderer) {
        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
        SDL_RenderFillRect(renderer, &shape);
    }
};

Player player;
const Uint8* keystate = SDL_GetKeyboardState(NULL);

class Bullet {
public:
    SDL_Rect shape;
    int speed;

    Bullet(int x, int y) {
        shape = { x, y, 5, 10 };
        speed = -10;
    }

    void update() {
        shape.y += speed;
    }

    void draw(SDL_Renderer* renderer) {
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_RenderFillRect(renderer, &shape);
    }
};

class Enemy {
public:
    SDL_Rect shape;
    int speed;

    Enemy(int x, int y) {
        shape = { x, y, 50, 50 };
        speed = 2;
    }

    void update() {
        shape.y += speed;
    }

    void draw(SDL_Renderer* renderer) {
        SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
        SDL_RenderFillRect(renderer, &shape);
    }
};





int main(int argc, char* args[]) {
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("弹幕射击游戏", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    Player player;
    std::vector<Bullet> bullets;
    std::vector<Enemy> enemies;
    bool quit = false;
    SDL_Event event;

    while (!quit) {
        while (SDL_PollEvent(&event) != 0) {
            if (event.type == SDL_QUIT) quit = true;
        }

        const Uint8* keystate = SDL_GetKeyboardState(NULL);
        if (keystate[SDL_SCANCODE_LEFT]) player.move(-1, 0);
        if (keystate[SDL_SCANCODE_RIGHT]) player.move(1, 0);
        if (keystate[SDL_SCANCODE_UP]) player.move(0, -1);
        if (keystate[SDL_SCANCODE_DOWN]) player.move(0, 1);
        if (keystate[SDL_SCANCODE_SPACE]) bullets.push_back(Bullet(player.shape.x + player.shape.w / 2 - 2, player.shape.y));

        for (auto& bullet : bullets) bullet.update();
        for (auto& enemy : enemies) enemy.update();

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        player.draw(renderer);
        for (auto& bullet : bullets) bullet.draw(renderer);
        for (auto& enemy : enemies) enemy.draw(renderer);

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
