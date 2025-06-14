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

#include "filterStage.h"
#include "scoringStage.h"

#ifdef DUMP_FILE
#include <stdio.h>
static FILE
    *filteredFile;
static FILE *filteredFile;
#endif

static ring_buffer_t *inBuff;
static ring_buffer_t *outBuff;
static void (*nextStage)(void);

/*

FIR filter designed with
http://t-filter.appspot.com

sampling frequency: 50 Hz

fixed point precision: 32 bits

* 0 Hz - 3 Hz
  gain = 1
  desired ripple = 5 dB
  actual ripple = n/a

* 4 Hz - 25 Hz
  gain = 0
  desired attenuation = -10 dB
  actual attenuation = n/a

*/

#define FILTER_TAP_NUM 17

static int filter_taps[FILTER_TAP_NUM] = {
  -281468814,
  106520334,
  127255214,
  162830244,
  205727178,
  248615023,
  284705529,
  308612275,
  316959956,
  308612275,
  284705529,
  248615023,
  205727178,
  162830244,
  127255214,
  106520334,
  -281468814
};


void initFilterStage(ring_buffer_t *pInBuff, ring_buffer_t *pOutBuff, void (*pNextStage)(void))
{
    inBuff = pInBuff;
    outBuff = pOutBuff;
    nextStage = pNextStage;

#ifdef DUMP_FILE
    filteredFile = fopen(DUMP_FILTERED_FILE_NAME, "w+");
#endif
}

void filterStage(void)
{
    if (ring_buffer_num_items(inBuff) == FILTER_TAP_NUM)
    {
        accumulator_t sum = 0;
        data_point_t dataPoint;
        data_point_t out;

        for (int8_t i = 0; i < FILTER_TAP_NUM; i++)
        {
            ring_buffer_peek(inBuff, &dataPoint, i);
            if (i == FILTER_TAP_NUM - 1)
                out.time = dataPoint.time;
            sum += dataPoint.magnitude * filter_taps[i];
        }
        out.magnitude = sum >> 16;
        out.orig_magnitude = dataPoint.orig_magnitude;

        ring_buffer_dequeue(inBuff, &dataPoint);
        ring_buffer_queue(outBuff, out);

#ifdef DUMP_FILE
        if (filteredFile)
        {
            if (!fprintf(filteredFile, "%ld, %ld, %ld\n", out.time, out.magnitude, out.orig_magnitude))
                puts("error writing file");
            fflush(filteredFile);
        }
#endif

        (*nextStage)();
    }
}
