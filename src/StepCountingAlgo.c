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
#include <stddef.h>
#include "StepCountingAlgo.h"
#include "ringbuffer.h"
#include "preProcessingStage.h"
#include "motionDetectStage.h"
#include "filterStage.h"
#include "scoringStage.h"
#include "detectionStage.h"
#include "postProcessingStage.h"

#include "string.h"

#define STRIDECONST 0.414

/* General data */
static met_t met;
static float bmrPerMinute;
static float stride;

static steps_t steps;
static float distance;

/* User data */
static gender_t gender;
static age_t age;
static height_t height;
static weight_t weight;

/* Buffers */
static ring_buffer_t rawBuf;
static ring_buffer_t ppBuf;
static ring_buffer_t mdBuf;
#ifndef SKIP_FILTER
static ring_buffer_t smoothBuf;
#endif
static ring_buffer_t peakScoreBuf;
static ring_buffer_t peakBuf;

static void increaseMET();
static void increaseDistance();
 
static void increaseStepCallback(void)
{
    steps++;
    increaseMET();
    increaseDistance();
}

static void increaseDistance() 
{
    /* compute distance dynamically */
    data_point_t lastDataPoint = getLastDataPoint();
    distance += 1.5 * lastDataPoint.peak_time;
}

static void increaseMET() 
{
    data_point_t lastDataPoint = getLastDataPoint();
    met += lastDataPoint.met * (lastDataPoint.peak_time / (60)); /* time in minutes */
}

void initUserData(char* userGender, uint8_t userAge, uint8_t userHeight, uint8_t userWeight) 
{
    /* init user information */
    gender = userGender;
    age = userAge;
    height = userHeight;
    weight = userWeight;

    /* init mbr */
    float bmr = strcmp(gender, "F") == 0 ? 
            (9.56 * weight) + (1.85 * height) - (4.68 * age) + 655 :
            (13.75 * weight) + (5 * height) - (6.76 * age) + 66;
    bmrPerMinute = bmr / (24 * 60); /* convert to bmr per min */

    /* init stride length */
    float height_float = height;
    stride = (height_float / 100) * STRIDECONST;
}

void initAlgo(char* gender, uint8_t age, uint8_t height, uint8_t weight)
{
    /* Init buffers */
    ring_buffer_init(&rawBuf);
    ring_buffer_init(&ppBuf);
    ring_buffer_init(&mdBuf);
#ifndef SKIP_FILTER
    ring_buffer_init(&smoothBuf);
#endif
    ring_buffer_init(&peakScoreBuf);
    ring_buffer_init(&peakBuf);

    initPreProcessStage(&rawBuf, &ppBuf, motionDetectStage);
#ifdef SKIP_FILTER
    initMotionDetectStage(&ppBuf, &mdBuf, scoringStage);
    initScoringStage(&mdBuf, &peakScoreBuf, detectionStage);
#else
    initMotionDetectStage(&ppBuf, &mdBuf, filterStage);
    initFilterStage(&mdBuf, &smoothBuf, scoringStage);
    initScoringStage(&smoothBuf, &peakScoreBuf, detectionStage);
#endif
    initDetectionStage(&peakScoreBuf, &peakBuf, postProcessingStage);
    initPostProcessingStage(&peakBuf, &increaseStepCallback);

    /* Set user data */
    initUserData(gender, age, height, weight);

    /* Set parameters */
    changeWindowSize(OPT_WINDOWSIZE);
    changeDetectionThreshold(OPT_DETECTION_THRESHOLD, OPT_DETECTION_THRESHOLD_FRAC);
    changeTimeThreshold(OPT_TIME_THRESHOLD);
    changeMotionThreshold(MOTION_THRESHOLD);

    currentTime = 0;
}

void processSample(time_accel_t time, accel_t x, accel_t y, accel_t z)
{
    preProcessSample(time, x, y, z);
}

void resetSteps(void)
{
    steps = 0;
    distance = 0;
    met = 0;
}

void resetAlgo(void)
{
    resetPreProcess();
    resetDetection();
    resetPostProcess();
    ring_buffer_init(&rawBuf);
    ring_buffer_init(&ppBuf);
    ring_buffer_init(&mdBuf);
#ifndef SKIP_FILTER
    ring_buffer_init(&smoothBuf);
#endif
    ring_buffer_init(&peakScoreBuf);
    ring_buffer_init(&peakBuf);
}

steps_t getSteps(void)
{
    return steps;
}

float getDistance(void) {
    /* constant stride length distance computation */
    float dist = steps * stride;

    return distance / 1000; /* convert from ms to s*/
}

calorie_t getCalories(void) 
{
    calorie_t kcal = met * bmrPerMinute / 1000; /* convert ot kcal */
    return kcal;
}
