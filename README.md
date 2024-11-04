# Bullet Hell Game

This project is a classic bullet hell game developed using SDL2, SDL_ttf, and SDL_image libraries in C++. The game involves a player who can shoot bullets at incoming enemies while dodging their attacks. The player's goal is to survive as long as possible while defeating enemies and increasing their score.

## Features

- **Main Menu**: Options to start the game or quit.
- **Gameplay**:
  - Control a player character who can shoot bullets.
  - Enemies spawn periodically and move toward the player.
  - Each enemy kill decreases shot interval and, after 15 kills, increases the number of bullets fired by the player.
  - Player loses life points when colliding with enemies or enemy bullets.
- **Game Over**: Displays the player's score, time survived, and number of enemies killed. Option to return to the main menu.

## Controls

- **Arrow Keys**: Move the player.
- **Space**: Shoot bullets.

## Libraries Required

- **SDL2**: Provides core graphics, sound, and input handling.
- **SDL_ttf**: Enables text rendering.
- **SDL_image**: Handles image loading for textures.

## File Structure

- **main.cpp**: Contains the main game logic and classes for `Game`, `Player`, `Bullet`, `Enemy`, and other game components.

## How to Run

1. Ensure SDL2, SDL_ttf, and SDL_image are installed and linked in your environment.
2. Build and run `main.cpp` using a compatible C++ compiler.
3. Make sure `player.png` and `enemy.png` images are available in the same directory.

## Class Overview

- **Game**: The main controller of the game, handles initialization, events, updates, and rendering.
- **Player**: Represents the player-controlled character with functions for movement and shooting.
- **Enemy**: Represents an enemy with automatic movement and random bullet shooting.
- **Bullet**: Represents bullets shot by both player and enemies.
- **GameObject**: Base class for `Player`, `Enemy`, and `Bullet` classes.

## Future Improvements

- Add more enemy types and bullet patterns.
- Implement power-ups for the player.
- Add sound effects and background music.
- Add difficulty levels.
![屏幕截图 2024-11-04 052842](https://github.com/user-attachments/assets/cf8dfc7e-f49c-4887-bc3a-86cf929eb291)
