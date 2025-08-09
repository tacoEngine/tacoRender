// tacoRender (c) Nikolas Wipper 2025

/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include "tr_timing.h"

#include <external/glad.h>

void InitTimer(Timer *timer) {
    glGenQueries(1, &timer->query);
}

void StartTimer(Timer *timer) {
    glBeginQuery(GL_TIME_ELAPSED, timer->query);
}

void StopTimer(Timer *timer) {
    glEndQuery(GL_TIME_ELAPSED);
}

float GetTimer(Timer *timer) {
    GLuint64 elapsed;
    glGetQueryObjectui64v(timer->query, GL_QUERY_RESULT, &elapsed);
    return (float) elapsed / 1e6;
}

void UninitTimer(Timer *timer) {
    glDeleteQueries(1, &timer->query);
    timer->query = 0;
}
