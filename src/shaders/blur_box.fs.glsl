#version 330 core

in vec2 fragTexCoord;

out float finalColor;

uniform sampler2D image;

#define RANGE 4

void main() {
    vec2 texelSize = 1.0 / vec2(textureSize(image, 0));
    float result = 0.0;
    for (int x = -RANGE; x <= RANGE; ++x)
    {
        for (int y = -RANGE; y <= RANGE; ++y)
        {
            vec2 offset = vec2(float(x), float(y)) * texelSize;
            result += texture(image, fragTexCoord + offset).r;
        }
    }
    finalColor = result / ((RANGE * 2 + 1) * (RANGE * 2 + 1));
}
