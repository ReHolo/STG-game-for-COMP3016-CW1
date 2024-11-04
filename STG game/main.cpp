#define SDL_MAIN_HANDLED
#include <iostream>
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <vector>
#include <string>
#include <cstdlib>
#include <ctime>
#include <windows.h>
#include <algorithm>

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;

class GameObject {
public:
    SDL_Rect rect;
    SDL_Texture* texture;

    GameObject(int x, int y, int w, int h, SDL_Texture* tex) : rect{ x, y, w, h }, texture(tex) {}
    virtual void update() {}
    virtual void render(SDL_Renderer* renderer) {
        if (texture) {
            SDL_RenderCopy(renderer, texture, nullptr, &rect);
        }
    }
};

class Bullet : public GameObject {
public:
    int speedY;
    int speedX; // 水平速度
    bool isPlayerBullet;

    Bullet(int x, int y, int spdY, bool isPlayer, int spdX = 0)
        : GameObject(x, y, 5, 10, nullptr), speedY(spdY), speedX(spdX), isPlayerBullet(isPlayer) {}

    void update() override {
        rect.y += speedY;
        rect.x += speedX; // 更新水平位置
    }

    void render(SDL_Renderer* renderer) override {
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderFillRect(renderer, &rect);
    }
};


class Player : public GameObject {
public:
    int lives;
    Uint32 lastShotTime;
    Uint32 shotInterval; // 将 shotInterval 改为普通成员变量
    int extraBulletCount = 0; // 额外弹幕的数量
    int enemyKillCount = 0;   // 击杀敌人的计数器

    Player(int x, int y, int w, int h, SDL_Texture* tex, int lv)
        : GameObject(x, y, w, h, tex), lives(lv), lastShotTime(0), shotInterval(300) {}

    void handleInput(const Uint8* currentKeyStates, std::vector<Bullet>& bullets) {
        int moveX = 0;
        int moveY = 0;
        if (currentKeyStates[SDL_SCANCODE_UP]) moveY = -5;
        if (currentKeyStates[SDL_SCANCODE_DOWN]) moveY = 5;
        if (currentKeyStates[SDL_SCANCODE_LEFT]) moveX = -5;
        if (currentKeyStates[SDL_SCANCODE_RIGHT]) moveX = 5;

        rect.x += moveX;
        rect.y += moveY;

        if (rect.y < 0) rect.y = 0;
        if (rect.y + rect.h > SCREEN_HEIGHT) rect.y = SCREEN_HEIGHT - rect.h;
        if (rect.x < 0) rect.x = 0;
        if (rect.x + rect.w > SCREEN_WIDTH) rect.x = SCREEN_WIDTH - rect.w;

        Uint32 currentTime = SDL_GetTicks();
        if (currentKeyStates[SDL_SCANCODE_SPACE] && currentTime - lastShotTime >= shotInterval) {
            // 发射主弹幕
            bullets.emplace_back(rect.x + rect.w / 2 - 2, rect.y, -10, true);

            // 根据 extraBulletCount 增加额外的斜方向弹幕
            for (int i = 0; i < extraBulletCount; ++i) {
                int offset = 5 + (i * 5); // 每个额外弹幕的偏移量
                bullets.emplace_back(rect.x + rect.w / 2 - 2, rect.y, -10, true, -offset); // 左斜弹幕
                bullets.emplace_back(rect.x + rect.w / 2 - 2, rect.y, -10, true, offset);  // 右斜弹幕
            }

            lastShotTime = currentTime;
        }
    }

    // 增加击杀计数器，每击杀15个敌人增加额外弹幕
    void increaseKillCount() {
        enemyKillCount++; // 先增加击杀计数

        // 每20个敌人增加一对弹幕，最多增加两对
        if (enemyKillCount % 20 == 0 && extraBulletCount < 2) {
            extraBulletCount++;
        }

        // 每击杀10个敌人，减少射击间隔，最小不低于100毫秒
        if (enemyKillCount % 1 == 0) {
            shotInterval = max(shotInterval - 5, 100U);
        }
    }
};

class Enemy : public GameObject {
public:
    Uint32 lastShotTime;
    Uint32 shootInterval;

    Enemy(int x, int y, int w, int h, SDL_Texture* tex)
        : GameObject(x, y, w, h, tex), lastShotTime(0) {
        shootInterval = 1000 + rand() % 2000;
    }

    void update() override {
        rect.y += 2;
    }

    void fireBullet(std::vector<Bullet>& bullets) {
        bullets.emplace_back(rect.x + rect.w / 2 - 2, rect.y + rect.h, 5, false);
    }
};

enum GameState {
    MAIN_MENU,
    PLAYING,
    GAME_OVER
};

class Game {
public:
    GameState gameState;

    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* playerTexture;
    SDL_Texture* enemyTexture;
    TTF_Font* font;

    std::vector<Enemy> enemies;
    std::vector<Bullet> bullets;
    Player* player;
    int score;
    int enemySpawnRate;
    const int minSpawnRate;
    Uint32 lastEnemySpawnTime;
    const int FPS;
    int frameDelay;
    Uint32 lastEnemyFireTime;
    int enemyKillCount;
    Uint32 gameStartTime;
    Uint32 finalGameTime;

    SDL_Rect startButtonRect;
    SDL_Rect quitButtonRect;
    SDL_Rect returnButtonRect;

    Game() : gameState(MAIN_MENU),
        window(nullptr),
        renderer(nullptr),
        playerTexture(nullptr),
        enemyTexture(nullptr),
        font(nullptr),
        player(nullptr), score(0),
        enemySpawnRate(3000),
        minSpawnRate(200),
        lastEnemySpawnTime(0),
        FPS(60),
        frameDelay(1000 / FPS),
        lastEnemyFireTime(0),
        enemyKillCount(0),
        gameStartTime(0),
        finalGameTime(0),
        startButtonRect{ 350, 250, 100, 50 },
        quitButtonRect{ 350, 350, 100, 50 },
        returnButtonRect{ 350, 450, 100, 50 } {
    }

    bool init() {
        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
            std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
            return false;
        }

        window = SDL_CreateWindow("Bullet Hell Game", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
        if (window == nullptr) {
            std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
            return false;
        }

        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
        if (renderer == nullptr) {
            std::cerr << "Renderer could not be created! SDL_Error: " << SDL_GetError() << std::endl;
            return false;
        }

        if (TTF_Init() == -1) {
            std::cerr << "SDL_ttf could not initialize! SDL_ttf Error: " << TTF_GetError() << std::endl;
            return false;
        }

        font = TTF_OpenFont("constan.ttf", 24);
        if (font == nullptr) {
            std::cerr << "Failed to load font! TTF_Error: " << TTF_GetError() << std::endl;
            return false;
        }

        return true;
    }

    SDL_Texture* loadTexture(const std::string& path) {
        SDL_Texture* newTexture = nullptr;
        SDL_Surface* loadedSurface = IMG_Load(path.c_str());
        if (loadedSurface == nullptr) {
            std::cerr << "Unable to load image " << path << "! SDL_image Error: " << IMG_GetError() << std::endl;
        }
        else {
            newTexture = SDL_CreateTextureFromSurface(renderer, loadedSurface);
            if (newTexture == nullptr) {
                std::cerr << "Unable to create texture from " << path << "! SDL Error: " << SDL_GetError() << std::endl;
            }
            SDL_FreeSurface(loadedSurface);
        }
        return newTexture;
    }

    void renderText(const std::string& message, int x, int y, SDL_Color color, bool centered = false, int fontSize = 24) {
        // 加载指定大小的字体
        TTF_Font* tempFont = TTF_OpenFont("constan.ttf", fontSize);
        if (tempFont == nullptr) {
            std::cerr << "Failed to load font! TTF_Error: " << TTF_GetError() << std::endl;
            return;
        }

        SDL_Surface* textSurface = TTF_RenderText_Solid(tempFont, message.c_str(), color);
        if (textSurface != nullptr) {
            SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);

            SDL_Rect renderQuad = { x, y, textSurface->w, textSurface->h };

            // 如果需要居中，则调整 x 坐标
            if (centered) {
                renderQuad.x = (SCREEN_WIDTH - textSurface->w) / 2;
            }

            SDL_RenderCopy(renderer, textTexture, nullptr, &renderQuad);

            SDL_DestroyTexture(textTexture);
            SDL_FreeSurface(textSurface);
        }
        else {
            std::cerr << "Unable to render text surface! SDL_ttf Error: " << TTF_GetError() << std::endl;
        }

        // 关闭临时字体
        TTF_CloseFont(tempFont);
    }

    void renderButton(const std::string& message, SDL_Rect& rect, SDL_Color textColor) {
        // 渲染按钮文本，获取文本的宽度和高度
        SDL_Surface* textSurface = TTF_RenderText_Solid(font, message.c_str(), textColor);
        if (textSurface != nullptr) {
            SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);

            // 设置按钮的边距
            int padding = 20;
            rect.w = textSurface->w + padding * 2;
            rect.h = textSurface->h + padding * 2;
            rect.x = (SCREEN_WIDTH - rect.w) / 2;  // 居中按钮

            // 渲染按钮背景
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_RenderFillRect(renderer, &rect);

            // 渲染按钮边框
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderDrawRect(renderer, &rect);

            // 渲染文本，使其居中在按钮内
            SDL_Rect textRect = { rect.x + padding, rect.y + padding, textSurface->w, textSurface->h };
            SDL_RenderCopy(renderer, textTexture, nullptr, &textRect);

            SDL_DestroyTexture(textTexture);
            SDL_FreeSurface(textSurface);
        }
    }

    void renderMainMenu() {
        SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF);
        SDL_RenderClear(renderer);

        SDL_Color white = { 255, 255, 255, 255 };
        SDL_Color red = { 255, 0, 0, 255 };
		SDL_Color black = { 0, 0, 0, 0 };

        // 居中显示游戏标题
        renderText("Bullet Hell Game", 0, 100, red, true,72);

        // 使用 renderButton 函数渲染按钮
        renderButton("Start Game", startButtonRect, black);
        renderButton("Quit Game", quitButtonRect, black);

        SDL_RenderPresent(renderer);
    }

    void renderGameOver() {
        SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF);
        SDL_RenderClear(renderer);

        SDL_Color white = { 255, 255, 255, 255 };
        SDL_Color red = { 255, 0, 0, 255 };
		SDL_Color black = { 0, 0, 0, 0 };

        // 居中显示游戏结束信息
        renderText("Game Over", 0, 150, red, true,72);

        // 显示分数、时间和杀敌数
        renderText("Score: " + std::to_string(score), 0, 250, white, true);
        renderText("Time: " + std::to_string(finalGameTime) + "s", 0, 300, white, true);
        renderText("Enemies Killed: " + std::to_string(enemyKillCount), 0, 350, white, true);

        // 显示返回主菜单按钮
        renderButton("Return to Main Menu", returnButtonRect, black);

        SDL_RenderPresent(renderer);
    }

    void renderHUD() {
        SDL_Color white = { 255, 255, 255, 255 };

        // 显示生命值、杀敌数和时间
        renderText("Lives: " + std::to_string(player->lives), 10, 10, white);
        renderText("Kills: " + std::to_string(enemyKillCount), 10, 40, white);
        Uint32 elapsedTime = (SDL_GetTicks() - gameStartTime) / 1000;
        renderText("Time: " + std::to_string(elapsedTime) + "s", 10, 70, white);
    }

    void render() {
        if (gameState == GAME_OVER) {
            renderGameOver();
        }
        else if (gameState == MAIN_MENU) {
            renderMainMenu();
        }
        else {
            SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF);
            SDL_RenderClear(renderer);

            player->render(renderer);
            for (auto& bullet : bullets) {
                bullet.render(renderer);
            }
            for (auto& enemy : enemies) {
                enemy.render(renderer);
            }

            renderHUD();

            SDL_RenderPresent(renderer);
        }
    }

    void close() {
        SDL_DestroyTexture(playerTexture);
        SDL_DestroyTexture(enemyTexture);
        TTF_CloseFont(font);

        playerTexture = nullptr;
        enemyTexture = nullptr;
        font = nullptr;

        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        renderer = nullptr;
        window = nullptr;

        TTF_Quit();
        SDL_Quit();
    }

    void resetGame() {
        player = new Player(400, 500, 50, 50, playerTexture, 3);
        enemies.clear();
        bullets.clear();
        score = 0;
        enemySpawnRate = 3000;
        lastEnemySpawnTime = 0;
        lastEnemyFireTime = 0;
        enemyKillCount = 0;
        gameStartTime = SDL_GetTicks();
        finalGameTime = 0;
    }

    void update() {
        if (gameState == GAME_OVER) {
            return;
        }

        // 更新子弹位置
        for (auto bulletIt = bullets.begin(); bulletIt != bullets.end();) {
            bulletIt->update();
            if (bulletIt->rect.y < 0 || bulletIt->rect.y > SCREEN_HEIGHT) {
                bulletIt = bullets.erase(bulletIt);  // 移除超出屏幕的子弹
            }
            else {
                ++bulletIt;
            }
        }

        // 更新敌人位置
        for (auto& enemy : enemies) {
            enemy.update();

            // 敌人随机发射子弹
            Uint32 currentTime = SDL_GetTicks();
            if (currentTime - enemy.lastShotTime > enemy.shootInterval) {
                enemy.fireBullet(bullets);
                enemy.lastShotTime = currentTime;
            }
        }

        // 检测子弹与敌人的碰撞
        checkBulletEnemyCollision();

        // 检测玩家与敌人的碰撞
        checkPlayerEnemyCollision();

        // 检测敌人子弹与玩家的碰撞
        checkBulletPlayerCollision();

        // 检测敌人是否到达屏幕底部
        checkEnemyBottomCollision();

        // 生成新的敌人
        spawnEnemy();

        // 检查玩家生命值，若为0则进入游戏结束状态
        if (player->lives <= 0) {
            gameState = GAME_OVER;
            finalGameTime = (SDL_GetTicks() - gameStartTime) / 1000;  // 记录游戏结束时间（秒）
        }
    }

    void handleMouseClick(int x, int y, bool& quit) {
        if (gameState == MAIN_MENU) {
            // 检查是否点击了 "Start Game" 按钮
            if (x >= startButtonRect.x && x <= startButtonRect.x + startButtonRect.w &&
                y >= startButtonRect.y && y <= startButtonRect.y + startButtonRect.h) {
                gameState = PLAYING;           // 切换到游戏进行状态
                gameStartTime = SDL_GetTicks(); // 记录游戏开始时间
                resetGame();                    // 重置游戏数据
            }
            // 检查是否点击了 "Quit Game" 按钮
            else if (x >= quitButtonRect.x && x <= quitButtonRect.x + quitButtonRect.w &&
                y >= quitButtonRect.y && y <= quitButtonRect.y + quitButtonRect.h) {
                quit = true; // 退出游戏
            }
        }
        else if (gameState == GAME_OVER) {
            // 检查是否点击了 "Return to Main Menu" 按钮
            if (x >= returnButtonRect.x && x <= returnButtonRect.x + returnButtonRect.w &&
                y >= returnButtonRect.y && y <= returnButtonRect.y + returnButtonRect.h) {
                gameState = MAIN_MENU; // 返回到主菜单
                resetGame();           // 重置游戏数据
            }
        }
    }


    void spawnEnemy() {
        Uint32 currentTime = SDL_GetTicks();
        if (currentTime - lastEnemySpawnTime > static_cast<Uint32>(enemySpawnRate)) {
            int x = rand() % (SCREEN_WIDTH - 50); // 随机生成敌人的 x 坐标
            enemies.emplace_back(x, 0, 50, 50, enemyTexture); // 在顶部生成新的敌人
            lastEnemySpawnTime = currentTime; // 更新上一次生成敌人的时间

            // 随着时间推移，逐渐减少敌人生成间隔，加快生成速度
            if (enemySpawnRate > minSpawnRate) {
                enemySpawnRate -= 100;
            }
        }
    }


    void handleEvents(bool& quit) {
        SDL_Event e;
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                quit = true;
            }
            else if (e.type == SDL_MOUSEBUTTONDOWN) {
                int x, y;
                SDL_GetMouseState(&x, &y);
                handleMouseClick(x, y, quit);
            }
        }

        if (gameState == PLAYING) {
            const Uint8* currentKeyStates = SDL_GetKeyboardState(nullptr);
            player->handleInput(currentKeyStates, bullets);
        }
    }

    void checkBulletEnemyCollision() {
        for (auto bulletIt = bullets.begin(); bulletIt != bullets.end();) {
            bool bulletRemoved = false;
            for (auto enemyIt = enemies.begin(); enemyIt != enemies.end();) {
                if (bulletIt->isPlayerBullet && SDL_HasIntersection(&bulletIt->rect, &enemyIt->rect)) {
                    bulletIt = bullets.erase(bulletIt);  // 移除子弹
                    enemyIt = enemies.erase(enemyIt);    // 移除敌人
                    score += 100;                       // 增加分数
                    enemyKillCount++;                   // 更新击杀计数
                    player->increaseKillCount();        // 检查是否需要增加额外弹幕
                    bulletRemoved = true;
                    break;
                }
                else {
                    ++enemyIt;
                }
            }
            if (!bulletRemoved) {
                ++bulletIt;
            }
        }
    }



    void checkPlayerEnemyCollision() {
        for (auto enemyIt = enemies.begin(); enemyIt != enemies.end();) {
            if (SDL_HasIntersection(&player->rect, &enemyIt->rect)) {
                enemyIt = enemies.erase(enemyIt);  // 移除敌人
                player->lives--;                  // 减少玩家生命值
                if (player->lives <= 0) {
                    gameState = GAME_OVER;        // 切换到游戏结束状态
                }
            }
            else {
                ++enemyIt;
            }
        }
    }

    void checkBulletPlayerCollision() {
        for (auto bulletIt = bullets.begin(); bulletIt != bullets.end();) {
            if (!bulletIt->isPlayerBullet && SDL_HasIntersection(&bulletIt->rect, &player->rect)) {
                bulletIt = bullets.erase(bulletIt);  // 移除敌人子弹
                player->lives--;                    // 减少玩家生命值
                if (player->lives <= 0) {
                    gameState = GAME_OVER;          // 切换到游戏结束状态
                }
            }
            else {
                ++bulletIt;
            }
        }
    }

    void checkEnemyBottomCollision() {
        for (auto enemyIt = enemies.begin(); enemyIt != enemies.end();) {
            if (enemyIt->rect.y + enemyIt->rect.h >= SCREEN_HEIGHT) {
                enemyIt = enemies.erase(enemyIt);  // 移除敌人
                player->lives--;                  // 扣除玩家生命值
                if (player->lives <= 0) {
                    gameState = GAME_OVER;        // 切换到游戏结束状态
                }
            }
            else {
                ++enemyIt;
            }
        }
    }

};

int main(int argc, char* args[]) {

    HWND hwnd = GetConsoleWindow();
    ShowWindow(hwnd, SW_HIDE);

    Game game;

    // 初始化游戏
    if (!game.init()) {
        std::cerr << "Failed to initialize!" << std::endl;
        SDL_Log("Program started");
        return -1;
    }

    // 加载资源
    game.playerTexture = game.loadTexture("player.png");
    game.enemyTexture = game.loadTexture("enemy.png");

    if (game.playerTexture == nullptr || game.enemyTexture == nullptr) {
        std::cerr << "Failed to load textures!" << std::endl;
        game.close();
        return -1;
    }

    // 初始化玩家和游戏对象
    game.player = new Player(375, 500, 50, 50, game.playerTexture, 3);
    game.enemies.clear();
    game.bullets.clear();

    // 主循环
    bool quit = false;
    while (!quit) {
        game.handleEvents(quit);

        switch (game.gameState) {
        case MAIN_MENU:
            game.renderMainMenu();
            break;

        case PLAYING:
            game.update();
            game.render();
            break;

        case GAME_OVER:
            game.renderGameOver();
            break;

        default:
            break;
        }

        SDL_Delay(game.frameDelay);
    }

    // 释放资源并关闭游戏
    game.close();
    return 0;
}
