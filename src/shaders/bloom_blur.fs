#version 330 core
in vec2 fragTexCoord;
out vec4 fragColor;
uniform sampler2D texture0;
uniform vec2 texelSize;    // 1/width, 1/height of blur RT
uniform float offset;      // pass index: 0.5, 1.5, 2.5 ...

void main() {
    vec4 sum = vec4(0.0);
    sum += texture(texture0, fragTexCoord + vec2(-offset,  offset) * texelSize);
    sum += texture(texture0, fragTexCoord + vec2( offset,  offset) * texelSize);
    sum += texture(texture0, fragTexCoord + vec2(-offset, -offset) * texelSize);
    sum += texture(texture0, fragTexCoord + vec2( offset, -offset) * texelSize);
    fragColor = sum * 0.25;
}
