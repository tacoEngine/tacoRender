#version 330 core

in vec2 fragTexCoord;

out vec4 finalColor;

uniform sampler2D normalMap;
uniform sampler2D depth;
uniform vec3 camPos;
uniform mat4 invView;
uniform mat4 invProj;
uniform mat4 camView;
uniform mat4 camProj;
uniform vec2 screenSize;

uniform float sectorCount = 4.0;
uniform float sampleCount = 4.0;
uniform float radius = 128.0;
uniform float thickness = 0.5;

const float PI = 3.14159265359;
const float PI_2 = PI * 2.0;
const float HALF_PI = PI * 0.5;

// Biggest number less than 1 (one) representable with IEEE754 (or really close to it)
const float maxDist = 0.999999940395355224609375;

vec3 WorldPosFromDepth(float depth) {
    float z = depth * 2.0 - 1.0;
    vec4 clipSpacePosition = vec4(fragTexCoord * 2.0 - 1.0, z, 1.0);
    vec4 viewSpacePosition = invProj * clipSpacePosition;
    viewSpacePosition /= viewSpacePosition.w;
    vec4 worldSpacePosition = invView * viewSpacePosition;
    return worldSpacePosition.xyz;
}

vec3 ViewPosFromWorld(vec3 wpos) {
    vec4 clipSpacePosition = vec4(wpos * 2.0 - 1.0, 1.0);
    vec4 viewSpacePosition = invProj * clipSpacePosition;
    viewSpacePosition /= viewSpacePosition.w;
    return viewSpacePosition.xyz;
}

float distSquared(vec2 A, vec2 B) {
    vec2 C = A - B;
    return dot(C, C);
}

// Gold Noise ©2015 dcerisano@standard3d.com
// - based on the Golden Ratio
// - uniform normalized distribution
// - fastest static noise generator function (also runs at low precision)
// - use with indicated fractional seeding method.

float PHI = 1.61803398874989484820459;  // Φ = Golden Ratio

float gold_noise(in vec2 xy, in float seed) {
    return fract(tan(distSquared(xy * PHI, xy) * seed) * xy.x);
}

vec3 random2t3(vec2 seed, uint d) {
    seed = mod(seed, 9);
    return vec3(gold_noise(seed, float(d)), gold_noise(seed, float(d + 1u)), gold_noise(seed, float(d + 2u)));
}

// Much of the below code is based in this shader toy: https://www.shadertoy.com/view/4cdfzf

uint CountBits(uint value) {
    value = value - ((value >> 1u) & 0x55555555u);
    value = (value & 0x33333333u) + ((value >> 2u) & 0x33333333u);
    return ((value + (value >> 4u) & 0xF0F0F0Fu) * 0x1010101u) >> 24u;
}

uint HorizonLoop(vec3 position, vec3 viewDir, vec2 uv, vec2 dir, vec2 jitter, uint globalOccludedBitfield, float samplingDirection, float sinN) {
    vec2 rayDir = dir.xy * samplingDirection;

    float s = pow(radius, 1.0 / sampleCount);

    float t = pow(s, jitter.x);

    for (int i = 0; i < sampleCount; i++) {
        vec2 samplePos = uv + rayDir * t;
        t *= s;

        samplePos /= screenSize;
        // handle oob
        if (samplePos.x < 0.0 || samplePos.x >= 1.0 || samplePos.y < 0.0 || samplePos.y >= 1.0) break;

        float sampleDepth = texture(depth, samplePos).r;

        vec3 samplePosVS = ViewPosFromWorld(vec3(samplePos, sampleDepth));

        vec3 deltaPosFront = samplePosVS - position;
        vec3 deltaPosBack = deltaPosFront - viewDir * thickness;

        // project samples onto unit circle and compute cos(angles) relative to viewDir
        vec2 horCos = vec2(dot(normalize(deltaPosFront), viewDir), dot(normalize(deltaPosBack), viewDir));

        // sampling direction flips min/max cos(angles)
        horCos = samplingDirection >= 0.0 ? horCos.xy : horCos.yx;

        // map to slice relative distribution
        float d05 = samplingDirection * 0.5;

        vec2 hor01 = ((0.5 + 0.5 * sinN) + d05) - d05 * horCos;

        // jitter sample locations + clamp01
        hor01 = clamp(hor01 + jitter.y * (1.0 / 32.0), 0.0, 1.0);

        // turn arc into bit mask
        uvec2 horInt = uvec2(floor(hor01 * 32.0));

        uint OxFFFFFFFFu = 0xFFFFFFFFu;

        uint mX = horInt.x < 32u ? OxFFFFFFFFu << horInt.x : 0u;
        uint mY = horInt.y != 0u ? OxFFFFFFFFu >> (32u - horInt.y) : 0u;

        uint occBits0 = mX & mY;

        globalOccludedBitfield = globalOccludedBitfield | occBits0;
    }

    return globalOccludedBitfield;
}

void main() {
    float dist = texture(depth, fragTexCoord).r;
    if (dist >= maxDist) discard;

    vec3 wpos = WorldPosFromDepth(dist);
    vec3 normal = normalize(texture(normalMap, fragTexCoord).rgb);

    // Move position slightly in the normal direction to avoid self shadowing
    vec3 position = wpos + normal * (1.0 / 256.0);

    position = (camView * vec4(position, 1)).xyz;
    normal = normalize((mat3(camView) * normal));

    float occlusion = 0.0;
    vec2 uv0 = fragTexCoord * screenSize;
    vec3 viewDir = normalize(-position);

    for (int slice = 0; slice < sectorCount; slice++) {
        vec3 rnd01 = random2t3(uv0, uint(slice));
        vec2 dir = vec2(cos(rnd01.x * PI), sin(rnd01.x * PI));

        vec3 sliceN = cross(viewDir, vec3(dir, 0.0));

        vec3 projNormal = normal - sliceN * dot(normal, sliceN);

        float projNSqrLen = dot(projNormal, projNormal);
        if (projNSqrLen == 0.0) {
            occlusion = sectorCount;
            break;
        }

        vec3 T = cross(sliceN, projNormal);

        float projNRcpLen = inversesqrt(projNSqrLen);

        float cosN = dot(projNormal, viewDir) * projNRcpLen;
        float sinN = dot(T, viewDir) * projNRcpLen;

        uint globalOccludedBitfield = 0u;
        globalOccludedBitfield = HorizonLoop(position, viewDir, uv0, dir, rnd01.yz, globalOccludedBitfield, 1, sinN);
        globalOccludedBitfield = HorizonLoop(position, viewDir, uv0, dir, rnd01.yz, globalOccludedBitfield, -1, sinN);
        occlusion += 1.0 - float(CountBits(globalOccludedBitfield)) / 32.0;
    }

    occlusion /= sectorCount;

    finalColor = vec4(occlusion, 0, 0, dist);
}
