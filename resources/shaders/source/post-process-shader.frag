#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 fragTexCoord;

layout(binding = 0) uniform sampler2D texSampler;

layout(location = 0) out vec4 outColor;

void main() {
    vec4 fragCol = texture(texSampler, fragTexCoord);
    float gamma = 2.2;
    outColor = vec4(pow(fragCol.rgb, vec3(1.0/gamma)), 1.0);
    //outColor = fragCol;;
}
