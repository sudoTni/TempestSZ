#version 330 core
in vec2 fragTexCoord;
out vec4 fragColor;
uniform sampler2D texture0;
uniform float threshold;   // e.g. 0.55

void main() {
    vec4 col = texture(texture0, fragTexCoord);
    float brightness = dot(col.rgb, vec3(0.2126, 0.7152, 0.0722));
    fragColor = (brightness > threshold) ? col : vec4(0.0);
}
