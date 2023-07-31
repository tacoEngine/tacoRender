// rl3d (c) Nikolas Wipper 2023

#include "rl3d_gl.h"

#include <rlgl.h>

void DrawScreenQuad() {
    rlBegin(RL_QUADS);

    rlColor4ub(1, 1, 1, 1);
    rlNormal3f(0.0f, 0.0f, 1.0f);                          // Normal vector pointing towards viewer

    // Top-left corner for texture and quad
    rlTexCoord2f(0, 0);
    rlVertex2i(0, 0);

    // Bottom-left corner for texture and quad
    rlTexCoord2f(0, 1);
    rlVertex2i(0, GetScreenHeight());

    // Bottom-right corner for texture and quad
    rlTexCoord2f(1, 1);
    rlVertex2i(GetScreenWidth(), GetScreenHeight());

    // Top-right corner for texture and quad
    rlTexCoord2f(1, 0);
    rlVertex2i(GetScreenWidth(), 0);

    rlEnd();
    rlSetTexture(0);
}
