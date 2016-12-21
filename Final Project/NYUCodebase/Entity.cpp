//
//  Entity.cpp
//  NYUCodebase
//
//  Created by Angela Wu on 11/11/16.
//  Copyright © 2016 Ivan Safrin. All rights reserved.
//

#include <stdio.h>
#include "Entity.h"

void Entity::Update(float elapsed) {
    
}

void Entity::Draw(ShaderProgram* program, Matrix& matrix, int index) {
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

bool Entity::collidesWith(Entity* enemy, Entity* exit) {
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
