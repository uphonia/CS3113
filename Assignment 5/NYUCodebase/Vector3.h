//
//  Vector3.h
//  NYUCodebase
//
//  Created by Angela Wu on 11/30/16.
//  Copyright © 2016 Ivan Safrin. All rights reserved.
//

#ifndef Vector3_h
#define Vector3_h

class Vector3 {
public:
    Vector3();
    Vector3(float x, float y, float z) : x(x), y(y), z(z) {}
    
    float x;
    float y;
    float z;
};

#endif /* Vector3_h */
