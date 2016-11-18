//
//  SheetSprite.h
//  NYUCodebase
//
//  Created by Angela Wu on 10/18/16.
//  Copyright © 2016 Ivan Safrin. All rights reserved.
//

#pragma once
#ifndef SheetSprite_h
#define SheetSprite_h

#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_image.h>
#include "ShaderProgram.h"

class SheetSprite {
public:
    SheetSprite();
    SheetSprite(unsigned int textureID, float u, float v, float width, float height, float size) : textureID(textureID),
        u(u), v(v), width(width), height(height) {}
    
    void Draw(ShaderProgram* program);
    
    float size;
    float u;
    float v;
    float width;
    float height;
    
    unsigned int textureID;
};

#endif
