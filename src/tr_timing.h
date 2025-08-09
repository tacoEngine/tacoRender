// tacoRender (c) Nikolas Wipper 2025

/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#ifndef TR_TIMING_H
#define TR_TIMING_H

#ifdef __cplusplus
extern "C" {
#endif

struct Timer;

void InitTimer(struct Timer *timer);
void StartTimer(struct Timer *timer);
void StopTimer(struct Timer *timer);
float GetTimer(struct Timer *timer);
void UninitTimer(struct Timer *timer);

typedef struct Timer {
    unsigned int query;

#ifdef __cplusplus
    void Start() {
        StartTimer(this);
    }

    void Stop() {
        StopTimer(this);
    }

    float Get() {
        return GetTimer(this);
    }
#endif
} Timer;

#ifdef __cplusplus
}
#endif

#endif //TR_TIMING_H
