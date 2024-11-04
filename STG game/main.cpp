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
    int speedX; // ˮƽ�ٶ�
    bool isPlayerBullet;

    Bullet(int x, int y, int spdY, bool isPlayer, int spdX = 0)
        : GameObject(x, y, 5, 10, nullptr), speedY(spdY), speedX(spdX), isPlayerBullet(isPlayer) {}

    void update() override {
        rect.y += speedY;
        rect.x += speedX; // ����ˮƽλ��
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
    Uint32 shotInterval; // �� shotInterval ��Ϊ��ͨ��Ա����
    int extraBulletCount = 0; // ���ⵯĻ������
    int enemyKillCount = 0;   // ��ɱ���˵ļ�����

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
            // ��������Ļ
            bullets.emplace_back(rect.x + rect.w / 2 - 2, rect.y, -10, true);

            // ���� extraBulletCount ���Ӷ����б����Ļ
            for (int i = 0; i < extraBulletCount; ++i) {
                int offset = 5 + (i * 5); // ÿ�����ⵯĻ��ƫ����
                bullets.emplace_back(rect.x + rect.w / 2 - 2, rect.y, -10, true, -offset); // ��б��Ļ
                bullets.emplace_back(rect.x + rect.w / 2 - 2, rect.y, -10, true, offset);  // ��б��Ļ
            }

            lastShotTime = currentTime;
        }
    }

    // ���ӻ�ɱ��������ÿ��ɱ15���������Ӷ��ⵯĻ
    void increaseKillCount() {
        enemyKillCount++; // �����ӻ�ɱ����

        // ÿ20����������һ�Ե�Ļ�������������
        if (enemyKillCount % 20 == 0 && extraBulletCount < 2) {
            extraBulletCount++;
        }

        // ÿ��ɱ10�����ˣ���������������С������100����
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
        // ����ָ����С������
        TTF_Font* tempFont = TTF_OpenFont("constan.ttf", fontSize);
        if (tempFont == nullptr) {
            std::cerr << "Failed to load font! TTF_Error: " << TTF_GetError() << std::endl;
            return;
        }

        SDL_Surface* textSurface = TTF_RenderText_Solid(tempFont, message.c_str(), color);
        if (textSurface != nullptr) {
            SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);

            SDL_Rect renderQuad = { x, y, textSurface->w, textSurface->h };

            // �����Ҫ���У������ x ����
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

        // �ر���ʱ����
        TTF_CloseFont(tempFont);
    }

    void renderButton(const std::string& message, SDL_Rect& rect, SDL_Color textColor) {
        // ��Ⱦ��ť�ı�����ȡ�ı��Ŀ�Ⱥ͸߶�
        SDL_Surface* textSurface = TTF_RenderText_Solid(font, message.c_str(), textColor);
        if (textSurface != nullptr) {
            SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);

            // ���ð�ť�ı߾�
            int padding = 20;
            rect.w = textSurface->w + padding * 2;
            rect.h = textSurface->h + padding * 2;
            rect.x = (SCREEN_WIDTH - rect.w) / 2;  // ���а�ť

            // ��Ⱦ��ť����
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_RenderFillRect(renderer, &rect);

            // ��Ⱦ��ť�߿�
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderDrawRect(renderer, &rect);

            // ��Ⱦ�ı���ʹ������ڰ�ť��
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

        // ������ʾ��Ϸ����
        renderText("Bullet Hell Game", 0, 100, red, true,72);

        // ʹ�� renderButton ������Ⱦ��ť
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

        // ������ʾ��Ϸ������Ϣ
        renderText("Game Over", 0, 150, red, true,72);

        // ��ʾ������ʱ���ɱ����
        renderText("Score: " + std::to_string(score), 0, 250, white, true);
        renderText("Time: " + std::to_string(finalGameTime) + "s", 0, 300, white, true);
        renderText("Enemies Killed: " + std::to_string(enemyKillCount), 0, 350, white, true);

        // ��ʾ�������˵���ť
        renderButton("Return to Main Menu", returnButtonRect, black);

        SDL_RenderPresent(renderer);
    }

    void renderHUD() {
        SDL_Color white = { 255, 255, 255, 255 };

        // ��ʾ����ֵ��ɱ������ʱ��
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

        // �����ӵ�λ��
        for (auto bulletIt = bullets.begin(); bulletIt != bullets.end();) {
            bulletIt->update();
            if (bulletIt->rect.y < 0 || bulletIt->rect.y > SCREEN_HEIGHT) {
                bulletIt = bullets.erase(bulletIt);  // �Ƴ�������Ļ���ӵ�
            }
            else {
                ++bulletIt;
            }
        }

        // ���µ���λ��
        for (auto& enemy : enemies) {
            enemy.update();

            // ������������ӵ�
            Uint32 currentTime = SDL_GetTicks();
            if (currentTime - enemy.lastShotTime > enemy.shootInterval) {
                enemy.fireBullet(bullets);
                enemy.lastShotTime = currentTime;
            }
        }

        // ����ӵ�����˵���ײ
        checkBulletEnemyCollision();

        // ����������˵���ײ
        checkPlayerEnemyCollision();

        // �������ӵ�����ҵ���ײ
        checkBulletPlayerCollision();

        // �������Ƿ񵽴���Ļ�ײ�
        checkEnemyBottomCollision();

        // �����µĵ���
        spawnEnemy();

        // ����������ֵ����Ϊ0�������Ϸ����״̬
        if (player->lives <= 0) {
            gameState = GAME_OVER;
            finalGameTime = (SDL_GetTicks() - gameStartTime) / 1000;  // ��¼��Ϸ����ʱ�䣨�룩
        }
    }

    void handleMouseClick(int x, int y, bool& quit) {
        if (gameState == MAIN_MENU) {
            // ����Ƿ����� "Start Game" ��ť
            if (x >= startButtonRect.x && x <= startButtonRect.x + startButtonRect.w &&
                y >= startButtonRect.y && y <= startButtonRect.y + startButtonRect.h) {
                gameState = PLAYING;           // �л�����Ϸ����״̬
                gameStartTime = SDL_GetTicks(); // ��¼��Ϸ��ʼʱ��
                resetGame();                    // ������Ϸ����
            }
            // ����Ƿ����� "Quit Game" ��ť
            else if (x >= quitButtonRect.x && x <= quitButtonRect.x + quitButtonRect.w &&
                y >= quitButtonRect.y && y <= quitButtonRect.y + quitButtonRect.h) {
                quit = true; // �˳���Ϸ
            }
        }
        else if (gameState == GAME_OVER) {
            // ����Ƿ����� "Return to Main Menu" ��ť
            if (x >= returnButtonRect.x && x <= returnButtonRect.x + returnButtonRect.w &&
                y >= returnButtonRect.y && y <= returnButtonRect.y + returnButtonRect.h) {
                gameState = MAIN_MENU; // ���ص����˵�
                resetGame();           // ������Ϸ����
            }
        }
    }


    void spawnEnemy() {
        Uint32 currentTime = SDL_GetTicks();
        if (currentTime - lastEnemySpawnTime > static_cast<Uint32>(enemySpawnRate)) {
            int x = rand() % (SCREEN_WIDTH - 50); // ������ɵ��˵� x ����
            enemies.emplace_back(x, 0, 50, 50, enemyTexture); // �ڶ��������µĵ���
            lastEnemySpawnTime = currentTime; // ������һ�����ɵ��˵�ʱ��

            // ����ʱ�����ƣ��𽥼��ٵ������ɼ�����ӿ������ٶ�
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
                    bulletIt = bullets.erase(bulletIt);  // �Ƴ��ӵ�
                    enemyIt = enemies.erase(enemyIt);    // �Ƴ�����
                    score += 100;                       // ���ӷ���
                    enemyKillCount++;                   // ���»�ɱ����
                    player->increaseKillCount();        // ����Ƿ���Ҫ���Ӷ��ⵯĻ
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
                enemyIt = enemies.erase(enemyIt);  // �Ƴ�����
                player->lives--;                  // �����������ֵ
                if (player->lives <= 0) {
                    gameState = GAME_OVER;        // �л�����Ϸ����״̬
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
                bulletIt = bullets.erase(bulletIt);  // �Ƴ������ӵ�
                player->lives--;                    // �����������ֵ
                if (player->lives <= 0) {
                    gameState = GAME_OVER;          // �л�����Ϸ����״̬
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
                enemyIt = enemies.erase(enemyIt);  // �Ƴ�����
                player->lives--;                  // �۳��������ֵ
                if (player->lives <= 0) {
                    gameState = GAME_OVER;        // �л�����Ϸ����״̬
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

    // ��ʼ����Ϸ
    if (!game.init()) {
        std::cerr << "Failed to initialize!" << std::endl;
        SDL_Log("Program started");
        return -1;
    }

    // ������Դ
    game.playerTexture = game.loadTexture("player.png");
    game.enemyTexture = game.loadTexture("enemy.png");

    if (game.playerTexture == nullptr || game.enemyTexture == nullptr) {
        std::cerr << "Failed to load textures!" << std::endl;
        game.close();
        return -1;
    }

    // ��ʼ����Һ���Ϸ����
    game.player = new Player(375, 500, 50, 50, game.playerTexture, 3);
    game.enemies.clear();
    game.bullets.clear();

    // ��ѭ��
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

    // �ͷ���Դ���ر���Ϸ
    game.close();
    return 0;
}
