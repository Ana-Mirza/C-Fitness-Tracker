/* 
The MIT License (MIT)

Copyright (c) 2020 Anna Brondin and Marcus Nordström and Dario Salvi

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
#include "motionDetectStage.h"
#include "StepCountingAlgo.h"

#define issigned(t) (((t)(-1)) < ((t)0))

#define umaxof(t) (((0x1ULL << ((sizeof(t) * 8ULL) - 1ULL)) - 1ULL) | \
                   (0xFULL << ((sizeof(t) * 8ULL) - 4ULL)))

#define smaxof(t) (((0x1ULL << ((sizeof(t) * 8ULL) - 1ULL)) - 1ULL) | \
                   (0x7ULL << ((sizeof(t) * 8ULL) - 4ULL)))

#define maxof(t) ((unsigned long long)(issigned(t) ? smaxof(t) : umaxof(t)))

static ring_buffer_t *inBuff;
static ring_buffer_t *outBuff;
static void (*nextStage)(void);
static int motionThreshold = 150;

void initMotionDetectStage(ring_buffer_t *pInBuff, ring_buffer_t *pOutBuff, void (*pNextStage)(void))
{
    inBuff = pInBuff;
    outBuff = pOutBuff;
    nextStage = pNextStage;
}

void changeMotionThreshold(int16_t threshold)
{
    motionThreshold = threshold;
}

void motionDetectStage(void)
{
    if (ring_buffer_num_items(inBuff) >= 15)
    {
        magnitude_t min = maxof(magnitude_t);
        magnitude_t max = 0;

        data_point_t dp;
        data_point_t prev_dp;
        for (int i = 0; i < 12; i++)
        {
            ring_buffer_peek(inBuff, &dp, i);
            if (dp.magnitude > max)
                max = dp.magnitude;
            if (dp.magnitude < min)
                min = dp.magnitude;
        }

        if (max - min > motionThreshold)
        {
            data_point_t dataPoint;
            ring_buffer_dequeue(inBuff, &dataPoint);
            ring_buffer_queue(outBuff, dataPoint);
            (*nextStage)();
        } else {
            ring_buffer_peek(inBuff, &dp, 1);
            ring_buffer_peek(inBuff, &prev_dp, 0);

            /* Add bmr calorie usage when there is no motion */
            if (ring_buffer_is_full(inBuff)) {
                float motionlessTime = dp.time - prev_dp.time;
                kcalories += bmr * motionlessTime; /* bmr per ms */
            }
        }
    }
}
