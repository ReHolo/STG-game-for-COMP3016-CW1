#include <iostream>
#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_image.h>
#include <vector>
#include <string>
#include <cstdlib>
#include <ctime>

struct GameObject {
    SDL_Rect rect;
    SDL_Texture* texture;
};

struct Bullet {
    SDL_Rect rect;
    int speed;
    bool isPlayerBullet;
};

enum GameState {
    MAIN_MENU,
    PLAYING,
    GAME_OVER
};

GameState gameState = MAIN_MENU;
SDL_Texture* backgroundTexture = nullptr;
SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;
SDL_Texture* playerTexture = nullptr;
SDL_Texture* enemyTexture = nullptr;
std::vector<GameObject> enemies;
std::vector<Bullet> bullets;
int playerLives = 3;
int score = 0;
int enemySpawnRate = 3000;
Uint32 lastEnemySpawnTime = 0;
const int FPS = 60;
const int frameDelay = 1000 / FPS;
SDL_Rect playerRect = { 375, 550, 50, 50 };
Uint32 lastEnemyFireTime = 0;
const int enemyFireRate = 2000;
int enemyKillCount = 0;
Uint32 gameStartTime = 0;
Uint32 finalGameTime = 0; // Holds the final game time when the player dies

bool init() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }

    window = SDL_CreateWindow("Bullet Hell Game", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600, SDL_WINDOW_SHOWN);
    if (window == nullptr) {
        std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == nullptr) {
        std::cerr << "Renderer could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }

    int imgFlags = IMG_INIT_PNG;
    if (!(IMG_Init(imgFlags) & imgFlags)) {
        std::cerr << "SDL_image could not initialize! SDL_image Error: " << IMG_GetError() << std::endl;
        return false;
    }

    if (TTF_Init() == -1) {
        std::cerr << "SDL_ttf could not initialize! SDL_ttf Error: " << TTF_GetError() << std::endl;
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

SDL_Texture* renderText(const std::string& message, SDL_Color color, int fontSize, SDL_Renderer* renderer) {
    TTF_Font* font = TTF_OpenFont("constan.ttf", fontSize);
    if (font == nullptr) {
        std::cerr << "Failed to load font: " << TTF_GetError() << std::endl;
        return nullptr;
    }

    SDL_Surface* surf = TTF_RenderText_Blended(font, message.c_str(), color);
    if (surf == nullptr) {
        TTF_CloseFont(font);
        std::cerr << "Failed to render text: " << TTF_GetError() << std::endl;
        return nullptr;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surf);
    if (texture == nullptr) {
        std::cerr << "Failed to create texture: " << SDL_GetError() << std::endl;
    }

    SDL_FreeSurface(surf);
    TTF_CloseFont(font);
    return texture;
}

void renderText(const std::string& message, int x, int y) {
    SDL_Color color = { 255, 255, 255, 255 };
    SDL_Texture* texture = renderText(message, color, 24, renderer);
    if (texture == nullptr) {
        return;
    }

    int textWidth = 0;
    int textHeight = 0;
    SDL_QueryTexture(texture, nullptr, nullptr, &textWidth, &textHeight);
    SDL_Rect renderQuad = { x, y, textWidth, textHeight };

    SDL_RenderCopy(renderer, texture, nullptr, &renderQuad);
    SDL_DestroyTexture(texture);
}

void close() {
    SDL_DestroyTexture(playerTexture);
    SDL_DestroyTexture(enemyTexture);
    playerTexture = nullptr;
    enemyTexture = nullptr;

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    window = nullptr;
    renderer = nullptr;

    IMG_Quit();
    TTF_Quit();
    SDL_Quit();
}

void spawnEnemy() {
    GameObject enemy;
    enemy.rect = { rand() % (800 - 50), 0, 50, 50 };
    enemy.texture = enemyTexture;
    enemies.push_back(enemy);
}

void fireBullet(SDL_Rect& rect, bool isPlayerBullet) {
    Bullet bullet;
    bullet.rect = { rect.x + rect.w / 2 - 5, rect.y, 10, 20 };
    bullet.speed = isPlayerBullet ? -10 : 5;
    bullet.isPlayerBullet = isPlayerBullet;
    bullets.push_back(bullet);
}

void resetGame() {
    // 重置游戏状态
    gameState = MAIN_MENU;
    playerLives = 3;
    score = 0;
    enemySpawnRate = 3000; // 恢复默认敌人生成速度
    lastEnemySpawnTime = 0;
    enemies.clear();
    bullets.clear();
    playerRect = { 375, 550, 50, 50 };
    gameStartTime = SDL_GetTicks();
    finalGameTime = 0;
    enemyKillCount = 0;
}

void handleEvents(bool& quit, SDL_Rect& playerRect) {
    SDL_Event e;
    while (SDL_PollEvent(&e) != 0) {
        if (e.type == SDL_QUIT) {
            quit = true;
        }
        else if (e.type == SDL_KEYDOWN) {
            if (e.key.keysym.sym == SDLK_SPACE) {
                fireBullet(playerRect, true);
            }
        }
    }

    const Uint8* currentKeyStates = SDL_GetKeyboardState(nullptr);
    int moveX = 0;
    int moveY = 0;
    if (currentKeyStates[SDL_SCANCODE_UP]) moveY = -5;
    if (currentKeyStates[SDL_SCANCODE_DOWN]) moveY = 5;
    if (currentKeyStates[SDL_SCANCODE_LEFT]) moveX = -5;
    if (currentKeyStates[SDL_SCANCODE_RIGHT]) moveX = 5;

    playerRect.x += moveX;
    playerRect.y += moveY;

    if (playerRect.y < 0) playerRect.y = 0;
    if (playerRect.y + playerRect.h > 600) playerRect.y = 600 - playerRect.h;
    if (playerRect.x < 0) playerRect.x = 0;
    if (playerRect.x + playerRect.w > 800) playerRect.x = 800 - playerRect.w;
}

void update() {
    for (auto it = bullets.begin(); it != bullets.end();) {
        it->rect.y += it->speed;
        if (it->rect.y < 0 || it->rect.y > 600) {
            it = bullets.erase(it);
        }
        else {
            ++it;
        }
    }

    // Update enemies and check if they reach the bottom of the screen
    for (auto it = enemies.begin(); it != enemies.end();) {
        it->rect.y += 2;

        // Check if enemy has reached the bottom of the screen
        if (it->rect.y >= 600) { // Assuming 600 is the screen height
            playerLives--; // Reduce player lives
            it = enemies.erase(it); // Remove enemy that reached the bottom
        }
        else {
            ++it;
        }
    }

    // Handle collisions between player bullets and enemies
    for (auto it = bullets.begin(); it != bullets.end();) {
        bool bulletRemoved = false;
        if (it->isPlayerBullet) {
            for (auto enemyIt = enemies.begin(); enemyIt != enemies.end();) {
                if (SDL_HasIntersection(&it->rect, &enemyIt->rect)) {
                    enemyIt = enemies.erase(enemyIt);
                    it = bullets.erase(it);
                    bulletRemoved = true;
                    score += 10;
                    enemyKillCount++;
                    break;
                }
                else {
                    ++enemyIt;
                }
            }
        }
        else {
            if (SDL_HasIntersection(&it->rect, &playerRect)) {
                playerLives--;
                it = bullets.erase(it);
                bulletRemoved = true;
            }
        }
        if (!bulletRemoved) ++it;
    }

    // Check for game over condition
    if (playerLives <= 0 && gameState != GAME_OVER) {
        gameState = GAME_OVER;
        finalGameTime = (SDL_GetTicks() - gameStartTime) / 1000; // Store the final game time in seconds
    }

    // Spawn enemies and manage enemy fire if the game is in PLAYING state
    if (gameState == PLAYING) {
        Uint32 currentTime = SDL_GetTicks();
        if (currentTime > lastEnemySpawnTime + enemySpawnRate) {
            spawnEnemy();
            lastEnemySpawnTime = currentTime;
        }

        if (enemySpawnRate > 1000) enemySpawnRate -= 1;

        if (currentTime > lastEnemyFireTime + enemyFireRate) {
            for (auto& enemy : enemies) {
                fireBullet(enemy.rect, false);
            }
            lastEnemyFireTime = currentTime + rand() % 2000;
        }
    }
}


void renderMainMenu(SDL_Renderer* renderer) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    SDL_Color white = { 255, 255, 255, 255 };
    SDL_Color yellow = { 255, 255, 0, 255 };

    SDL_Texture* titleTexture = renderText("Bullet Hell Game", white, 48, renderer);
    if (titleTexture != nullptr) {
        int textWidth, textHeight;
        SDL_QueryTexture(titleTexture, nullptr, nullptr, &textWidth, &textHeight);
        SDL_Rect titleRect = { 400 - textWidth / 2, 150, textWidth, textHeight };
        SDL_RenderCopy(renderer, titleTexture, nullptr, &titleRect);
        SDL_DestroyTexture(titleTexture);
    }

    SDL_Texture* startTexture = renderText("Start Game", yellow, 32, renderer);
    if (startTexture != nullptr) {
        int textWidth, textHeight;
        SDL_QueryTexture(startTexture, nullptr, nullptr, &textWidth, &textHeight);
        SDL_Rect startRect = { 400 - textWidth / 2, 300, textWidth, textHeight };
        SDL_RenderCopy(renderer, startTexture, nullptr, &startRect);
        SDL_DestroyTexture(startTexture);
    }

    SDL_Texture* exitTexture = renderText("Exit Game", white, 32, renderer);
    if (exitTexture != nullptr) {
        int textWidth, textHeight;
        SDL_QueryTexture(exitTexture, nullptr, nullptr, &textWidth, &textHeight);
        SDL_Rect exitRect = { 400 - textWidth / 2, 360, textWidth, textHeight };
        SDL_RenderCopy(renderer, exitTexture, nullptr, &exitRect);
        SDL_DestroyTexture(exitTexture);
    }

    SDL_RenderPresent(renderer);
}

void handleMainMenuEvents(bool& quit, GameState& gameState) {
    SDL_Event e;
    int mouseX, mouseY;

    // Define the positions for "Start Game" and "Exit Game" text
    SDL_Rect startGameRect = { 400 - 75, 300, 150, 32 }; // Adjust width/height based on text size
    SDL_Rect exitGameRect = { 400 - 75, 360, 150, 32 }; // Adjust width/height based on text size

    while (SDL_PollEvent(&e) != 0) {
        if (e.type == SDL_QUIT) {
            quit = true;
        }
        else if (e.type == SDL_MOUSEBUTTONDOWN) {
            SDL_GetMouseState(&mouseX, &mouseY);

            // Check if the mouse click is within "Start Game"
            if (mouseX >= startGameRect.x && mouseX <= startGameRect.x + startGameRect.w &&
                mouseY >= startGameRect.y && mouseY <= startGameRect.y + startGameRect.h) {
                gameState = PLAYING;
                gameStartTime = SDL_GetTicks();
            }

            // Check if the mouse click is within "Exit Game"
            if (mouseX >= exitGameRect.x && mouseX <= exitGameRect.x + exitGameRect.w &&
                mouseY >= exitGameRect.y && mouseY <= exitGameRect.y + exitGameRect.h) {
                quit = true;
            }
        }
    }
}

void renderGameOver() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    SDL_Color red = { 255, 0, 0, 255 };
    SDL_Color white = { 255, 255, 255, 255 };

    // Render "Game Over" message
    SDL_Texture* gameOverTexture = renderText("Game Over", red, 48, renderer);
    if (gameOverTexture != nullptr) {
        int textWidth, textHeight;
        SDL_QueryTexture(gameOverTexture, nullptr, nullptr, &textWidth, &textHeight);
        SDL_Rect renderQuad = { 400 - textWidth / 2, 150, textWidth, textHeight };
        SDL_RenderCopy(renderer, gameOverTexture, nullptr, &renderQuad);
        SDL_DestroyTexture(gameOverTexture);
    }

    // Display the number of enemies killed
    std::string enemiesKilledText = "Enemies Killed: " + std::to_string(enemyKillCount);
    SDL_Texture* killCountTexture = renderText(enemiesKilledText, white, 32, renderer);
    if (killCountTexture != nullptr) {
        int textWidth, textHeight;
        SDL_QueryTexture(killCountTexture, nullptr, nullptr, &textWidth, &textHeight);
        SDL_Rect renderQuad = { 400 - textWidth / 2, 250, textWidth, textHeight };
        SDL_RenderCopy(renderer, killCountTexture, nullptr, &renderQuad);
        SDL_DestroyTexture(killCountTexture);
    }

    // Display the final game time
    std::string timeText = "Time: " + std::to_string(finalGameTime) + "s";
    SDL_Texture* timeTexture = renderText(timeText, white, 32, renderer);
    if (timeTexture != nullptr) {
        int textWidth, textHeight;
        SDL_QueryTexture(timeTexture, nullptr, nullptr, &textWidth, &textHeight);
        SDL_Rect renderQuad = { 400 - textWidth / 2, 300, textWidth, textHeight };
        SDL_RenderCopy(renderer, timeTexture, nullptr, &renderQuad);
        SDL_DestroyTexture(timeTexture);
    }

    // Render "Return to Main Menu" option
    SDL_Texture* returnTexture = renderText("Return to Main Menu", white, 32, renderer);
    if (returnTexture != nullptr) {
        int textWidth, textHeight;
        SDL_QueryTexture(returnTexture, nullptr, nullptr, &textWidth, &textHeight);
        SDL_Rect returnRect = { 400 - textWidth / 2, 400, textWidth, textHeight };
        SDL_RenderCopy(renderer, returnTexture, nullptr, &returnRect);
        SDL_DestroyTexture(returnTexture);
    }

    SDL_RenderPresent(renderer);
}


void handleGameOverEvents(bool& quit) {
    SDL_Event e;
    int mouseX, mouseY;

    // Define the position for "Return to Main Menu" text
    SDL_Rect returnMenuRect = { 400 - 75, 400, 150, 32 }; // Adjust width/height based on text size

    while (SDL_PollEvent(&e) != 0) {
        if (e.type == SDL_QUIT) {
            quit = true;
        }
        else if (e.type == SDL_MOUSEBUTTONDOWN) {
            SDL_GetMouseState(&mouseX, &mouseY);

            // Check if the mouse click is within "Return to Main Menu"
            if (mouseX >= returnMenuRect.x && mouseX <= returnMenuRect.x + returnMenuRect.w &&
                mouseY >= returnMenuRect.y && mouseY <= returnMenuRect.y + returnMenuRect.h) {

                // Reset game variables and return to main menu
				resetGame();
            }
        }
    }
}


void render() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    SDL_RenderCopy(renderer, playerTexture, nullptr, &playerRect);

    for (const auto& enemy : enemies) {
        SDL_RenderCopy(renderer, enemy.texture, nullptr, &enemy.rect);
    }

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    for (const auto& bullet : bullets) {
        SDL_RenderFillRect(renderer, &bullet.rect);
    }

    renderText("Lives: " + std::to_string(playerLives), 10, 10);
    renderText("Enemies Killed: " + std::to_string(enemyKillCount), 10, 40);
    Uint32 gameTime = (SDL_GetTicks() - gameStartTime) / 1000;
    renderText("Time: " + std::to_string(gameTime) + "s", 10, 70);

    SDL_RenderPresent(renderer);
}

int main(int argc, char* args[]) {
    if (!init()) {
        std::cerr << "Failed to initialize!" << std::endl;
        return -1;
    }

    playerTexture = loadTexture("player.png");
    enemyTexture = loadTexture("enemy.png");
    if (playerTexture == nullptr || enemyTexture == nullptr) {
        std::cerr << "Failed to load textures!" << std::endl;
        return -1;
    }

    bool quit = false;

    while (!quit) {
        Uint32 frameStart = SDL_GetTicks();

        switch (gameState) {
        case MAIN_MENU:
            renderMainMenu(renderer);
            handleMainMenuEvents(quit, gameState);
            break;
        case PLAYING:
            handleEvents(quit, playerRect);
            update();
            render();
            break;
        case GAME_OVER:
            renderGameOver();
            handleGameOverEvents(quit);
            break;
        }

        Uint32 frameTime = SDL_GetTicks() - frameStart;
        if (frameDelay > frameTime) {
            SDL_Delay(frameDelay - frameTime);
        }
    }

    close();
    return 0;
}
