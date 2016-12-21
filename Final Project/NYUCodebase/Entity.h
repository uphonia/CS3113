//
//  Entity.h
//  NYUCodebase
//
//  Created by Angela Wu on 11/11/16.
//  Copyright © 2016 Ivan Safrin. All rights reserved.
//

#ifndef Entity_h
#define Entity_h

#include "ShaderProgram.h"
#include "SheetSprite.h"

#define FIXED_TIMESTEP 0.0166666f
#define MAX_TIMESTEPS 6

class Entity {
public:
    Entity() {}
    Entity(std::string type, bool isStatic, float x, float y, Matrix matrix, GLuint sheet) : type(type),
        isStatic(isStatic), x(x), y(y), matrix(matrix), spriteSheet(sheet) {}
    
    void Update(float elapsed);
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

    bool collidesWith(Entity* enemy, Entity* exit) {
        if (type == "PLAYER") {
            if (x+width/2 >= enemy->x-width/2 || x-width/2 <= enemy->x+width/2 || y-height/2 <= enemy->y+height/2) {
                collided = true;
            }
            if (x+width/2 >= exit->x-width/2 || x-width/2 <= exit->x+width/2 || y-height/2 <= exit->y+height/2) {
                exited = true;
            }
        }
        
        int gridX = 0;
        int gridY = 0;
        
        
        return true;
    }
    
    float x;
    float y;
    float x_velocity = 0.5;
    float y_velocity = 0.5;
    float x_acceleration;
    float y_acceleration = -9.81;
    float x_friction;
    float y_friction;
    float x_gravity;
    float y_gravity;
    
    int gridX = 0;
    int gridY = 0;
    
    float width;
    float height;
    
    bool isStatic;
    bool isSolid = true;
    
    bool collided;
    bool exited;
    
    std::string type;
    Matrix matrix;
    GLuint spriteSheet;
    int index;
    
    bool collidedTop;
    bool collidedBottom;
    bool collidedLeft;
    bool collidedRight;
};

#endif /* Entity_h */
