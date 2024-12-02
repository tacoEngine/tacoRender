#version 330 core

in vec2 fragTexCoord;

out float finalColor;

uniform sampler2D normalMap;
uniform sampler2D depth;
uniform mat4 invView;
uniform mat4 invProj;
uniform mat4 camView;
uniform mat4 camProj;

vec3 samples[64] = vec3[](
vec3(0.0261425, -0.0376851, 4.00204e-07),
vec3(-0.0517141, -0.0320865, 0.0304131),
vec3(-0.0108267, 0.040398, 0.0315651),
vec3(-0.0314388, -0.0327688, 0.0292523),
vec3(-0.00132874, -0.0056109, 0.00382464),
vec3(0.0297163, 0.062378, 0.0697156),
vec3(-0.0489338, 0.00322945, 0.0507385),
vec3(0.0689938, 0.0338294, 0.0349743),
vec3(0.0167713, -0.0321483, 0.00932237),
vec3(0.0331278, 0.0172987, 0.0213404),
vec3(0.0374994, 0.0812687, 0.0208025),
vec3(0.0266481, -0.0865055, 0.0659467),
vec3(0.0547641, -0.0130673, 0.0280206),
vec3(-0.0110478, -0.01767, 0.00933272),
vec3(0.00571618, 0.00555481, 0.00339805),
vec3(0.001716, 0.000476389, 0.0476425),
vec3(-0.00607237, -0.000156404, 0.0128097),
vec3(6.96767e-05, -0.0420028, 0.0466964),
vec3(0.00540731, 0.0752215, 0.0251832),
vec3(0.0514841, -0.0885747, 0.0926251),
vec3(-0.0847474, -0.0655798, 0.0724548),
vec3(0.0660382, 0.0362147, 0.121362),
vec3(-0.0245497, 0.0357539, 0.0459818),
vec3(0.084097, 0.0122515, 0.161992),
vec3(-0.0471186, 0.069756, 0.0420852),
vec3(-0.0261632, -0.0039447, 0.0330319),
vec3(0.00775989, 0.0018387, 0.00197227),
vec3(0.167912, -0.000560453, 0.0985381),
vec3(0.0578085, 0.180922, 0.128394),
vec3(0.0217065, -0.0290399, 0.00807598),
vec3(-0.111954, -0.0507763, 0.0102483),
vec3(0.0672952, 0.0322446, 0.00206515),
vec3(0.187537, -0.0884211, 0.0438314),
vec3(-0.0661418, 0.105045, 0.0435408),
vec3(0.0592117, -0.239374, 0.169697),
vec3(0.00562892, 0.045657, 0.0295806),
vec3(-0.126569, -0.0322389, 0.173734),
vec3(-0.105291, -0.0336205, 0.369015),
vec3(-0.133294, -0.221413, 0.0464706),
vec3(0.207733, 0.25832, 0.108203),
vec3(-0.0605341, -0.0128894, 0.0677724),
vec3(-0.0119915, 0.0655307, 0.00231462),
vec3(-0.262367, -0.063802, 0.337014),
vec3(-0.144795, -0.114031, 0.0155575),
vec3(0.141442, -0.182272, 0.0632041),
vec3(0.437972, -0.0676212, 0.24775),
vec3(0.196835, -0.312091, 0.357106),
vec3(-0.0222231, 0.279056, 0.0967121),
vec3(0.141587, 0.175742, 0.166133),
vec3(-0.0631059, 0.138069, 0.494785),
vec3(0.479211, 0.198913, 0.33888),
vec3(0.0271384, -0.152772, 0.308639),
vec3(0.369594, -0.414181, 0.249154),
vec3(0.38887, 0.241946, 0.231701),
vec3(0.0066864, 0.00571323, 0.0110748),
vec3(-0.00410669, 0.0027978, 0.0496037),
vec3(0.228858, -0.015091, 0.486861),
vec3(0.470433, 0.535686, 0.128289),
vec3(-0.0906902, -0.659009, 0.49676),
vec3(-0.372504, -0.109116, 0.217853),
vec3(-0.0523188, -0.0971253, 0.710221),
vec3(0.116336, -0.0672402, 0.0425562),
vec3(-0.000688899, 0.000160498, 0.00043486),
vec3(-0.151823, 0.190876, 0.324663)
);

vec3 ViewPosFromDepth(float depth) {
    float z = depth * 2.0 - 1.0;
    vec4 clipSpacePosition = vec4(fragTexCoord * 2.0 - 1.0, z, 1.0);
    vec4 viewSpacePosition = invProj * clipSpacePosition;
    viewSpacePosition /= viewSpacePosition.w;
    return viewSpacePosition.xyz;
}

float random(vec3 seed, uint i){
    vec4 seed4 = vec4(seed, i);
    float dot_product = dot(seed4, vec4(12.9898, 78.233, 45.164, 94.673));
    return fract(sin(dot_product) * 43758.5453);
}

void main() {
    float dist = texture(depth, fragTexCoord).r;
    if (dist == 1) discard;
    vec3 viewPos = ViewPosFromDepth(dist);

    vec3 normal = normalize(texture(normalMap, fragTexCoord).rgb);

    vec3 randomVec = normalize(vec3(random(viewPos, 0u) * 2 + 1, random(viewPos, 1u) * 2 + 1, 0));

    // parameters (you'd probably want to use them as uniforms to more easily tweak the effect)
    const int kernelSize = 64;
    const float radius = 0.5;
    const float bias = 0.025;

    // create TBN change-of-basis matrix: from tangent-space to view-space
    vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN = mat3(camView) * mat3(tangent, bitangent, normal);
    // iterate over the sample kernel and calculate occlusion factor
    float occlusion = 0.0;
    for(int i = 0; i < kernelSize; ++i) {
        // get sample position
        vec3 samplePos = TBN * samples[i]; // from tangent to view-space
        samplePos = viewPos + samplePos * radius;

        // project sample position (to sample texture) (to get position on screen/texture)
        vec4 offset = vec4(samplePos, 1.0);
        offset = camProj * offset; // from view to clip-space
        offset.xyz /= offset.w; // perspective divide
        offset.xyz = offset.xyz * 0.5 + 0.5; // transform to range 0.0 - 1.0

        // get sample depth
        float sampleDepth = ViewPosFromDepth(texture(depth, offset.xy).r).z; // get depth value of kernel sample

        // range check & accumulate
        float rangeCheck = smoothstep(0.0, 1.0, radius / abs(viewPos.z - sampleDepth));
        occlusion += (sampleDepth >= samplePos.z + bias ? 1.0 : 0.0) * rangeCheck;
    }
    occlusion = 1.0 - (occlusion / kernelSize);

    finalColor = occlusion;
}
