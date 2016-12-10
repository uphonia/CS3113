/*
 
 Angela Wu N14475962
 CS3113
 Assignment 5 - SAT
 
 Controls:
 
 
 */

#ifdef _WINDOWS
#include <GL/glew.h>
#endif
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_image.h>
#include "Matrix.h"
#include "ShaderProgram.h"
#include <vector>

#ifdef _WINDOWS
#define RESOURCE_FOLDER ""
#else
#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

SDL_Window* displayWindow;
const Uint8 *keys = SDL_GetKeyboardState(NULL);

Matrix triangleMatrix;
Matrix square1Matrix;
Matrix square2Matrix;

#define PI 3.1415926

class Vector3 {
public:
    Vector3() {
        x = 0.0f;
        y = 0.0f;
        z = 0.0f;
    }
    
    Vector3(float x, float y, float z) : x(x), y(y), z(z) {}
    
    void normalize() {
        float length = sqrt(x*x + y*y);
        x /= length;
        y /= length;
    }
    
    float x;
    float y;
    float z;
};

class Entity {
public:
    Entity() {
        position = Vector3();
        velocity = Vector3();
    }
    
    Entity(Matrix matrix, int numVertices, Vector3 position, Vector3 velocity, float rotation) : matrix(matrix), numVertices(numVertices), position(position), velocity(velocity), rotation(rotation) {}
    
    Matrix matrix;
    float rotation;
    
    Vector3 position;
    Vector3 velocity;
    Vector3 direction;
    
    std::vector<Vector3> ePoints;
    
    int numVertices;
};

Entity* triangle;
Entity* square1;
Entity* square2;

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

bool testSATSeparationForEdge(float edgeX, float edgeY, const std::vector<Vector3> &points1, const std::vector<Vector3> &points2) {
    float normalX = -edgeY;
    float normalY = edgeX;
    float len = sqrtf(normalX*normalX + normalY*normalY);
    normalX /= len;
    normalY /= len;
    
    std::vector<float> e1Projected;
    std::vector<float> e2Projected;
    
    for(int i=0; i < points1.size(); i++) {
        e1Projected.push_back(points1[i].x * normalX + points1[i].y * normalY);
    }
    for(int i=0; i < points2.size(); i++) {
        e2Projected.push_back(points2[i].x * normalX + points2[i].y * normalY);
    }
    
    std::sort(e1Projected.begin(), e1Projected.end());
    std::sort(e2Projected.begin(), e2Projected.end());
    
    float e1Min = e1Projected[0];
    float e1Max = e1Projected[e1Projected.size()-1];
    float e2Min = e2Projected[0];
    float e2Max = e2Projected[e2Projected.size()-1];
    float e1Width = fabs(e1Max-e1Min);
    float e2Width = fabs(e2Max-e2Min);
    float e1Center = e1Min + (e1Width/2.0);
    float e2Center = e2Min + (e2Width/2.0);
    float dist = fabs(e1Center-e2Center);
    float p = dist - ((e1Width+e2Width)/2.0);
    
    if(p < 0) {
        return true;
    }
    return false;
}

bool checkSATCollision(const std::vector<Vector3> &e1Points, const std::vector<Vector3> &e2Points) {
    for(int i=0; i < e1Points.size(); i++) {
        float edgeX, edgeY;
        
        if(i == e1Points.size()-1) {
            edgeX = e1Points[0].x - e1Points[i].x;
            edgeY = e1Points[0].y - e1Points[i].y;
        } else {
            edgeX = e1Points[i+1].x - e1Points[i].x;
            edgeY = e1Points[i+1].y - e1Points[i].y;
        }
        
        bool result = testSATSeparationForEdge(edgeX, edgeY, e1Points, e2Points);
        if(!result) {
            return false;
        }
    }
    
    for(int i=0; i < e2Points.size(); i++) {
        float edgeX, edgeY;
        
        if(i == e2Points.size()-1) {
            edgeX = e2Points[0].x - e2Points[i].x;
            edgeY = e2Points[0].y - e2Points[i].y;
        } else {
            edgeX = e2Points[i+1].x - e2Points[i].x;
            edgeY = e2Points[i+1].y - e2Points[i].y;
        }
        bool result = testSATSeparationForEdge(edgeX, edgeY, e1Points, e2Points);
        if(!result) {
            return false;
        }
    }
    return true;
}

void Setup() {
    // setup SDL
    SDL_Init(SDL_INIT_VIDEO);
    displayWindow = SDL_CreateWindow("Assignment 5", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, SDL_WINDOW_OPENGL);
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

void Update(float elapsed) {
    // Moving left
    if (keys[SDL_SCANCODE_A]) {
        triangle->position.x -= elapsed;
    }
    
    // Moving right
    if (keys[SDL_SCANCODE_D]) {
        triangle->position.x += elapsed;
    }
    
    // Moving Up
    if (keys[SDL_SCANCODE_W]) {
        triangle->position.y += elapsed;
    }
    
    // Moving down
    if (keys[SDL_SCANCODE_S]) {
        triangle->position.y -= elapsed;
    }
    
    if (keys[SDL_SCANCODE_SPACE]) {
        triangle->rotation += 70.0f * elapsed;
        square1->rotation += 60.0f * elapsed;
        square2->rotation += 120.0f * elapsed;
    }
    
}

void collisionResponse(Entity* entity1, Entity* entity2) {
    int maxChecks = 10;
    while(checkSATCollision(entity1->ePoints, entity2->ePoints) && maxChecks > 0) {
        std::cout << "test" << std::endl;
        Vector3 responseVector = Vector3(entity1->position.x - entity2->position.x, entity1->position.y - entity2->position.y, 0.0);
        std::cout << responseVector.x << " " << responseVector.y << " " << responseVector.z << std::endl;
        responseVector.normalize();
        
        entity1->position.x -= responseVector.x * 0.002;
        entity1->position.y -= responseVector.y * 0.002;
        
        entity2->position.x += responseVector.x * 0.002;
        entity2->position.y -= responseVector.y * 0.002;
        maxChecks -= 1;
    }
}

int main(int argc, char *argv[])
{
    SDL_Event event;
    float angle = 0;
    
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
    
    // New objects
    triangle = new Entity(triangleMatrix, 3, Vector3(0.0, 0.0, 0.0), Vector3(2.0, 2.0, 0.0), 0.0f);
    triangle->ePoints.push_back(Vector3(0.5, -0.5, 0.0));
    triangle->ePoints.push_back(Vector3(0.0, 0.5, 0.0));
    triangle->ePoints.push_back(Vector3(-0.5, -0.5, 0.0));
    
    square1 = new Entity(square1Matrix, 6, Vector3(0.0, 0.0, 0.0), Vector3(2.0, 2.0, 0.0), 0.0f);
    square1->ePoints.push_back(Vector3(-2.0, -0.5, 0.0));
    square1->ePoints.push_back(Vector3(-3.2, -0.5, 0.0));
    square1->ePoints.push_back(Vector3(-3.2, 0.5, 0.0));
    square1->ePoints.push_back(Vector3(-2.0, 0.5, 0.0));
    
    square2 = new Entity(square2Matrix, 6, Vector3(0.0, 0.0, 0.0), Vector3(2.0, 2.0, 0.0), 0.0f);
    square2->ePoints.push_back(Vector3(1.0, -0.5, 0.0));
    square2->ePoints.push_back(Vector3(2.2, -0.5, 0.0));
    square2->ePoints.push_back(Vector3(2.2, 0.5, 0.0));
    square2->ePoints.push_back(Vector3(1.0, 0.5, 0.0));
    
    std::vector<Entity*> entities;
    entities.push_back(triangle);
    entities.push_back(square1);
    entities.push_back(square2);
    
    while (!ProcessEvents(event)) {
        
        float ticks = (float)SDL_GetTicks()/1000.0f;
        float elapsed = ticks - lastFrameTicks;
        lastFrameTicks = ticks;
        
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        
        program.setProjectionMatrix(projectionMatrix);
        program.setViewMatrix(viewMatrix);
        program.setModelMatrix(triangleMatrix);
        
        glUseProgram(program.programID);
        
        Update(elapsed);
        
        collisionResponse(triangle, square1);
        collisionResponse(square1, square2);
        collisionResponse(triangle, square2);
        
        // drawing of objects
        triangleMatrix.identity();
        triangleMatrix.Translate(triangle->position.x, triangle->position.y, 0.0);
        triangleMatrix.Rotate(triangle->rotation*(float)PI/180);
        triangleMatrix.Scale(0.5, 0.5, 0.0);
        
        float tri_vertices[] {
            0.5f, -0.5f,
            0.0f, 0.5f,
            -0.5f, -0.5f
        };
        
        glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, tri_vertices);
        glEnableVertexAttribArray(program.positionAttribute);
        
        glDrawArrays(GL_TRIANGLES, 0, 3);
        
        glDisableVertexAttribArray(program.positionAttribute);
        glDisableVertexAttribArray(program.texCoordAttribute);
        
        program.setModelMatrix(square1Matrix);

        square1Matrix.identity();
        square1Matrix.Translate(square1->position.x, square1->position.y, 0.0);
        square1Matrix.Rotate(square1->rotation*(float)PI/180);
        square1Matrix.Scale(0.5, 0.5, 0.0);
        
        float sq1_vertices[] {
            -2.0, -0.5f,
            -3.2, -0.5f,
            -3.2, 0.5f,
            -2.0, -0.5f,
            -3.2, 0.5f,
            -2.0, 0.5f
        };
        
        glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, sq1_vertices);
        glEnableVertexAttribArray(program.positionAttribute);
        
        glDrawArrays(GL_TRIANGLES, 0, 6);
        
        glDisableVertexAttribArray(program.positionAttribute);
        glDisableVertexAttribArray(program.texCoordAttribute);
        
        program.setModelMatrix(square1Matrix);
        
        square2Matrix.identity();
        square2Matrix.Translate(square2->position.x, square2->position.y, 0.0);
        square2Matrix.Rotate(square2->rotation*(float)PI/180);
        square2Matrix.Scale(2.0, 2.0, 0.0);
        
        float sq2_vertices[] {
            1.0, -0.5f,
            2.2, -0.5f,
            2.2, 0.5f,
            1.0, -0.5f,
            2.2, 0.5f,
            1.0, 0.5f
        };
        
        glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, sq2_vertices);
        glEnableVertexAttribArray(program.positionAttribute);
        
        glDrawArrays(GL_TRIANGLES, 0, 6);
        
        glDisableVertexAttribArray(program.positionAttribute);
        glDisableVertexAttribArray(program.texCoordAttribute);
        
        SDL_GL_SwapWindow(displayWindow);
    }
    
    SDL_Quit();
    return 0;
}
