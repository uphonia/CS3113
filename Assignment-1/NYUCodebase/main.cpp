#ifdef _WINDOWS
#include <GL/glew.h>
#endif
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_image.h>
#include "Matrix.h"
#include "ShaderProgram.h"

#ifdef _WINDOWS
#define RESOURCE_FOLDER ""
#else
#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

SDL_Window* displayWindow;
ShaderProgram* program;

class Entity {
public:
    Entity(ShaderProgram* program, Matrix matrix, GLuint textureID, float x, float y) : program(program), matrix(matrix), textureID(textureID), x(x), y(y) {}
    
    ShaderProgram* program;
    GLuint textureID;
    Matrix matrix;
    
    float x;
    float y;
    
    void Draw() {
        program->setModelMatrix(matrix);
        
        glBindTexture(GL_TEXTURE_2D, textureID);
        
        float vertices[] = {
            -0.5f, -0.5f,
            0.5f, -0.5f,
            0.5f, 0.5f,
            -0.5f, -0.5f,
            0.5f, 0.5f,
            -0.5f, 0.5f
        };
        
        glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices);
        glEnableVertexAttribArray(program->positionAttribute);
        
        float texCoords[] = {0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0};
        glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
        glEnableVertexAttribArray(program->texCoordAttribute);
        
        glDrawArrays(GL_TRIANGLES, 0, 6);
        
        glDisableVertexAttribArray(program->positionAttribute);
        glDisableVertexAttribArray(program->texCoordAttribute);

    }
};

void DrawSpriteSheetSprite(ShaderProgram* progam, int index, int spriteCountX, int spriteCountY) {
    float u = (float)(((int)index) % spriteCountX) / (float) spriteCountX;
    float v = (float)(((int)index) / spriteCountX) / (float) spriteCountY;
    float spriteWidth = 1.0/(float)spriteCountX;
    float spriteHeight = 1.0/(float)spriteCountY;
    
    GLfloat texCoords[] = {
        u, v+spriteHeight,
        u+spriteWidth, v,
        u, v,
        u+spriteWidth, v,
        u, v+spriteHeight,
        u+spriteWidth, v+spriteHeight
    };
    
    float vertices[] = {
        -0.5f, -0.5f,
        0.5f, 0.5f,
        -0.5f, 0.5f,
        0.5f, 0.5f,
        -0.5f, -0.5f,
        0.5f, -0.5f
    };
    
    glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(program->positionAttribute);
    
    glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
    glEnableVertexAttribArray(program->texCoordAttribute);
    
    glDrawArrays(GL_TRIANGLES, 0, 6);
    
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
    displayWindow = SDL_CreateWindow("Hungry Cat", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 360, SDL_WINDOW_OPENGL);
    SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
    SDL_GL_MakeCurrent(displayWindow, context);

    
}

// SDL event loops
bool ProcessEvents(SDL_Event& event) {
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
            return true;
        }
    }
    return false;
}

void Update(Entity* cat, Entity* milk, Entity* fish) {
    if (cat->x > 2.0f || cat->x < -2.0f)
        cat->x *= -1.0;
    
}

void Render(Entity* cat, Entity* milk, Entity* fish) {
    cat->Draw();
    milk->Draw();
    fish->Draw();
    
}

void CleanUp() {
    SDL_Quit();
}

int main(int argc, char *argv[])
{
    Setup();
    SDL_Event event;
    
    glViewport(0, 0, 640, 360);
    
    Matrix projectionMatrix;
    Matrix modelMatrix;
    Matrix viewMatrix;
    
    Matrix catMatrix;
    Matrix milkMatrix;
    Matrix fishMatrix;
    
    projectionMatrix.setOrthoProjection(-3.55, 3.55, -2.0f, 2.0f, -1.0f, 1.0f);
    
    program = new ShaderProgram(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");
    
    // Blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    //modelMatrix.identity();
    
    //glUseProgram(program->programID);
    
    float lastFrameTicks = 0.0f;
    float catX = 0.0f;
    
    Entity* cat = new Entity(program, catMatrix, LoadTexture("cat.png"), 0.0, -1.5);
    Entity* milk = new Entity(program, milkMatrix, LoadTexture("milk.png"), -1.0, 0.0);
    Entity* fish = new Entity(program, fishMatrix, LoadTexture("fish.png"), 4.0, 0.0);
    
    cat->matrix.Translate(0.0f, -1.0f, 0.0f);
    milk->matrix.Translate(1.5f, 0.0f, 0.0f);
    fish->matrix.Translate(-1.5f, 0.0f, 0.0f);
    
    while (!ProcessEvents(event)) {
        
        float angle;
        float ticks = (float)SDL_GetTicks()/1000.0f;
        float elapsed = ticks - lastFrameTicks;
        lastFrameTicks = ticks;
        cat->x += elapsed;
        
        // Drawing
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f); // Makes background white
        glClear(GL_COLOR_BUFFER_BIT);
        
        program->setModelMatrix(modelMatrix);
        program->setProjectionMatrix(projectionMatrix);
        program->setViewMatrix(viewMatrix);
        
        Render(cat, milk, fish);
        //Update(cat, milk, fish);
        
        SDL_GL_SwapWindow(displayWindow);
    }
    
    CleanUp();
    return 0;
}
