/*
 
 Angela Wu N14475962
 CS3113
 Bonus - Pong (Added sound)
 
 Controls:
 
 Left Paddle: W and S
 Right Paddle: Up and Down arrow key
 
 */

#ifdef _WINDOWS
#include <GL/glew.h>
#endif
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_image.h>
#include "Matrix.h"
#include "ShaderProgram.h"
#include <SDL_mixer.h>

#ifdef _WINDOWS
#define RESOURCE_FOLDER ""
#else
#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

SDL_Window* displayWindow;

// initializes SDL_mixer;

class Entity {
public:
    Entity(float x, float y, float width, float height): x(x), y(y), width(width), height(height){}
    
    float x;
    float y;
    float width;
    float height;
    
    float speed = 2.3f;
    float x_dir = cos(rand()/1000.0f);
    float y_dir = sin(rand()/1000.0f);
};

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
    // setup SDL
    SDL_Init(SDL_INIT_VIDEO);
    displayWindow = SDL_CreateWindow("Homework 2 - Pong", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, SDL_WINDOW_OPENGL);
    SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
    SDL_GL_MakeCurrent(displayWindow, context);
    
}

int main(int argc, char *argv[])
{
    SDL_Event event;
    bool done = false;
    
    Setup();
    
    glViewport(0, 0, 800, 600);
    
    Matrix projectionMatrix;
    Matrix modelMatrix;
    Matrix viewMatrix;
    
    projectionMatrix.setOrthoProjection(-3.55, 3.55, -2.0f, 2.0f, -1.0f, 1.0f);
    
    ShaderProgram program(RESOURCE_FOLDER"vertex.glsl", RESOURCE_FOLDER"fragment.glsl");
    
    // Keeping time
    float lastFrameTicks = 0.0f;
    
    // Blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096);
    Mix_Chunk *bounce = Mix_LoadWAV("bounce.wav");
    Mix_Chunk *score = Mix_LoadWAV("score.wav");
    Mix_Music *music = Mix_LoadMUS("music.mp3");
    
    // New objects
    Entity* left_paddle = new Entity(-3.275, 0.0, .15, .6);
    Entity* right_paddle = new Entity(3.275, 0.0, .15, .6);
    Entity* ball = new Entity(0.0, 0.0, .15, .15);
    
    const Uint8 *keys = SDL_GetKeyboardState(NULL);
    
    while (!done) {
        // play music
        Mix_PlayMusic(music, -1);
        
        float ticks = (float)SDL_GetTicks()/1000.0f;
        float elapsed = ticks - lastFrameTicks;
        lastFrameTicks = ticks;
        
        ball->y += ball->speed*elapsed*ball->y_dir;
        ball->x += ball->speed*elapsed*ball->x_dir;
        
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
                done = true;
            }
        }
        
        // left paddle control
        if (keys[SDL_SCANCODE_W]) {
            if (left_paddle->y+left_paddle->height/2 < 2.0) {
                left_paddle->y += elapsed;
            }
        }
        if (keys[SDL_SCANCODE_S]) {
            if (left_paddle->y-left_paddle->height/2 > -2.0) {
                left_paddle->y -= elapsed;
            }
        }
        
        // right paddle control
        if (keys[SDL_SCANCODE_UP]) {
            if (right_paddle->y+right_paddle->height/2 < 2.0) {
                right_paddle->y += elapsed;
            }
        }
        
        if (keys[SDL_SCANCODE_DOWN]) {
            if (right_paddle->y-right_paddle->height/2 > -2.0) {
                right_paddle->y -= elapsed;
            }
        }
        
        // left player wins
        if (ball->x > (right_paddle->x+right_paddle->width/2)) {
            Mix_PlayChannel(-1, score, 0);
            std::cout << "Left Player wins!\n";
            ball->x = 0;
            ball->y = 0;
        }
        
        // right player wins
        else if (ball->x < (left_paddle->x-left_paddle->width/2)) {
            Mix_PlayChannel(-1, score, 0);
            std::cout << "Right Player wins!\n";
            ball->x = 0;
            ball->y = 0;
        }
        
        // ball collision with top and bottom of screen
        else if (ball->y+ball->width/2 > 2.0f || ball->y-ball->width/2 < -2.0f) {
            ball->y_dir *= -1.0;
            Mix_PlayChannel(-1, bounce, 0);
        }
        
        // ball collision with left paddle
        else if (ball->x-ball->width/2 <= (left_paddle->x+left_paddle->width/2) &&
                 ball->y+ball->height/2 <= (left_paddle->y+left_paddle->height/2+ball->height) &&
                 ball->y+ball->height/2 >= (left_paddle->y-left_paddle->height/2+ball->height))
        {
            ball->x_dir *= -1.0;
            Mix_PlayChannel(-1, bounce, 0);
        }
        
        // ball collision with right paddle
        else if (ball->x+ball->width/2 >= (right_paddle->x-right_paddle->width/2) &&
                 ball->y+ball->height/2 <= (right_paddle->y+right_paddle->height/2+ball->height) &&
                 ball->y+ball->height/2 >= (right_paddle->y-right_paddle->height/2+ball->height))
        {
            ball->x_dir *= -1.0;
            Mix_PlayChannel(-1, bounce, 0);
        }
        
        // Drawing
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        
        program.setModelMatrix(modelMatrix);
        program.setProjectionMatrix(projectionMatrix);
        program.setViewMatrix(viewMatrix);
        
        glUseProgram(program.programID);
        
        modelMatrix.identity();
        
        // left paddle
        float left_vertices[] = {
            -3.35, -0.3f+left_paddle->y,
            -3.2, -0.3f+left_paddle->y,
            -3.2, 0.3f+left_paddle->y,
            -3.35, -0.3f+left_paddle->y,
            -3.2, 0.3f+left_paddle->y,
            -3.35, 0.3f+left_paddle->y
        };
        
        glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, left_vertices);
        glEnableVertexAttribArray(program.positionAttribute);
        
        float left_coords[] = {0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0};
        glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, left_coords);
        glEnableVertexAttribArray(program.texCoordAttribute);
        
        glDrawArrays(GL_TRIANGLES, 0, 6);
        
        glDisableVertexAttribArray(program.positionAttribute);
        glDisableVertexAttribArray(program.texCoordAttribute);
        
        // right paddle
        program.setModelMatrix(modelMatrix);
        
        float right_vertices[] = {
            3.35, -0.3f+right_paddle->y,
            3.2, -0.3f+right_paddle->y,
            3.2, 0.3f+right_paddle->y,
            3.35, -0.3f+right_paddle->y,
            3.2, 0.3f+right_paddle->y,
            3.35, 0.3f+right_paddle->y
        };
        
        glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, right_vertices);
        glEnableVertexAttribArray(program.positionAttribute);
        
        float right_coords[] = {0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0};
        glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, right_coords);
        glEnableVertexAttribArray(program.texCoordAttribute);
        
        glDrawArrays(GL_TRIANGLES, 0, 6);
        
        glDisableVertexAttribArray(program.positionAttribute);
        glDisableVertexAttribArray(program.texCoordAttribute);
        
        // ball
        program.setModelMatrix(modelMatrix);
        
        float ball_vertices[] = {
            0.1f+ball->x, -0.05f+ball->y,
            -0.1f+ball->x, -0.05f+ball->y,
            0.1f+ball->x, 0.1f+ball->y,
            -0.1f+ball->x, -0.05f+ball->y,
            -0.1f+ball->x, 0.1f+ball->y,
            0.1f+ball->x, 0.1f+ball->y
        };
        
        glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, ball_vertices);
        glEnableVertexAttribArray(program.positionAttribute);
        
        float ball_coords[] = {0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0};
        glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, ball_coords);
        glEnableVertexAttribArray(program.texCoordAttribute);
        
        glDrawArrays(GL_TRIANGLES, 0, 6);
        
        glDisableVertexAttribArray(program.positionAttribute);
        glDisableVertexAttribArray(program.texCoordAttribute);
        
        SDL_GL_SwapWindow(displayWindow);
    }
    
    // clean up sound
    Mix_FreeChunk(bounce);
    Mix_FreeChunk(score);
    Mix_FreeMusic(music);
    
    SDL_Quit();
    return 0;
}
