/* 
The MIT License (MIT)

Copyright (c) 2020 Anna Brondin and Marcus Nordström

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef STEP_COUNTING_ALGO_H
#define STEP_COUNTING_ALGO_H
#include <stdint.h>
#include "config.h"

/**
    This function initializes user health information.
    @param gender
    @param age
    @param height meters
    @param weight kg
*/
void initUserData(char* userGender, uint8_t userAge, uint8_t userHeight, uint8_t userWeight);

/**
    Initializes all buffers and everything the algorithm needs.
    This function takes user information as input.
    @param gender
    @param age
    @param height meters
    @param weight kg
*/
void initAlgo(char* gender, uint8_t age, uint8_t height, uint8_t weight);

/**
    This function takes the raw accelerometry data and computes the entire algorithm
    @param time, the current time in ms
    @param x, the x axis
    @param y, the y axis
    @param z, the z axis
*/
void processSample(time_accel_t time, accel_t x, accel_t y, accel_t z);

/**
    Resets the number of walked steps
*/
void resetSteps(void);

/**
    Resets the entire algorithm
*/
void resetAlgo(void);

/**
    Returns the number of walked steps
    @return steps walked
*/
steps_t getSteps(void);

/**
    Returns the distance in meters
    @return distance walked
*/
float getDistance(void);

/**
    Returns the calories burned
    @return current caorie burn
*/
calorie_t getCalories(void);

/**
    Returns speed of the user
    @return steps per second
*/
float getStepsPerSec(void);

/* Extern variables */
extern double kcalories;
extern float bmr;
extern float stride;

#endif