#version 330 core
in vec2 fragTexCoord;
out vec4 fragColor;
uniform sampler2D texture0;   // original scene
uniform sampler2D bloomTex;   // blurred bloom
uniform float bloomStrength;  // e.g. 1.6

void main() {
    vec4 scene = texture(texture0, fragTexCoord);
    vec4 bloom  = texture(bloomTex,  fragTexCoord);
    vec3 base        = scene.rgb;
    vec3 bloomContrib = bloom.rgb * bloomStrength;
    // 8% screen-blend layer on top of additive contribution
    vec3 bloomScreen  = 1.0 - (1.0 - bloomContrib) * (1.0 - base * 0.08);
    fragColor.rgb = base + bloomContrib + bloomScreen * 0.08;
    // Reinhard tone-map to prevent overflow
    fragColor.rgb = fragColor.rgb / (fragColor.rgb + vec3(1.0));
    fragColor.a = 1.0;
}
