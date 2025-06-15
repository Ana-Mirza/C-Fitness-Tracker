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
#include "scoringStage.h"
#include "detectionStage.h"

#ifdef DUMP_FILE
#include <stdio.h>
static FILE
    *scoringFile;
static FILE *scoringFile;
#endif

// Returns true if adding overflows
int will_overflow(int32_t a, int32_t b) {
    return ((b > 0) && (a > INT32_MAX - b)) || 
           ((b < 0) && (a < INT32_MIN - b));
}

// Safe addition
int32_t safe_add(int64_t a, int64_t b) {
    if (will_overflow(a, b)) {
        return (b > 0) ? INT32_MAX : INT32_MIN;
    }
    return a + b;
}

static ring_buffer_t *inBuff;
static ring_buffer_t *outBuff;
static void (*nextStage)(void);

static ring_buffer_size_t windowSize = 10;
static ring_buffer_size_t midpoint = 5; //half of size

void initScoringStage(ring_buffer_t *pInBuff, ring_buffer_t *pOutBuff, void (*pNextStage)(void))
{
    inBuff = pInBuff;
    outBuff = pOutBuff;
    nextStage = pNextStage;

#ifdef DUMP_FILE
    scoringFile = fopen(DUMP_SCORING_FILE_NAME, "w+");
#endif
}

void scoringStage(void)
{
    if (ring_buffer_num_items(inBuff) == windowSize)
    {
        magnitude_t diffLeft = 0;
        magnitude_t diffRight = 0;
        data_point_t midpointData;
        ring_buffer_peek(inBuff, &midpointData, midpoint);
        data_point_t dataPoint;
        for (ring_buffer_size_t i = 0; i < midpoint; i++)
        {
            ring_buffer_peek(inBuff, &dataPoint, i);
            uint32_t diff = midpointData.magnitude - dataPoint.magnitude;
            diffLeft = safe_add(diffLeft, diff);
        }
        for (ring_buffer_size_t j = midpoint + 1; j < windowSize; j++)
        {
            ring_buffer_peek(inBuff, &dataPoint, j);
            uint32_t diff = midpointData.magnitude - dataPoint.magnitude;
            diffRight = safe_add(diffRight, diff);
        }
        magnitude_t scorePeak = safe_add(diffLeft, diffRight) / (windowSize - 1);
        data_point_t out;
        out.time = midpointData.time;
        out.magnitude = scorePeak;
        out.orig_magnitude = midpointData.orig_magnitude;
        ring_buffer_queue(outBuff, out);
        ring_buffer_dequeue(inBuff, &midpointData);
        (*nextStage)();

#ifdef DUMP_FILE
        if (scoringFile)
        {
            if (!fprintf(scoringFile, "%lld, %lld, %lld, %lld\n", out.time, out.magnitude, midpointData.magnitude, out.orig_magnitude))
                puts("error writing file");
            fflush(scoringFile);
        }
#endif
    }
}

void changeWindowSize(ring_buffer_size_t windowsize)
{
    windowSize = windowsize;
    midpoint = windowsize / 2;
}