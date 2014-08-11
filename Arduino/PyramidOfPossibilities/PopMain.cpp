//
//  Main.cpp
//  PlatformCode
//
//  Created by Bill on 8/11/14.
//  Copyright (c) 2014 RN. All rights reserved.
//

#include "PoPMain.h"
#include "Constants.h"

#include "Arduino.h"
#include "Accelerometer.h"
#include "RNLights.h"
#include "RNChaser.h"
#include "RNInfo.h"
#include "hsv2rgb.h"
#include "Controller.h"
#include "RNSerial.h"
#include "ledPositions.h"
#include "watchdog.h"
#include "RNEEPROM.h"
#include "malloc.h"
#include "mac.h"
#include <math.h>




RNInfo *info;
RNController * controller;
Platform platform( 0,0,0,700,0,0);

extern RNLights lights;
static int heapSize(){
    return mallinfo().uordblks;
}

void debugTriadPositions();

void setupMain() {
    Serial.println("PoP board starting");
    print_mac();
    char  platformData[9];
    Platform platform(0,0,0,0,0,0);
    bool success = readFromEEPROM(sizeof(Platform), (char*) &platform);
    if (success) {
        Serial.print("Read from EEPROM ");
        Serial.print(platform.identifier);
        Serial.println();
    }
    int bytesRead = Serial.readBytes((char *) platformData, 9);
    Serial.print(bytesRead);
    Serial.println(" bytes read");
    Serial.print(sizeof(Platform));
    Serial.println(" platform size");
    if (bytesRead == 9) {
        bool success = platform.initialize(platformData, 9);
        if (success) {
            writeToEEPROM(sizeof(Platform), (char*) &platform);
            Serial.println("success, wrote to EEPROM");
        }
        else
            Serial.println("Fail parsing data from central");
        Serial.print(platform.y);
    }
    initializeAccelerometer(constants.PULSE_THSX,constants.PULSE_THSY,
                            constants.PULSE_THSZ);
    setupSerial2(constants.serial2BaudRate);

    info = new RNInfo(constants.LEDs, platform);
    controller = new RNController(*info);


#ifndef FULL_STRIP
    debugTriadPositions();
#endif

    createWatchdog(constants.watchdogTimeout);


}

const uint8_t chunk = constants.brightnessChunkSize;
uint8_t scaleBrightness(uint8_t value) {
    uint8_t result = 0;
    while (value > chunk) {
        result += chunk;
        value -= chunk;
        value/= 2;
    }
    return result + value;

}

unsigned long avgTime = 0;
int count = 0;

void accelerometerCallback( float totalG, float directionalG[3], uint8_t source)  {
    info->accelerometerCallback(totalG,directionalG, source);
}

void capOverallBrightness(RNLights & lights) {
    uint8_t avgPixelBrightness = lights.getAvgPixelBrightness();
    uint8_t avgBrightness = avgPixelBrightness * lights.getBrightness()/256;
    if (avgBrightness > 16) {

        int goal= scaleBrightness(avgBrightness);

        int newBrightness = goal * 255 / avgPixelBrightness;

#ifdef RN_PRINT_BRIGHTNESS_ADJUSTMENTS
        info->printf("Avg brightness is %d/%d, goal is %d, Reducing brightness from %d -> %d\n",
                     avgPixelBrightness, avgBrightness, goal, lights.getBrightness(), newBrightness);
#endif /* RN_PRINT_BRIGHTNESS_ADJUSTMENTS */

        lights.setBrightness(newBrightness);
    }
    //  else info.printf("Avg brightness is %d/%d\n", avgPixelBrightness, avgBrightness);

}





void loop() {
    refreshWatchdog();
    unsigned long startMicros = micros();

    updateAccelerometer();

    // display lights
    lights.reset();
    controller->paint(lights);
    capOverallBrightness(lights);
    lights.show();
    
    
    unsigned long endMicros = micros();
    avgTime = (15*avgTime + endMicros - startMicros)/16;
    
    int timeToDelay = (10 - (endMicros - startMicros)/1000);
    if (timeToDelay > 0)
        delay(timeToDelay);
    int blink = (millis() /100)%2;
    digitalWrite(ONBOARD_LED_PIN, blink);
    if (count++ >= 100) {
        
        // Print head size for debugging.
#ifdef RN_PRINT_HEAP_SIZE
        info->printf("Avg time = %5d, delay = %dms, heapSize = %d\n",
                     avgTime, timeToDelay, heapSize());
        count = 0;
#endif /* RN_PRINT_HEAP_SIZE */
        
    }
    // Serial.println(millis()/10);
}


void debugTriadPositions() {

    info->printf("Running. id = %3d, xyz = %4d,%4d,%4d\n", info->identifier, info->x, info->y,info->z);
    info->printf("%-3s  %-4s %-4s %-11s    %-4s %-4s   %-11s  %-11s\n",
                 "id", "x", "y", "angle", "g x", "g y", "angle", "radius");
    int minAngleLED = 0;
    int maxAngleLED = 0;
    float maxAngle = 0;
    float minAngle =1;
    int closestLED = 0;
    float closestLEDRange = 100000;
    for(int i = 0; i < info->numLEDs; i++)  {
        info->printf("%3d  %4d %4d %11f    %4d %4d   %11f  %11f, %11f\n",
                     i, getLEDXPosition(i),
                     getLEDYPosition(i),
                     info->getLocalAngle(i)*360,
                     info->x + getLEDXPosition(i),
                     info->y + getLEDYPosition(i),
                     info->getGlobalAngle(i)*360,
                     (info->getGlobalAngle(i) - info->getPlatformGlobalAngle())*360,
                     info->getGlobalRadius(i));
        float angleDiff = info->getGlobalAngle(i) - info->getPlatformGlobalAngle();
        angleDiff = angleDiff  - round(angleDiff);

        if (maxAngle < angleDiff) {
            maxAngle = angleDiff;
            maxAngleLED = i;
        }
        if (minAngle > angleDiff) {
            minAngle = angleDiff;
            minAngleLED = i;
        }
        if (closestLEDRange > info->getGlobalRadius(i)) {
            closestLEDRange =info->getGlobalRadius(i);
            closestLED = i;
        }

    }



    lights.setAllPixelColors(0, 20, 0);
    lights.setPixelColor(minAngleLED, 255, 0, 0);
    lights.setPixelColor(maxAngleLED, 0, 0, 255);
    lights.setPixelColor(closestLED, 80,80,80);

    info->printf("minimum led %3d at %11f\n", minAngleLED, minAngle*360);
    info->printf("closest led %3d at %11f\n", closestLED, closestLEDRange*360);
    info->printf("maximum led %3d at %11f\n", maxAngleLED, maxAngle*360);

    lights.show();
    delay(10000);
    lights.reset();
}






