/*
 
 Angela Wu N14475962
 CS3113
 Homework 3 - Space Invaders
 
 Controls:
 
 Left and Right Arrow Key to move spaceship
 
 */

#ifdef _WINDOWS
#include <GL/glew.h>d
#endif
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_image.h>
#include "Matrix.h"
#include "ShaderProgram.h"
#include <vector>
#include "SheetSprite.h"

#ifdef _WINDOWS
#define RESOURCE_FOLDER ""
#else
#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

#define MAX_LASERS 30

SDL_Window* displayWindow;
ShaderProgram* program;
const Uint8 *keys = SDL_GetKeyboardState(NULL);
float lastFrameTicks = 0.0f;
int state;
int p_laserIndex = 0;
int e_laserIndex = 0;
bool shoot = false;

// keep index value of boundaries of the enemy area
int topLeft = 0;
int bottomLeft = 21;
int topRight = 6;
int bottomRight = 27;

Matrix projectionMatrix;
Matrix modelMatrix;
Matrix viewMatrix;

enum GameState { STATE_MAIN_MENU, STATE_GAME_LEVEL, STATE_GAME_OVER };

class Entity {
public:
    Entity(ShaderProgram* program, Matrix matrix, SheetSprite sprite):
    program(program), matrix(matrix), sprite(sprite), width(sprite.width), height(sprite.height), x(0.0f), y(0.0f), u(sprite.u), v(sprite.v), size(sprite.size) {}
    
    float x;
    float y;
    
    float u;
    float v;
    
    float size;
    float width;
    float height;
    
    ShaderProgram* program;
    Matrix matrix;
    SheetSprite sprite;
    
    float speed = 2.0f;
    float alive = true;
    
    void Draw() {
        program->setModelMatrix(matrix);
        glBindTexture(GL_TEXTURE_2D, sprite.textureID);
        
        GLfloat texCoords[] {
            u, v+height,
            u+width, v,
            u, v,
            u+width, v,
            u, v+height,
            u+width, v+height
        };
        
        float aspect = width / height;
        
        float vertices[] {
            -0.5f * size * aspect, -0.5f * size,
            0.5f * size * aspect, 0.5f * size,
            -0.5f * size * aspect, 0.5f * size,
            0.5f * size * aspect, 0.5f * size,
            -0.5f * size * aspect, -0.5f * size,
            0.5f * size * aspect, -0.5f * size
        };
        
        glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices);
        glEnableVertexAttribArray(program->positionAttribute);
    
        glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
        glEnableVertexAttribArray(program->texCoordAttribute);
        
        glDrawArrays(GL_TRIANGLES, 0, 6);
    
        glDisableVertexAttribArray(program->positionAttribute);
        glDisableVertexAttribArray(program->texCoordAttribute);
    }
    
    void Update(float elapsed) {
        
    }
};

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
    displayWindow = SDL_CreateWindow("Homework 3 - Space Invaders", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 500, 400, SDL_WINDOW_OPENGL);
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

// check collision between laser and player
bool checkPlayerColli(Entity* enemy_laser, Entity* player) {
    if (enemy_laser->y-enemy_laser->height/2 < player->y+player->height/2) {
        state = STATE_GAME_OVER;
        return true;
    }
    return false;
}

// check collision between laser and enemy
// doesn't work
bool checkEnemyColli(Entity* player_laser, Entity* enemy) {
    if (player_laser->y+player_laser->height/2 > enemy->y+enemy->height/2) {
        // remove laser
        player_laser->x = -2000.0f;
        // remove enemy
        enemy->alive = false;
        enemy->x = -2000.0f;
        return true;
    }
    else {
        player_laser->y = 2000.0f;
    }
    return false;
}

/* only one laser is shot
* bullet moves continuously if SPACE is held down
* bullet cannot keep up with x pos of ship
*/
void playerShoot(Entity* p_laser, Entity* ship, float elapsed) {
    if (p_laser->y < 3.0f) {
        p_laser->x = ship->x;
        p_laser->y += elapsed*p_laser->speed;
    }
    else {
        p_laser->x = ship->x;
        p_laser->y = -1.5f+ship->height/2;
    }
}

// could not implement
void enemyShoot(std::vector<Entity*> lasers, float elapsed) {
    if (e_laserIndex > MAX_LASERS-1) {
        e_laserIndex = 0;
    }
    
    if (lasers[e_laserIndex]->y > -2.0f) {
        lasers[e_laserIndex]->y -= 5.0f*elapsed;
    }
    e_laserIndex++;
    
}

void UpdateMainMenu() {
    if (keys[SDL_SCANCODE_TAB])
        state = STATE_GAME_LEVEL;
}

// draw main menu
void RenderMainMenu(GLuint font_texture) {
    modelMatrix.identity();
    modelMatrix.Translate(-2.0f, 1.5f, 0.0f);
    program->setModelMatrix(modelMatrix);
    DrawText(program, font_texture, "SPACE INVADERS", .4f, -.2f);
    
    modelMatrix.Translate(-0.65f, -1.0f, 0.0f);
    program->setModelMatrix(modelMatrix);
    DrawText(program, font_texture, "PRESS TAB TO BEGIN", .4f, -.2f);
}

void RenderGameLevel(Entity* ship, Entity* p_laser, std::vector<Entity*> e_lasers,
                     std::vector<Entity*> enemies, float elapsed)
{
    // draw player
    ship->matrix.Translate(ship->x, -1.5, 0.0f);
    program->setModelMatrix(ship->matrix);
    ship->Draw();
    ship->matrix.identity();
    
    // draw enemies moving
    // had hard time making them move properly
    for (int i = 0; i < enemies.size(); i++) {
        if (enemies[i]->alive) {
            if (enemies[topLeft]->x > -3.5f) {
                enemies[i]->x -= elapsed*enemies[i]->speed;
                enemies[i]->matrix.Translate(-elapsed*1.5f, 0.0f, 0.0f);
                enemies[i]->Draw();
            }
            else {
                if (enemies[topRight]->x < 1.7f) {
                    enemies[i]->x += elapsed*enemies[i]->speed;
                    enemies[i]->matrix.Translate(elapsed*enemies[i]->speed, 0.0f, 0.0f);
                    enemies[i]->Draw();
                }
            }
        }
    }
    
    // draw laser; laser does not move properly
    p_laser->matrix.Translate(p_laser->x, p_laser->y, 0.0f);
    program->setModelMatrix(p_laser->matrix);
    p_laser->Draw();
    p_laser->matrix.identity();
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

void Update(Entity* ship, Entity* p_laser, float elapsed) {
    UpdateMainMenu();
    
    // Moving left
    if (keys[SDL_SCANCODE_A]) {
        if (ship->x > -3.2f) {
            ship->x -= elapsed*ship->speed;
        }
    }
    
    // Moving right
    if (keys[SDL_SCANCODE_D]) {
        if (ship->x < 1.7f) {
            ship->x += elapsed*ship->speed;
        }
    }
    
    // shoot laser
    if (keys[SDL_SCANCODE_SPACE]) {
        playerShoot(p_laser, ship, elapsed);
    }
}


void CleanUp() {
    SDL_Quit();
}

int main(int argc, char *argv[])
{
    Setup();
    SDL_Event event;
    
    glViewport(0, 0, 640, 360);
    
    Matrix shipMatrix;
    Matrix enemyMatrix;
    Matrix p_laserMatrix;
    Matrix e_laserMatrix;
    
    GLuint font = LoadTexture("font1.png");
    
    projectionMatrix.setOrthoProjection(-3.55, 3.55, -2.0f, 2.8f, -1.0f, 1.0f);
    
    program = new ShaderProgram(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    float lastFrameTicks = 0.0f;
    float startingY = 1.8f;
    
    // load sprite sheet
    GLuint spriteSheetTexture = LoadTexture("sprites.png");
    SheetSprite shipSprite = SheetSprite(program, spriteSheetTexture, 0.0f/128.0f, 0.0f/128.0f, 62.0f/128.0f, 59.0f/128.0f, 0.5f);
    SheetSprite laserSprite = SheetSprite(program, spriteSheetTexture, 62.0f/128.0f, 61.0f/128.0f, 9.0f/128.0f, 37.0f/128.0f, 0.2f);
    SheetSprite enemySprite = SheetSprite(program, spriteSheetTexture, 0.0f/128.0f, 61.0f/128.0f, 60.0f/128.0f, 60.0f/128.0f, 0.4f);
    
    // player model
    Entity* ship = new Entity(program, shipMatrix, shipSprite);
    
    // only one player projectile
    Entity* p_laser = new Entity(program, p_laserMatrix, laserSprite);
    p_laser->x = -2000.0f;
    p_laser->y = -1.5f+ship->height/2;
    p_laser->matrix.Translate(-2000.0f, 0.0f, 0.0f);
    
    // enemy projectiles
    std::vector<Entity*> enemy_lasers;
    for (int i = 0; i < MAX_LASERS; i++) {
        Entity* laser = new Entity(program, e_laserMatrix, laserSprite);
        laser->x = -2000.0f;
        laser->matrix.Translate(laser->x, 0.0f, 0.0f);
        enemy_lasers.push_back(laser);
    }
    
    // enemies
    std::vector<Entity*> enemies;
    for (int i = 0; i < 4; i++) {
        for (int i = 0; i < 7; i++) {
            Entity* enemy = new Entity(program, enemyMatrix, enemySprite);
            enemy->x = -2.3f + float(i/2.0f);
            enemy->y = (startingY - (i / 7)) / 2.0f;
            enemy->matrix.Translate(enemy->x, enemy->y, 0.0f);
            enemies.push_back(enemy);
        }
        startingY += 1.0f;
    }
    
    while (!ProcessEvents(event)) {
        float ticks = (float)SDL_GetTicks()/1000.0f;
        float elapsed = ticks - lastFrameTicks;
        lastFrameTicks = ticks;
        
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        
        program->setProjectionMatrix(projectionMatrix);
        program->setViewMatrix(viewMatrix);
        program->setModelMatrix(modelMatrix);
        modelMatrix.identity();
        
        switch (state) {
            case STATE_MAIN_MENU:
                RenderMainMenu(font);
                break;
            case STATE_GAME_LEVEL:
                RenderGameLevel(ship, p_laser, enemy_lasers, enemies, elapsed);
                break;
            case STATE_GAME_OVER:
                RenderGameOver(font);
                break;
        }
        
        Update(ship, p_laser, elapsed);
        
        SDL_GL_SwapWindow(displayWindow);
    }
    
    CleanUp();
    return 0;
}
