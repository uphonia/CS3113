/*
 
 Angela Wu N14475962
 CS3113
 Homework 4 - Platformer
 
 AD to move
 D - moves in x direction and moves in -y direction
 SPACE to jump
 
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

#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"

using namespace std;

#define FIXED_TIMESTEP 0.0166666f
#define MAX_TIMESTEPS 6
#define LEVEL_HEIGHT 20;
#define LEVEL_WIDTH 50;
#define TILE_SIZE 0.1f;

SDL_Window* displayWindow;
ShaderProgram* program;
const Uint8 *keys = SDL_GetKeyboardState(NULL);
float lastFrameTicks = 0.0f;
int state;

Matrix projectionMatrix;
Matrix modelMatrix;
Matrix viewMatrix;

Matrix playerMatrix;
Matrix enemyMatrix;
Matrix doorMatrix;

// tile attributes
int mapHeight;
int mapWidth;

vector<float> vertexData;
vector<float> texCoordData;

unsigned char** levelData;

float penetration;
float max_X1 = 1.26;
float max_X2 = 1.60f;

GLuint spriteSheet;

int gridX = 0;
int gridY = 0;

enum GameState { MAIN_MENU, GAME_LEVEL, GAME_OVER };

// if tile has certain value, it is a solid
bool isSolid(int val) {
    if (val == 256 || val == 123 || val == 152 || val == 95)
        return true;
    return false;
}

// convert entity positions to grid coordinates
// make sure tile coords !< 0 or > than map
void worldToTileCoordinates(float worldX, float worldY, int *gridX, int *gridY) {
    *gridX = (int)(worldX / 0.1f); // 1.5
    *gridY = (int)(-worldY / 0.1f); // 8.73
}

float lerp(float v0, float v1, float t) {
    return (-1.0-t)*v0 + t*v1;
}

class Entity {
public:
    Entity() {}
    Entity(std::string type, bool isStatic, float x, float y, Matrix matrix, GLuint sheet) : type(type),
        isStatic(isStatic), x(x), y(y), matrix(matrix), spriteSheet(sheet) {}
    
    // move enemy
    void Update(float elapsed) {
        x_velocity = 0.2f;
        /*
        x_velocity = lerp(x_velocity, 0.0f, elapsed * x_friction);
        y_velocity = lerp(y_velocity, 0.0f, elapsed * y_friction);
        
        x_velocity += x_acceleration * elapsed;
        y_velocity += y_acceleration * elapsed;
        */
        
        if (x >= max_X1 && x <= max_X2) {
            x += x_velocity * elapsed;
        }
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
            -0.5f*0.1f, -0.5f*0.1f,
            0.5f*0.1f, 0.5f*0.1f,
            -0.5f*0.1f, 0.5f*0.1f,
            0.5f*0.1f, 0.5f*0.1f,
            -0.5f*0.1f, -0.5f*0.1f,
            0.5f*0.1f, -0.5f*0.1f
        };
        
        program->setModelMatrix(matrix);
        matrix.identity();
        matrix.Translate(x, y, 0.0f);
        
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
    
    void collidesWith(Entity* enemy, Entity* exit, float elapsed) {
        // bottom collision
        worldToTileCoordinates(x, y - .1/2 , &gridX, &gridY);
        if (isSolid(levelData[gridY][gridX])){
            collidedBottom = true;
            y_velocity = 0;
            y_acceleration = 0;
        }
        else {
            collidedBottom = false;
            y_acceleration = 9.81;
        }
        
        // top collision
        worldToTileCoordinates(x, y + 0.1 / 2, &gridX, &gridY);
        if (isSolid(levelData[gridY][gridX])){
            collidedTop = true;
            y_velocity = 0;
            y_acceleration = 9.81;
        }
        else {
            collidedTop = false;
        }
        
        // right collision
        worldToTileCoordinates(x + .1/ 2, y, &gridX, &gridY);
        if (isSolid(levelData[gridY][gridX])){
            collidedRight = true;
            x_velocity = 0;
            x_acceleration = 0;
        }
        else {
            collidedRight = false;
            x_velocity = 0.5;
        }
        
        if (gridY == 15 && gridX == 42) {
            cout << "You have exited!" << endl;
        }
        
        // left collision
        worldToTileCoordinates(x - 0.1 / 2, y, &gridX, &gridY);
        if (isSolid(levelData[gridY][gridX])){
            collidedLeft = true;
            x_velocity = 0;
            x_acceleration = 0;
        }
        else {
            collidedLeft = false;
            x_velocity = 0.5;
        }
    }
    
    float x;
    float y;
    
    float x_velocity = 0.5f;
    float y_velocity = 0.5f;
    
    float x_acceleration = 0.1;
    float y_acceleration = 9.81;
    
    float x_friction;
    float y_friction = 0.5;
    
    float x_gravity;
    float y_gravity = -9.81;
    
    int gridX = 0;
    int gridY = 0;
    
    float width = 0.1f;
    float height = 0.1f;
    
    bool isStatic;
    bool solid = true;
    
    bool collided;
    bool exited;
    
    string type;
    Matrix matrix;
    GLuint spriteSheet;
    int index;
    
    bool collidedTop = false;
    bool collidedBottom = false;
    bool collidedLeft = false;
    bool collidedRight = false;
};

Entity player;
Entity enemy;
Entity door;

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

// vector to keep track of solid tiles
vector<Tile*> solidTiles;

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

void Setup() {
    SDL_Init(SDL_INIT_VIDEO);
    displayWindow = SDL_CreateWindow("Homework 4 - Platformer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, SDL_WINDOW_OPENGL);
    SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
    SDL_GL_MakeCurrent(displayWindow, context);
}

bool ProcessEvents(SDL_Event& event) {
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
            return true;
        }
    }
    return false;
}

void UpdateMainMenu() {
    if (keys[SDL_SCANCODE_TAB])
        state = GAME_LEVEL;
}

// draw main menu
void RenderMainMenu(GLuint font_texture) {
    modelMatrix.identity();
    modelMatrix.Translate(0.0f, -5.0f, 0.0f);
    program->setModelMatrix(modelMatrix);
    DrawText(program, font_texture, "PRESS SPACE TO BEGIN", .4f, -.2f);
}

void centerPlayer() {
    viewMatrix.identity();
    viewMatrix.Scale(5.0f, 5.0f, 1.0f);
    viewMatrix.Translate(-player.x, -player.y, 0.0f);
    program->setViewMatrix(viewMatrix);
}

void RenderGameLevel() {
    player.Draw(program, playerMatrix, 80);
    centerPlayer();
    
    enemy.Draw(program, enemyMatrix, 446);
    enemyMatrix.Translate(0.5f, -0.0f, 0.0f);
}

void RenderGameOver(GLuint font_texture) {
    modelMatrix.identity();
    modelMatrix.Translate(-2.0f, 1.5f, 0.0f);
    program->setModelMatrix(modelMatrix);
    DrawText(program, font_texture, "GAME OVER", .4f, -.2f);
    
    modelMatrix.Translate(-0.65f, -1.0f, 0.0f);
    program->setModelMatrix(modelMatrix);
    DrawText(program, font_texture, "PRESS TAB TO RESTART", .4f, -.2f);
}

void Update(float elapsed) {
    // Moving left
    if (keys[SDL_SCANCODE_A]) {
        // next two lines causes the player to have inf velocity
        //player.x_velocity = lerp(player.x_velocity, 0.0f, elapsed * player.x_friction);
        //player.x_velocity -= player.x_acceleration * elapsed;
        
        if (!player.collidedLeft)
            player.x -= player.x_velocity * elapsed;
    }
    
    // Moving right
    if (keys[SDL_SCANCODE_D]) {
        // next two lines causes the player to have inf velocity
        //player.x_velocity = lerp(player.x_velocity, 0.0f, elapsed * player.x_friction);
        //player.x_velocity += player.x_acceleration * elapsed;

        if (!player.collidedRight) {
            player.x += player.x_velocity * elapsed;
            player.y -= player.y_velocity * elapsed;
        }
    }
    
    // Jump
    if (keys[SDL_SCANCODE_SPACE]) {
        player.y_velocity = 0.5f;
        player.y += player.y_velocity * elapsed;
    }
}

void UpdateGame(float elapsed) {
    
    player.collidesWith(&enemy, &door, elapsed);
    Update(elapsed);
    
    enemy.Update(elapsed);
}

void CleanUp() {
    SDL_Quit();
}

// read header data
// creates the 2D array
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
    
    else { // allocate our map data
        levelData = new unsigned char*[mapHeight];
        
        for (int i = 0; i < mapHeight; ++i) {
            levelData[i] = new unsigned char[mapWidth];
        }
        return true;
    }
}

// read tile data
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
                        if (val == 124) {
                            Tile* tile = new Tile(x*0.1f, y*0.1f, true, 0.1f/2, 0.1f/2);
                            tile->value = val;
                            solidTiles.push_back(tile);
                        }
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

// method to place entity at a certain position
void placeEntity(string type, float xPos, float yPos){
    if (type == "PLAYER") {
        player = Entity(type, false, xPos, yPos, playerMatrix, spriteSheet);
    }
    
    else if (type == "ENEMY") {
        enemy = Entity(type, false, xPos, yPos, enemyMatrix, spriteSheet);
    }
    
    else if (type == "EXIT") {
        door = Entity(type, false, xPos, yPos, doorMatrix, spriteSheet);
    }
}

// read entity data
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

/*
 * render tile map level
 * render entire tilemap as one vertex array
 * - go through tiles line by line
 * - add vertices to a single array
 * - Y axis points up, tile indexes count down
 */
void RenderMap() {
    float margin = 2.0f/692.0f;
    
    for (int y = 0; y < 20; y++) {
        for (int x = 0; x < 50; x++) {
            float u = ((float)(((int)levelData[y][x]) % 30)/(float)30) + margin;
            float v = ((float)(((int)levelData[y][x]) / 30)/(float)30) + margin;
            
            float spriteWidth = 1.0f/(float)30 - 2*margin;
            float spriteHeight = 1.0f/(float)30 - 2*margin;
            
            vertexData.insert(vertexData.end(), {
                0.1f * x, -0.1f * y,
                0.1f * x, (-0.1f * y) - 0.1f,
                (0.1f * x) + 0.1f, (-0.1f * y) - 0.1f,
                
                0.1f * x, -0.1f * y,
                (0.1f * x) + 0.1f, (-0.1f * y) - 0.1f,
                (0.1f * x) + 0.1f, -0.1f * y
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

void drawMap(){
    program->setModelMatrix(modelMatrix);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    glUseProgram(program->programID);
    glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertexData.data());
    glEnableVertexAttribArray(program->positionAttribute);
    glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoordData.data());
    glEnableVertexAttribArray(program->texCoordAttribute);
    
    modelMatrix.identity();
    program->setModelMatrix(modelMatrix);
    
    glBindTexture(GL_TEXTURE_2D, spriteSheet);
    glDrawArrays(GL_TRIANGLES, 0, vertexData.size() / 2);
    glDisableVertexAttribArray(program->positionAttribute);
    glDisableVertexAttribArray(program->texCoordAttribute);
}

void drawEntity(ShaderProgram *program, int mapTexture, int index){
    float u = (float)(((int)index) % 30) / (float) 30;
    float v = (float)(((int)index) / 30) / (float) 30;
    
    float spriteWidth = 1.0f/(float)30;
    float spriteHeight = 1.0f/(float)30;
    
    GLfloat texCoords[] = {
        u, v+spriteHeight,
        u+spriteWidth, v,
        u, v,
        u+spriteWidth, v,
        u, v+spriteHeight,
        u+spriteWidth, v+spriteHeight
    };
    
    float vertices[] = {
        -0.1f/2, -0.1f/2,
        0.1f/2, 0.1f/2,
        -0.1f/2, 0.1f/2,
        0.1f/2, 0.1f/2,
        -0.1f/2, -0.1f/2,
        0.1f/2, -0.1f/2
    };
    
    glUseProgram(program->programID);
    
    glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(program->positionAttribute);
    
    glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
    glEnableVertexAttribArray(program->texCoordAttribute);
    
    glBindTexture(GL_TEXTURE_2D, mapTexture);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    
    glDisableVertexAttribArray(program->positionAttribute);
    glDisableVertexAttribArray(program->texCoordAttribute);
}

int main(int argc, char *argv[])
{
    Setup();
    
    program = new ShaderProgram(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");
    SDL_Event event;
    glViewport(0, 0, 640, 400);
    
    spriteSheet = LoadTexture("sheet.png");
    GLuint font = LoadTexture("font1.png");
    
    /*
     * read file line by line
     */
    ifstream infile("platformer.txt");
    string line;
    while (getline(infile, line)) {
        // handle line
        if (line == "[header]") {
            if (!readHeader(infile)) {
                return 0;
            }
        }
        else if (line == "[layer]") {
            readLayerData(infile);
        }
        else if (line == "[Objects]") {
            readEntityData(infile);
        }
    }
    
    //projectionMatrix.setOrthoProjection(-3.55, 3.55, -2.0f, 2.8f, -1.0f, 1.0f);
    projectionMatrix.setOrthoProjection(-0.7, 4.0, -2.0, 2.0f, -1.0f, 1.0f); // changed ortho to be able to see whole map
    
    enemy.x_velocity = 0.4;
    float lastFrameTicks = 0.0f;
    
    while (!ProcessEvents(event)) {
        float ticks = (float)SDL_GetTicks()/1000.0f;
        float elapsed = ticks - lastFrameTicks;
        lastFrameTicks = ticks;
        
        float fixedElapsed = elapsed;
        if (fixedElapsed > FIXED_TIMESTEP * MAX_TIMESTEPS) {
            fixedElapsed = FIXED_TIMESTEP * MAX_TIMESTEPS;
        }
        
        while (fixedElapsed >= FIXED_TIMESTEP) {
            fixedElapsed -= FIXED_TIMESTEP;
            Update(FIXED_TIMESTEP);
        }
        
        Update(fixedElapsed);
        
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        
        program->setProjectionMatrix(projectionMatrix);
        program->setViewMatrix(viewMatrix);
        program->setModelMatrix(modelMatrix);
        
        UpdateGame(elapsed);
        RenderMap();
        RenderGameLevel();
        
        SDL_GL_SwapWindow(displayWindow);
    }
    
    CleanUp();
    return 0;
}
