// tacoRender (c) Nikolas Wipper 2025

/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#ifndef TR_MATH_H
#define TR_MATH_H

#include <raylib.h>

#ifdef __cplusplus
extern "C" {



#endif

typedef struct Plane {
    Vector3 normal;
    float constant;
} Plane;

typedef struct Frustum {
    Plane planes[6];
} Frustum;

BoundingBox TransformAABB(BoundingBox box, Matrix mvp);
bool IsAABBInFrustum(Frustum frustum, BoundingBox box);
Frustum CreateFrustumFromCamera(Camera cam,
                                float aspect,
                                float fovY,
                                float zNear,
                                float zFar);

#ifdef __cplusplus
}
#endif

#endif //TR_MATH_H
