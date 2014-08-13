//
//  Beacon.cpp
//
//  Created by Bill on 7/18/14.
//  Copyright (c) 2014 RN. All rights reserved.
//

#include "Beacon.h"
#include <math.h>

void Beacon::paint(RNLights & lights) {
    
    float angle = getAnimationMinutes() * parameters.rpm;


    for(int i = 0; i < lights.getNumPixels(); i++) {
        float diff = (info.getGlobalAngle(i) - angle) * parameters.numBeacons;
        
        float a = diff - roundf(diff);
        if (a < 0) a = -a;
        if (a < parameters.width) {
            uint8_t gradientPosition = a*256/parameters.width;
            uint32_t color = parameters.gradient.getColor(gradientPosition);
            lights.setPixelColor(i,color);
        }
    }

    info.showActivityWithSparkles(lights);
    
}

const char * Beacon:: name() {
    return "Beacon";
}
