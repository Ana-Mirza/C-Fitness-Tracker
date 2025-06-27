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

#include "detectionStage.h"
#include "postProcessingStage.h"
#include "StepCountingAlgo.h"
#include "config.h"

#ifdef DUMP_FILE
#include <stdio.h>
static FILE
    *detectionFile;
static FILE *detectionFile;
#endif

static ring_buffer_t *inBuff;
static ring_buffer_t *outBuff;
static void (*nextStage)(void);

static double kcal = 0;
data_point_t lastDataPoint;

static magnitude_t mean = 0;
float rawMagnitudeMean = 0;
static accumulator_t std = 0;
static time_accel_t count = 0;
static int16_t threshold_int = 0;
static int16_t threshold_frac = 6;

void initDetectionStage(ring_buffer_t *pInBuff, ring_buffer_t *peakBufIn, void (*pNextStage)(void))
{
    inBuff = pInBuff;
    outBuff = peakBufIn;
    nextStage = pNextStage;
    rawMagnitudeMean = 0;
    mean = 0;
    std = 0;

#ifdef DUMP_FILE
    detectionFile = fopen(DUMP_DETECTION_FILE_NAME, "w+");
#endif
}

void detectionStage(void)
{
    if (!ring_buffer_is_empty(inBuff))
    {
        accumulator_t oMean = mean;
        data_point_t dataPoint;
        ring_buffer_dequeue(inBuff, &dataPoint);
        count++;
        if (count == 1)
        {
            mean = dataPoint.magnitude;
            std = 0;
            lastDataPoint = dataPoint;
            rawMagnitudeMean = (float)dataPoint.orig_magnitude;
        }
        else if (count == 2)
        {
            mean = (mean + dataPoint.magnitude) / 2;
            rawMagnitudeMean = (float) (rawMagnitudeMean + (float)dataPoint.orig_magnitude) / 2.0;
            std = sqrt(((dataPoint.magnitude - mean) * (dataPoint.magnitude - mean)) + ((oMean - mean) * (oMean - mean))) / 2;
        }
        else
        {
            mean = (dataPoint.magnitude + ((count - 1) * mean)) / count;
            rawMagnitudeMean = (float)(dataPoint.orig_magnitude + (float)((count - 1) * rawMagnitudeMean)) / (float)count;
            accumulator_t part1 = ((std * std) / (count - 1)) * (count - 2);
            accumulator_t part2 = ((oMean - mean) * (oMean - mean));
            accumulator_t part3 = ((dataPoint.magnitude - mean) * (dataPoint.magnitude - mean)) / count;
            std = (accumulator_t)sqrt(part1 + part2 + part3);
        }
        if (count > 15)
        {
            if ((dataPoint.magnitude - mean) > (std * threshold_int + (std / threshold_frac)))
            {
                // This is a peak
                ring_buffer_queue(outBuff, dataPoint);

                /* Peak time interval */
                dataPoint.peak_time = dataPoint.time - lastDataPoint.time;

                if (lastDataPoint.time == 0)
                    dataPoint.peak_time = 0;

                /* Compute MET constant */
                if (dataPoint.magnitude < 200) {
                    dataPoint.met = 1;
                } else if (dataPoint.magnitude < 500) {
                    dataPoint.met = 2;
                } else if (dataPoint.magnitude < 800) {
                    dataPoint.met = 5;
                } else if (dataPoint.magnitude < 1000) {
                    dataPoint.met = 10;
                } else if (dataPoint.magnitude < 1500) {
                    dataPoint.met = 13;
                } else if (dataPoint.magnitude < 2000) {
                    dataPoint.met = 15;
                } else if (dataPoint.magnitude < 2500) {
                    dataPoint.met = 17;
                } else if (dataPoint.magnitude > 2500) {
                    dataPoint.met = 23;
                }

                /* Increase calories burned */
                kcalories += (bmr * dataPoint.met * dataPoint.peak_time);

#ifdef DUMP_FILE
                if (detectionFile)
                {
                    if (!fprintf(detectionFile, "%lld, %lld, %lld, %lld, %f, %lld, %0.12f, %f\n",
                         dataPoint.time, dataPoint.magnitude, dataPoint.orig_magnitude, dataPoint.met, bmr, dataPoint.peak_time, kcalories, rawMagnitudeMean))
                         puts("error writing file");
                    // if (!fprintf(detectionFile, "mean=%lld, std=%lld, threshold_int=%lld threshold_frac=%lld\n",
                    //     mean, std, threshold_int, threshold_frac))
                    //     puts("error writing file");
                    fflush(detectionFile);
                }
#endif
                (*nextStage)();

                lastDataPoint = dataPoint;
            }
        }
    }
}

void resetDetection(void)
{
    std = 0;
    mean = 0;
    count = 0;
    rawMagnitudeMean = 0;
}

void changeDetectionThreshold(int16_t whole, int16_t frac)
{
    threshold_int = whole;
    threshold_frac = frac;
}

magnitude_t getMagAvg(void) {
    return rawMagnitudeMean;
}
