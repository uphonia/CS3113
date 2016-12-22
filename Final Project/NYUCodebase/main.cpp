/*
 
 Angela Wu N14475962
 CS3113
 Final Project
 
 A D to move left and right
 SPACE to jump
 ESC for quit screen
 G on Main Menu for GodMode
 */

#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_image.h>
#include <vector>
#include <fstream>
#include <string>
#include <iostream>
#include <sstream>

#include "Matrix.h"
#include "SheetSprite.h"
#include "ShaderProgram.h"
#include <SDL_mixer.h>

#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"

using namespace std;

#define FIXED_TIMESTEP 0.0166666f
#define MAX_TIMESTEPS 6
#define LEVEL_HEIGHT 20;
#define LEVEL_WIDTH 50;

const float TILE_SIZE = 0.1f;

SDL_Event event;
bool done = false;

SDL_Window* displayWindow;
ShaderProgram* program;
const Uint8 *keys = SDL_GetKeyboardState(NULL);
float lastFrameTicks = 0.0f;

Matrix projectionMatrix;
Matrix modelMatrix;
Matrix viewMatrix;

Matrix playerMatrix;
Matrix enemyMatrix;
Matrix coinMatrix;
Matrix interactionMatrix;

// tile attributes
int mapHeight;
int mapWidth;

vector<float> vertexData;
vector<float> texCoordData;

unsigned char** levelData;

float penetration;

GLuint spriteSheet;

int gridX = 0;
int gridY = 0;

bool godMode = false;

float elapsed;

// Audio
Mix_Chunk *jump;
Mix_Chunk *coin;
Mix_Chunk *door; // get through doors on each level
Mix_Chunk *hit;
Mix_Chunk *gameover;
Mix_Chunk *win; // pass all 3 levels
Mix_Music *music;

// Screenshake
float screenShakeValue = 0.0f;
float screenShakeSpeed = 5.0f;
float screenShakeIntensity = .80f;

float animationTime;

enum GameState { MAIN_MENU, LEVEL_1, LEVEL_2, LEVEL_3, GAME_OVER, EXIT_SCREEN, EXIT, WIN };

int state = MAIN_MENU;
int currentLevelState; // keep track of which level you are on

std::vector<int> levels = { LEVEL_1, LEVEL_2, LEVEL_3 };
std::vector<string> files = { "world_1.txt", "world_2.txt", "world_3.txt" };

void DrawText(ShaderProgram *program, int fontTexture, std::string text, float size, float spacing) {
    float texture_size = 1.0 / 16.0f;
    std::vector<float> vertexData;
    std::vector<float> texCoordData;
    
    for (int i = 0; i < text.size(); i++) {
        float texture_x = (float)(((int)text[i]) % 16) / 16.0f;
        float texture_y = (float)(((int)text[i]) / 16) / 16.0f;
        
        vertexData.insert(vertexData.end(), {
            ((size + spacing) * i) + (-0.5f * size), 0.5f * size,
            ((size + spacing) * i) + (-0.5f * size), -0.5f * size,
            ((size + spacing) * i) + (0.5f * size), 0.5f * size,
            ((size + spacing) * i) + (0.5f * size), -0.5f * size,
            ((size + spacing) * i) + (0.5f * size), 0.5f * size,
            ((size + spacing) * i) + (-0.5f * size), -0.5f * size,
        });
        
        texCoordData.insert(texCoordData.end(), {
            texture_x, texture_y,
            texture_x, texture_y + texture_size,
            texture_x + texture_size, texture_y,
            texture_x + texture_size, texture_y + texture_size,
            texture_x + texture_size, texture_y,
            texture_x, texture_y + texture_size,
        });
    }
    glUseProgram(program->programID);
    
    glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertexData.data());
    glEnableVertexAttribArray(program->positionAttribute);
    
    glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoordData.data());
    glEnableVertexAttribArray(program->texCoordAttribute);
    
    glBindTexture(GL_TEXTURE_2D, fontTexture);
    glDrawArrays(GL_TRIANGLES, 0, text.size() * 6);
    
    glDisableVertexAttribArray(program->positionAttribute);
    glDisableVertexAttribArray(program->texCoordAttribute);
}

GLuint LoadTexture(const char *image_path) {
    SDL_Surface *surface = IMG_Load(image_path);
    
    // Generates a new OpenGL texture ID
    GLuint textureID;
    glGenTextures(1, &textureID);
    
    // Binds texture to a texture target
    glBindTexture(GL_TEXTURE_2D, textureID);
    
    // Set texture data of specified texture target
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface->w, surface->h, 0, GL_BGRA, GL_UNSIGNED_BYTE, surface->pixels);
    
    // Set texture parameter of specified texture target
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    SDL_FreeSurface(surface);
    
    return textureID;
}

void CleanUp() {
    Mix_FreeChunk(jump);
    Mix_FreeChunk(coin);
    Mix_FreeChunk(gameover);
    Mix_FreeChunk(win);
    Mix_FreeChunk(hit);
    Mix_FreeChunk(door);
    Mix_FreeMusic(music);
    SDL_Quit();
}

class Tile {
public:
    Tile(float x, float y, bool solid, float width, float height) : xPos(x), yPos(y), isSolid(solid), width(width), height(height) {}
    
    bool isSolid = true;
    float width;
    float height;
    float xPos;
    float yPos;
    unsigned int value;
};

bool readHeader(std::ifstream &stream) {
    string line;
    mapWidth = -1;
    mapHeight = -1;
    
    while(getline(stream, line)) {
        if (line == "") break;
        
        istringstream sStream(line);
        string key, value;
        getline(sStream, key, '=');
        getline(sStream, value);
        
        if (key == "width") {
            mapWidth = atoi(value.c_str());
        }
        else if (key == "height") {
            mapHeight = atoi(value.c_str());
        }
    }
    
    if (mapWidth == -1 || mapHeight == -1) {
        return false;
    }
    
    else {
        // allocate our map data
        levelData = new unsigned char*[mapHeight];
        
        for (int i = 0; i < mapHeight; ++i) {
            levelData[i] = new unsigned char[mapWidth];
        }
        return true;
    }
}

bool readLayerData(std::ifstream &stream) {
    string line;
    
    while(getline(stream, line)) {
        if (line == "") break;
        
        istringstream sStream(line);
        string key, value;
        getline(sStream, key, '=');
        getline(sStream, value);
        
        if (key == "data") {
            for (int y = 0; y < mapHeight; y++) {
                getline(stream, line);
                istringstream lineStream(line);
                string tile;
                for (int x = 0; x < mapWidth; x++) {
                    getline(lineStream, tile, ',');
                    unsigned int val = (unsigned int)atoi(tile.c_str());
                    if (val > 0) {
                        // tiles in this format are indexed from 1, not 0
                        levelData[y][x] = val-1;
                    }
                    else {
                        levelData[y][x] = 0;
                    }
                }
            }
        }
    }
    return true;
}

/*
 * render tile map level
 * render entire tilemap as one vertex array
 * - go through tiles line by line
 * - add vertices to a single array
 * - Y axis points up, tile indexes count down
 */
void RenderMap() {
    float margin = 2.0f/692.0f;
    
    for (int y = 0; y < 15; y++) {
        for (int x = 0; x < 50; x++) {
            float u = ((float)(((int)levelData[y][x]) % 30)/(float)30) + margin;
            float v = ((float)(((int)levelData[y][x]) / 30)/(float)30) + margin;
            
            float spriteWidth = 1.0f/(float)30 - 2*margin;
            float spriteHeight = 1.0f/(float)30 - 2*margin;
            
            vertexData.insert(vertexData.end(), {
                TILE_SIZE * x, -TILE_SIZE * y,
                TILE_SIZE * x, (-TILE_SIZE * y) - TILE_SIZE,
                (TILE_SIZE * x) + TILE_SIZE, (-TILE_SIZE * y) - TILE_SIZE,
                
                TILE_SIZE * x, -TILE_SIZE * y,
                (TILE_SIZE * x) + TILE_SIZE, (-TILE_SIZE * y) - TILE_SIZE,
                (TILE_SIZE * x) + TILE_SIZE, -TILE_SIZE * y
            });
            
            texCoordData.insert(texCoordData.end(), {
                u, v,
                u, v+(spriteHeight),
                u+spriteWidth, v+(spriteHeight),
                
                u, v,
                u+spriteWidth, v+spriteHeight,
                u+spriteWidth, v
            });
        }
    }
    
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glUseProgram(program->programID);
    
    glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertexData.data());
    glEnableVertexAttribArray(program->positionAttribute);
    
    glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoordData.data());
    glEnableVertexAttribArray(program->texCoordAttribute);
    
    glBindTexture(GL_TEXTURE_2D, spriteSheet);
    glDrawArrays(GL_TRIANGLES, 0, vertexData.size() / 2);
    
    glDisableVertexAttribArray(program->positionAttribute);
    glDisableVertexAttribArray(program->texCoordAttribute);
}

// if tile has certain value, it is a solid
bool isSolid(int val) {
    if (val == 123 || val == 152 || val == 95 || val == 158)
        return true;
    return false;
}

// if tile has certain value, player dies
bool isDeath(int val) {
    if (val == 70)
        return true;
    return false;
}

// convert entity positions to grid coordinates
// make sure tile coords !< 0 or > than map
void worldToTileCoordinates(float worldX, float worldY, int *gridX, int *gridY) {
    *gridX = (int)(worldX / 0.1f);
    *gridY = (int)(-worldY / 0.1f);
}

float lerp(float v0, float v1, float t) {
    return (1.0-t)*v0 + t*v1;
}

class Entity {
public:
    Entity() {}
    Entity(std::string type, float x, float y, Matrix matrix, GLuint sheet) : type(type), x(x), y(y), matrix(matrix), spriteSheet(sheet), isAlive(true) {}
    
    void movePlayer(float elapsed) {
        
        x_acceleration = 0.0f;
        
        if (keys[SDL_SCANCODE_D]){
            x_acceleration = 3.1f;
            lookLeft = false;
        }
        
        if (keys[SDL_SCANCODE_A]){
            x_acceleration = -3.1f;
            lookLeft = true;
        }
        
        if (keys[SDL_SCANCODE_SPACE]) {
            Mix_PlayChannel(-1, jump, 0);
            if (collidedBottom == true){
                y_velocity= 3.8;
            }
        }
        
        x_velocity = lerp(x_velocity, 0.0f, elapsed * x_friction);
        y_velocity = lerp(y_velocity, 0.0f, elapsed * y_friction);
        
        x_velocity += x_acceleration * elapsed;
        y_velocity -= y_acceleration * elapsed;
        
        float gravity = -9.81f;
        
        y_velocity += gravity * elapsed;
        
        x += x_velocity * elapsed;
        y += y_velocity * elapsed;
        
        if (lookLeft) {
            playerMatrix.identity();
            playerMatrix.Translate(x, y, 0.0);
            playerMatrix.Scale(-1.0, 1.0, 0.0);
            program->setModelMatrix(playerMatrix);
        }
    }
    
    // Straight-up movement in all directions
    void godMode(float elapsed) {
        x_acceleration = 0.0f;
        
        if (keys[SDL_SCANCODE_D]){
            x += 2.0f * elapsed;
            lookLeft = false;
        }
        
        if (keys[SDL_SCANCODE_A]){
            x -= 2.0f * elapsed;
            lookLeft = true;
        }
        
        if (keys[SDL_SCANCODE_W]) {
            y += 2.0f * elapsed;
        }
        
        if (keys[SDL_SCANCODE_S]) {
            y -= 2.0f * elapsed;
        }
    }
    
    // Moves enemy left and right
    void moveEnemy(float elapsed) {
        x_velocity = lerp(x_velocity, 0.0f, elapsed * x_friction);
        x_velocity += (x_acceleration+1.0) * elapsed;
        if (collidedLeft == true || collidedRight == true) {
            x_velocity *= -1;
        }
        x += x_velocity * elapsed;
    }
    
    void Draw(ShaderProgram *program, Matrix& matrix, int index) {
        float u = (float)((index) % 30) / (float)30;
        float v = (float)((index) / 30) / (float)30;
        
        float spriteWidth = 1.0f / (float)30;
        float spriteHeight = 1.0f / (float)30;
        
        GLfloat texCoords[] = {
            u, v + spriteHeight,
            u + spriteWidth, v,
            u, v,
            u + spriteWidth, v,
            u, v + spriteHeight,
            u + spriteWidth, v + spriteHeight
        };
        
        float vertices[] = {
            -0.5f*TILE_SIZE, -0.5f*TILE_SIZE,
            0.5f*TILE_SIZE, 0.5f*TILE_SIZE,
            -0.5f*TILE_SIZE, 0.5f*TILE_SIZE,
            0.5f*TILE_SIZE, 0.5f*TILE_SIZE,
            -0.5f*TILE_SIZE, -0.5f*TILE_SIZE,
            0.5f*TILE_SIZE, -0.5f*TILE_SIZE
        };
        
        matrix.identity();
        matrix.Translate(x, y, 0.0f);
        program->setModelMatrix(matrix);
        
        glUseProgram(program->programID);
        
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices);
        glEnableVertexAttribArray(program->positionAttribute);
        
        glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
        glEnableVertexAttribArray(program->texCoordAttribute);
        
        glBindTexture(GL_TEXTURE_2D, spriteSheet);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        
        glDisableVertexAttribArray(program->positionAttribute);
        glDisableVertexAttribArray(program->texCoordAttribute);
    }
    
    // checks collision with world
    void collidesWith() {
        
        // bottom collision
        worldToTileCoordinates(x, y - .1/2 , &gridX, &gridY);
        if (type == "PLAYER") {
            if (isSolid(levelData[gridY][gridX])){
                collidedBottom = true;
                y_velocity = 0;
                y_penetration = fabs((-TILE_SIZE * gridY) - (y - height/2));
                y += y_penetration;
            }
            else {
                collidedBottom = false;
                y_penetration = 0;
            }
            
            if (isDeath(levelData[gridY][gridX])) {
                Mix_PlayChannel(-1, hit, 0);
                state = GAME_OVER;
                Mix_PlayChannel(-1, gameover, 0);
            }
        }
        
        // top collision
        worldToTileCoordinates(x, y + 0.1 / 2, &gridX, &gridY);
        if (type == "PLAYER") {
            if (isSolid(levelData[gridY][gridX])){
                collidedTop = true;
                y_velocity = 0;
                y_penetration = fabs((y + height/2) - (-TILE_SIZE * gridY));
                y -= y_penetration +0.01;
            }
            else {
                collidedTop = false;
                y_penetration = 0;
            }
            
            if (isDeath(levelData[gridY][gridX])) {
                Mix_PlayChannel(-1, hit, 0);
                state = GAME_OVER;
                Mix_PlayChannel(-1, gameover, 0);
            }
            
        }
        
        // player right collision
        worldToTileCoordinates(x + .1/ 2, y, &gridX, &gridY);
        if (type == "PLAYER") {
            if (isSolid(levelData[gridY][gridX])){
                collidedRight = true;
                x_velocity = 0;
                x_penetration = fabs((x + width/2) - (TILE_SIZE * gridX));
                x -= x_penetration;
            }
            else {
                collidedRight = false;
                x_penetration = 0;
            }
            
            if (isDeath(levelData[gridY][gridX])) {
                Mix_PlayChannel(-1, hit, 0);
                state = GAME_OVER;
                Mix_PlayChannel(-1, gameover, 0);
            }
        }
        
        // enemy right collision
        if (type == "ENEMY") {
            if (currentLevelState == 1) {
                if (gridY == 10) {
                    if (gridX == 25 || gridX == 46) {
                        collidedRight = true;
                        x_penetration = fabs((x + width/2) - (TILE_SIZE * gridX));
                        x -= x_penetration;
                    }
                }
                else
                    collidedRight = false;
            }
            else if (currentLevelState == 2) {
                if (gridY == 10) {
                    if (gridX == 46) {
                        collidedRight = true;
                        x_penetration = fabs((x + width/2) - (TILE_SIZE * gridX));
                        x -= x_penetration;
                    }
                }
                else
                    collidedRight = false;
            }
            else if (currentLevelState == 3) {
                if (gridY == 10) {
                    if (gridX == 26) {
                        collidedRight = true;
                        x_penetration = fabs((x + width/2) - (TILE_SIZE * gridX));
                        x -= x_penetration;
                    }
                }
                else if (gridY == 4) {
                    if (gridX == 45) {
                        collidedRight = true;
                        x_penetration = fabs((x + width/2) - (TILE_SIZE * gridX));
                        x -= x_penetration;
                    }
                }
                else
                    collidedRight = false;
            }
        }
        
        // player left collision
        worldToTileCoordinates(x - 0.1 / 2, y, &gridX, &gridY);
        if (type == "PLAYER") {
            if (isSolid(levelData[gridY][gridX])){
                collidedLeft = true;
                x_velocity = 0;
                x_penetration = fabs((TILE_SIZE * gridX) + TILE_SIZE - (x - width/2));
                x += x_penetration;
            }
            else {
                collidedLeft = false;
                x_penetration = 0;
            }
            
            if (isDeath(levelData[gridY][gridX])) {
                Mix_PlayChannel(-1, hit, 0);
                state = GAME_OVER;
                Mix_PlayChannel(-1, gameover, 0);
            }
        }
        
        // enemy left collision
        if (type == "ENEMY") {
            if (currentLevelState == 1) {
                if (gridY == 10) {
                    if (gridX == 34 || gridX == 16) {
                        collidedLeft = true;
                        x_penetration = fabs((TILE_SIZE * gridX) + TILE_SIZE - (x - width/2));
                        x += x_penetration;
                    }
                }
                else
                    collidedLeft = false;
            }
            else if (currentLevelState == 2) {
                if (gridY == 10) {
                    if (gridX == 17) {
                        collidedLeft = true;
                        x_penetration = fabs((TILE_SIZE * gridX) + TILE_SIZE - (x - width/2));
                        x += x_penetration;
                    }
                }
                else
                    collidedLeft = false;
            }
            else if (currentLevelState == 3) {
                if (gridY == 10 && gridX == 8) {
                    collidedLeft = true;
                    x_penetration = fabs((TILE_SIZE * gridX) + TILE_SIZE - (x - width/2));
                    x += x_penetration;
                }
                else if (gridY == 4 && gridX == 36) {
                    collidedLeft = true;
                    x_penetration = fabs((TILE_SIZE * gridX) + TILE_SIZE - (x - width/2));
                    x += x_penetration;
                }
                else
                    collidedLeft = false;
            }
        }
    }
    
    void stuffCollision(float elapsed);
    
    float x;
    float y;
    
    float x_velocity = 0.0f;
    float y_velocity = 0.0f;
    
    float x_acceleration = 0.0;
    float y_acceleration = 10.0;
    
    float x_friction = 4.0;
    float y_friction = 2.0;

    float x_penetration = 0;
    float y_penetration = 0;
    
    int gridX = 0;
    int gridY = 0;
    
    float width = 0.1f;
    float height = 0.1f;
    
    bool collided;
    bool exited;
    bool lookLeft = false;
    bool isAlive;
    
    string type;
    Matrix matrix;
    GLuint spriteSheet;
    int index;
    
    bool collidedTop = false;
    bool collidedBottom = false;
    bool collidedLeft = false;
    bool collidedRight = false;
};

// check for collision between entities
bool entityCollision(Entity e1, Entity e2) {
    return (!(e1.x - TILE_SIZE / 2 > e2.x  || e1.x + TILE_SIZE / 2 < e2.x  ||
              e1.y - TILE_SIZE / 2 > e2.y || e1.y + TILE_SIZE / 2 < e2.y ));
}

Entity player;
Entity enemy;

std::vector<Entity> enemies;
std::vector<Entity> coins;
std::vector<Entity> interactions;

void nextStage();

// clears vectors and arrays
void clearData() {
    enemies.clear();
    coins.clear();
    interactions.clear();
    vertexData.clear();
    texCoordData.clear();
    
    for (int i = 0; i < mapHeight; i++) {
        delete[] levelData[i];
    }
    delete[] levelData;
}

// collision with other objects and things
void Entity::stuffCollision(float elapsed) {
    // collision with coins
    for (int i = 0; i < coins.size(); i++) {
        if (coins[i].isAlive == true && entityCollision(player, coins[i])) {
            Mix_PlayChannel(-1, coin, 0);
            coins[i].isAlive = false;
        }
    }
    
    // collision with spikes and exit
    for (int i = 0; i < interactions.size(); i++) {
        if (entityCollision(player, interactions[i])) {
            if (interactions[i].type == "SPIKE") {
                Mix_PlayChannel(-1, hit, 0);
                state = GAME_OVER;
                Mix_PlayChannel(-1, gameover, 0);
            }
            else if (interactions[i].type == "DOOR") {
                Mix_PlayChannel(-1, door, 0);
                exited = true;
                
                nextStage();
            }
        }
    }
    
    // collision with enemies
    for (int i = 0; i < enemies.size(); i++) {
        if (entityCollision(player, enemies[i])) {
            // play death sound
            Mix_PlayChannel(-1, hit, 0);
            state = GAME_OVER;
            Mix_PlayChannel(-1, gameover, 0);
        }
    }
}

// method to place entity at a certain position
void placeEntity(string type, float xPos, float yPos){
    if (type == "PLAYER") {
        player = Entity(type, xPos, yPos, playerMatrix, spriteSheet);
    }
    
    else if (type == "ENEMY") {
        enemy = Entity(type, xPos, yPos, enemyMatrix, spriteSheet);
        enemy.x_friction = 1.4f;
        enemies.push_back(enemy);
    }
    
    else if (type == "COLLECTIBLE") {
        Entity entity(type, xPos, yPos, coinMatrix, spriteSheet);
        coins.push_back(entity);
    }
    
    else if (type == "SPIKE" || type == "DOOR") {
        Entity interaction(type, xPos, yPos, interactionMatrix, spriteSheet);
        interactions.push_back(interaction);
    }
}

bool readEntityData(std::ifstream &stream) {
    string line;
    string type;
    
    while (getline(stream, line)) {
        if (line == "") break;
        
        istringstream sStream(line);
        string key, value;
        getline(sStream, key, '=');
        getline(sStream, value);
        
        if (key == "type") {
            type = value;
        }
        else if (key == "location") {
            istringstream lineStream(value);
            string xPosition, yPosition;
            getline(lineStream, xPosition, ',');
            getline(lineStream, yPosition, ',');
            
            float placeX = atoi(xPosition.c_str())*0.1f+0.05;
            float placeY = atoi(yPosition.c_str())*-0.1f+0.05;
            
            placeEntity(type, placeX, placeY);
        }
    }
    return true;
}

void readFile(std::string text) {
    ifstream file(text);
    string line;
    while (getline(file, line)) {
        if (line == "[header]") {
            if (!readHeader(file)) {
                return;
            }
        }
        else if (line == "[layer]") {
            readLayerData(file);
        }
        else if (line == "[Objects]") {
            readEntityData(file);
        }
    }
}

void Setup() {
    SDL_Init(SDL_INIT_VIDEO);
    displayWindow = SDL_CreateWindow("Final Project", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 400, SDL_WINDOW_OPENGL);
    SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
    SDL_GL_MakeCurrent(displayWindow, context);
}

void resetViewMatrix() {
    viewMatrix.identity();
    program->setViewMatrix(viewMatrix);
}

/* Methods to draw game states */
void MainMenu(GLuint font_texture) {
    resetViewMatrix();
    
    modelMatrix.identity();
    modelMatrix.Translate(-0.5f, 1.0, 0.0f);
    program->setModelMatrix(modelMatrix);
    DrawText(program, font_texture, "PLATFORM3R", .4f, -0.1);
    
    modelMatrix.identity();
    modelMatrix.Translate(-1.0f, 0.0f, 0.0f);
    program->setModelMatrix(modelMatrix);
    DrawText(program, font_texture, "SPACE TO BEGIN", .3f, -0.05f);
    
    modelMatrix.identity();
    modelMatrix.Translate(-1.0f, -0.5f, 0.0f);
    program->setModelMatrix(modelMatrix);
    DrawText(program, font_texture, "ESC TO QUIT", .3f, -0.05f);
    
    modelMatrix.identity();
    modelMatrix.Translate(-1.4f, -1.0f, 0.0f);
    program->setModelMatrix(modelMatrix);
    DrawText(program, font_texture, "G FOR GODMODE (no tile collisions)", .2f, -0.05f);
    
    modelMatrix.identity();
    
    if (keys[SDL_SCANCODE_SPACE]) {
        clearData();
        state = LEVEL_1;
        readFile("world_1.txt");
    }
    else if (keys[SDL_SCANCODE_ESCAPE])
        state = EXIT_SCREEN;
    
    else if (keys[SDL_SCANCODE_G]) {
        godMode = true;
        clearData();
        state = LEVEL_1;
        readFile("world_1.txt");
    }
}

void GameOver(GLuint font_texture) {
    
    // screen shake changes
    screenShakeValue += elapsed;
    screenShakeIntensity -= elapsed;
    if(screenShakeIntensity < 0)
        screenShakeIntensity = 0;
    
    modelMatrix.identity();
    modelMatrix.Translate(-1.2f, .5f, 0.0f);
    program->setModelMatrix(modelMatrix);
    DrawText(program, font_texture, "GAME OVER", .4f, 0.1f);
    
    modelMatrix.identity();
    modelMatrix.Translate(-0.7f, 0.0f, 0.0f);
    program->setModelMatrix(modelMatrix);
    DrawText(program, font_texture, "SPACE TO RESTART", .3f, -.06f);
    
    modelMatrix.identity();
    modelMatrix.Translate(-0.7f, -0.5f, 0.0f);
    program->setModelMatrix(modelMatrix);
    DrawText(program, font_texture, "ESC TO QUIT", .3f, -.06f);
    
    modelMatrix.identity();
    
    // screen shake
    viewMatrix.identity();
    viewMatrix.Translate(0.0f, sin(screenShakeValue * 100.0) * screenShakeIntensity, 0.0f);
    
    if (keys[SDL_SCANCODE_G])
        godMode = true;
    
    if (keys[SDL_SCANCODE_ESCAPE]) {
        state = EXIT;
    }
    
    // restart from the beginning
    else if (keys[SDL_SCANCODE_SPACE]) {
        screenShakeIntensity = .80f;
        screenShakeValue = 0.0f;
        clearData();
        state = LEVEL_1;
        readFile(files[state-1]);
    }
}

void ExitPrompt(GLuint font_texture) {
    resetViewMatrix();
    
    modelMatrix.identity();
    modelMatrix.Translate(-1.2f, 0.6f, 0.0f);
    program->setModelMatrix(modelMatrix);
    DrawText(program, font_texture, "SURE YOU WANT TO QUIT?", .3f, -0.08);
    
    modelMatrix.identity();
    modelMatrix.Translate(-1.2f, 0.0f, 0.0f);
    program->setModelMatrix(modelMatrix);
    DrawText(program, font_texture, "Y TO QUIT", .3f, -0.08f);
    
    modelMatrix.identity();
    modelMatrix.Translate(-1.2f, -0.5f, 0.0f);
    program->setModelMatrix(modelMatrix);
    DrawText(program, font_texture, "N TO CONTINUE", .3f, -0.08f);
    
    modelMatrix.identity();
    
    if (keys[SDL_SCANCODE_N]) {
        // resume game
        state = currentLevelState;
    }
    else if (keys[SDL_SCANCODE_Y]) {
        godMode = false;
        state = EXIT;
    }
}

void Win(GLuint font_texture) {
    godMode = false;
    resetViewMatrix();
    
    modelMatrix.identity();
    modelMatrix.Translate(-1.2f, 0.5f, 0.0f);
    program->setModelMatrix(modelMatrix);
    DrawText(program, font_texture, "YOU WIN!", .4f, 0.0);
    
    modelMatrix.identity();
    modelMatrix.Translate(-0.7f, 0.0f, 0.0f);
    program->setModelMatrix(modelMatrix);
    DrawText(program, font_texture, "SPACE TO RESTART", .3f, -.06f);
    
    modelMatrix.identity();
    modelMatrix.Translate(-0.7f, -0.5f, 0.0f);
    program->setModelMatrix(modelMatrix);
    DrawText(program, font_texture, "ESC TO QUIT", .3f, -.06f);
    
    modelMatrix.identity();
    modelMatrix.Translate(-1.0f, -1.0f, 0.0f);
    program->setModelMatrix(modelMatrix);
    DrawText(program, font_texture, "G TO RESTART IN GODMODE", .2f, -.05f);
    
    modelMatrix.identity();
    
    Mix_PlayChannel(-1, win, 0);
    Mix_VolumeChunk(win, 10);
    
    if (keys[SDL_SCANCODE_SPACE]) {
        state = LEVEL_1;
        readFile(files[state-1]);
    }
    else if (keys[SDL_SCANCODE_ESCAPE])
        state = EXIT;
    
    else if (keys[SDL_SCANCODE_G]) {
        godMode = true;
        state = LEVEL_1;
        readFile("world_1.txt");
    }
}

// set to follow player
void centerPlayer() {
    viewMatrix.identity();
    viewMatrix.Scale(5.0f, 5.0f, 1.0f);
    viewMatrix.Translate(-player.x, -player.y, 0.0f);
    program->setViewMatrix(viewMatrix);
}

// draws game level
void RenderGameLevel() {
    player.Draw(program, playerMatrix, 80);
    
    for (int i = 0; i < enemies.size(); i++)
        enemies[i].Draw(program, enemyMatrix, 445);
    
    for (int i = 0; i < coins.size(); i++) {
        if (coins[i].isAlive == true)
            coins[i].Draw(program, coinMatrix, 78);
    }
    
    for (int i = 0; i < interactions.size(); i++) {
        if (interactions[i].type == "SPIKE") {
            interactions[i].Draw(program, interactionMatrix, 70);
        }
        else if (interactions[i].type == "DOOR") {
            interactions[i].Draw(program, interactionMatrix, 168);
        }
    }
    
    if (keys[SDL_SCANCODE_ESCAPE])
        state = EXIT_SCREEN;
}

/***************************************
 Method: void nextStage()
 
 Changes level states. Clears entity and 
 level data after each level so there 
 are no overlaps
****************************************/

void nextStage() {
    if (state == LEVEL_1 && player.exited) {
        clearData();
        readFile(files[state]);
        state = LEVEL_2;
    }
    else if (state == LEVEL_2 && player.exited) {
        clearData();
        readFile(files[state]);
        state = LEVEL_3;
    }
    else if (state == LEVEL_3 && player.exited) {
        clearData();
        state = WIN;
    }
}

// update entity movement and collision
void Update(float elapsed) {
    player.movePlayer(elapsed);
    
    player.collidesWith();
    player.stuffCollision(elapsed);
    
    for (int i = 0; i < enemies.size(); i++) {
        enemies[i].moveEnemy(elapsed);
        enemies[i].collidesWith();
    }
}

// update entity movement in God Mode, no collision checking
void GMUpdate(float elapsed) {
    player.godMode(elapsed);
    player.stuffCollision(elapsed);
    
    for (int i = 0; i < enemies.size(); i++) {
        enemies[i].moveEnemy(elapsed);
        enemies[i].collidesWith();
    }
}

int main(int argc, char *argv[])
{
    Setup();
    program = new ShaderProgram(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");

    glViewport(0, 0, 640, 400);
    
    /***************** TEXTURES ****************************/
    spriteSheet = LoadTexture("sheet.png");
    GLuint font = LoadTexture("font1.png");
    
    /****************** AUDIO ********************************/
    
    Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096);
    
    jump = Mix_LoadWAV("jump.wav");
    coin = Mix_LoadWAV("coin.wav");
    door = Mix_LoadWAV("exit.wav");
    hit = Mix_LoadWAV("hit.wav");
    gameover = Mix_LoadWAV("gameover.wav");
    win = Mix_LoadWAV("win.wav");
    music = Mix_LoadMUS("music.mp3");
    
    Mix_PlayMusic(music, -1);
    
    /***********************************************************/
    
    projectionMatrix.setOrthoProjection(-2.0, 3.8, -2.0, 2.0f, -1.0f, 1.0f);

    float lastFrameTicks = 0.0f;
    
    while (!done) {
        
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
                done = true;
            }
        }
        
        float ticks = (float)SDL_GetTicks()/1000.0f;
        elapsed = ticks - lastFrameTicks;
        lastFrameTicks = ticks;
        
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glEnable(GL_BLEND);
        
        program->setProjectionMatrix(projectionMatrix);
        program->setViewMatrix(viewMatrix);
        program->setModelMatrix(modelMatrix);
        
        /******************* STATES **************************/
        if (state == MAIN_MENU) {
            MainMenu(font);
        }
        
        else if (state == LEVEL_1) {
            currentLevelState = LEVEL_1;
            player.exited = false;
            
            centerPlayer();
            RenderMap();
            RenderGameLevel();
            
            float fixedElapsed = elapsed;
            if (fixedElapsed > FIXED_TIMESTEP * MAX_TIMESTEPS) {
                fixedElapsed = FIXED_TIMESTEP * MAX_TIMESTEPS;
            }
            while (fixedElapsed >= FIXED_TIMESTEP) {
                fixedElapsed -= FIXED_TIMESTEP;
                if (godMode == true)
                    GMUpdate(FIXED_TIMESTEP);
                else
                    Update(FIXED_TIMESTEP);
            }
            if (godMode == true)
                GMUpdate(fixedElapsed);
            else
                Update(fixedElapsed);
        }
        
        else if (state == LEVEL_2) {
            currentLevelState = LEVEL_2;
            player.exited = false;
            
            centerPlayer();
            RenderMap();
            RenderGameLevel();
            
            float fixedElapsed = elapsed;
            if (fixedElapsed > FIXED_TIMESTEP * MAX_TIMESTEPS) {
                fixedElapsed = FIXED_TIMESTEP * MAX_TIMESTEPS;
            }
            
            while (fixedElapsed >= FIXED_TIMESTEP) {
                fixedElapsed -= FIXED_TIMESTEP;
                if (godMode == true)
                    GMUpdate(FIXED_TIMESTEP);
                else
                    Update(FIXED_TIMESTEP);
            }
            if (godMode == true)
                GMUpdate(fixedElapsed);
            else
                Update(fixedElapsed);
        }
        
        else if (state == LEVEL_3) {
            currentLevelState = LEVEL_3;
            player.exited = false;
            
            centerPlayer();
            RenderMap();
            RenderGameLevel();
            
            float fixedElapsed = elapsed;
            if (fixedElapsed > FIXED_TIMESTEP * MAX_TIMESTEPS) {
                fixedElapsed = FIXED_TIMESTEP * MAX_TIMESTEPS;
            }
            
            while (fixedElapsed >= FIXED_TIMESTEP) {
                fixedElapsed -= FIXED_TIMESTEP;
                if (godMode == true)
                    GMUpdate(FIXED_TIMESTEP);
                else
                    Update(FIXED_TIMESTEP);
            }
            if (godMode == true)
                GMUpdate(fixedElapsed);
            else
                Update(fixedElapsed);
        }
        
        else if (state == GAME_OVER) {
            GameOver(font);
        }
        
        else if (state == EXIT_SCREEN) {
            ExitPrompt(font);
        }
        
        else if (state == EXIT) {
            return 0;
        }
        
        else if (state == WIN) {
            Win(font);
        }
        
        /***********************************************************/
        
        SDL_GL_SwapWindow(displayWindow);
    }
    
    //clearData();
    CleanUp();
    return 0;
}

