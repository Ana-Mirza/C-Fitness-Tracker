/* 
The MIT License (MIT)

Copyright (c) 2020 Anna Brondin, Marcus Nordström and Dario Salvi

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
#include "detectionStage.h"

#include "postProcessingStage.h"
#ifdef DUMP_FILE
#include <stdio.h>
static FILE
    *postProcFile;
static FILE *postProcFile;
#endif

static steps_t stepCounter;
static ring_buffer_t *inBuff;
static data_point_t lastDataPoint;
static int16_t timeThreshold = 300; // in ms, this discards steps that are too close in time, 3 steps /s is a reasonable maximum
static void (*stepCallback)(void);

void initPostProcessingStage(ring_buffer_t *pInBuff, void (*stepCallbackIn)(void))
{
    inBuff = pInBuff;
    stepCallback = stepCallbackIn;
    stepCounter = 0;
    lastDataPoint.time = 0;
    lastDataPoint.magnitude = 0;

#ifdef DUMP_FILE
    postProcFile = fopen(DUMP_POSTPROC_FILE_NAME, "w+");
#endif
}

void postProcessingStage(void)
{
    if (!ring_buffer_is_empty(inBuff))
    {
        data_point_t dataPoint;
        ring_buffer_dequeue(inBuff, &dataPoint);
        if (lastDataPoint.time == 0)
        {
            lastDataPoint = dataPoint;
        }
        else
        {
            if ((dataPoint.time - lastDataPoint.time) > timeThreshold)
            {
                stepCounter++;

                /* Weighted step stripe */
                float magAvg = 1.6;
                float stepsPerSec = (float)stepCounter / ((float)dataPoint.time / 1000);

                if (stepsPerSec < magAvg) {
                    dataPoint.weight = 0.5;
                } else if (stepsPerSec == magAvg) {
                    dataPoint.weight = 1.0;
                } else if (stepsPerSec > magAvg) {
                    dataPoint.weight = 1.5;
                }

                /* Peak time interval */
                dataPoint.peak_time = dataPoint.time - lastDataPoint.time;

                /* Compute MET constant */
                if (dataPoint.orig_magnitude < 5) {
                    dataPoint.met = 2;
                } else if (dataPoint.orig_magnitude < 10) {
                    dataPoint.met = 4;
                } else if (dataPoint.orig_magnitude < 15) {
                    dataPoint.met = 9;
                } else if (dataPoint.orig_magnitude < 20) {
                    dataPoint.met = 12;
                } else if (dataPoint.orig_magnitude < 25) {
                    dataPoint.met = 17;
                } else if (dataPoint.orig_magnitude > 25) {
                    dataPoint.met = 23;
                }

                lastDataPoint = dataPoint;
                (*stepCallback)();

#ifdef DUMP_FILE
                if (postProcFile)
                {
                    if (!fprintf(postProcFile, "%lld, %lld, %lld, %lld, %lld, %f, %f\n", 
                        dataPoint.time, dataPoint.magnitude, dataPoint.orig_magnitude, dataPoint.met, dataPoint.peak_time, dataPoint.weight, stepsPerSec))
                        puts("error writing file");
                    fflush(postProcFile);
                }
#endif
            }
            else
            {
                if (dataPoint.magnitude > lastDataPoint.magnitude)
                {
                    lastDataPoint = dataPoint;
                }
            }
        }
    }
}

void resetPostProcess(void)
{
    lastDataPoint.magnitude = 0;
    lastDataPoint.time = 0;
    stepCounter = 0;
}

void changeTimeThreshold(int16_t thresh)
{
    timeThreshold = thresh;
}

data_point_t getLastDataPoint(void) {
    return lastDataPoint;
}

