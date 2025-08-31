// tacoRender (c) Nikolas Wipper 2025

/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include "tr_math.h"

#include <raymath.h>

inline bool Inside(float x, float min, float max) {
    return (x >= min && x <= max);
}

inline bool Vector3Inside(Vector3 x, BoundingBox box) {
    return Inside(x.x, box.min.x, box.max.x) &&
           Inside(x.y, box.min.y, box.max.y) &&
           Inside(x.z, box.min.z, box.max.z);
}

// From https://www.realtimerendering.com/resources/GraphicsGems/gems/TransBox.c
BoundingBox TransformAABB(BoundingBox box, Matrix mvp) {
    Vector3 translate = (Vector3) {mvp.m12, mvp.m13, mvp.m14};
    float m[4][4] = {
        {mvp.m0, mvp.m1, mvp.m2, mvp.m3},
        {mvp.m4, mvp.m5, mvp.m6, mvp.m7},
        {mvp.m8, mvp.m9, mvp.m10, mvp.m11},
        {mvp.m12, mvp.m13, mvp.m14, mvp.m15},
    };
    BoundingBox result;

    float Amin[3], Amax[3];
    float Bmin[3], Bmax[3];

    // Copy box A into a min array and a max array for easy reference.
    Amin[0] = box.min.x;
    Amin[1] = box.min.y;
    Amin[2] = box.min.z;
    Amax[0] = box.max.x;
    Amax[1] = box.max.y;
    Amax[2] = box.max.z;

    // Take care of translation by beginning at T.
    Bmin[0] = Bmax[0] = translate.x;
    Bmin[1] = Bmax[1] = translate.y;
    Bmin[2] = Bmax[2] = translate.z;

    // Now find the extreme points by considering the product of the
    // min and max with each component of M.
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++) {
            float a = m[i][j] * Amin[j];
            float b = m[i][j] * Amax[j];
            Bmin[i] += fminf(a, b);
            Bmax[i] += fmaxf(a, b);
        }

    // Copy the result into the new box.
    result.min.x = Bmin[0];
    result.max.x = Bmax[0];
    result.min.y = Bmin[1];
    result.max.y = Bmax[1];
    result.min.z = Bmin[2];
    result.max.z = Bmax[2];

    return result;
}

Plane MakePlane(Vector3 a, Vector3 normal) {
    Vector3 n = Vector3Normalize(normal);
    return (Plane) {
        n,
        Vector3DotProduct(n, a) // use normalized normal
    };
}

Frustum CreateFrustumFromCamera(Camera cam,
                                float aspect,
                                float fovY,
                                float zNear,
                                float zFar) {
    const float halfVSide = zFar * tanf(DEG2RAD * fovY * .5f);
    const float halfHSide = halfVSide * aspect;
    Vector3 camFront = Vector3Normalize(Vector3Subtract(cam.target, cam.position));
    Vector3 camRight = Vector3Normalize(Vector3CrossProduct(camFront, cam.up));
    Vector3 camUp = Vector3Normalize(Vector3CrossProduct(camRight, camFront));
    Vector3 frontMultFar = Vector3Scale(camFront, zFar);

    return (Frustum) {
        {
            MakePlane(Vector3Add(cam.position, Vector3Scale(camFront, zNear)),
                      camFront),
            MakePlane(Vector3Add(cam.position, frontMultFar),
                      Vector3Negate(camFront)),
            MakePlane(cam.position,
                      Vector3CrossProduct(Vector3Subtract(frontMultFar, Vector3Scale(camRight, halfHSide)), camUp)),
            MakePlane(cam.position,
                      Vector3CrossProduct(camUp, Vector3Add(frontMultFar, Vector3Scale(camRight, halfHSide)))),
            MakePlane(cam.position,
                      Vector3CrossProduct(camRight, Vector3Subtract(frontMultFar, Vector3Scale(camUp, halfVSide)))),
            MakePlane(cam.position,
                      Vector3CrossProduct(Vector3Add(frontMultFar, Vector3Scale(camUp, halfVSide)), camRight)),
        }
    };
}

float Vector3DistanceToPlane(Vector3 point, Plane plane) {
    return Vector3DotProduct(plane.normal, point) - plane.constant;
}

bool IsAABBInFrustum(Frustum frustum, BoundingBox box) {
    for (int i = 0; i < 6; i++) {
        Vector3 extents = {
                    (box.max.x - box.min.x) / 2.f,
                    (box.max.y - box.min.y) / 2.f,
                    (box.max.z - box.min.z) / 2.f
                }, center = Vector3Add(box.min, extents);

        // Compute the projection interval radius of b onto L(t) = b.c + t * p.n
        const float r = extents.x * fabsf(frustum.planes[i].normal.x) +
                        extents.y * fabsf(frustum.planes[i].normal.y) +
                        extents.z * fabsf(frustum.planes[i].normal.z);

        if (-r > Vector3DistanceToPlane(center, frustum.planes[i]))
            return false;
    }
    return true;
}
