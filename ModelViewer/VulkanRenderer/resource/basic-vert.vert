#version 450

layout(push_constant) uniform PushConstants {
    layout(offset=0) mat4 modelMatrix;
    layout(offset=64) mat4 normalMatrix;
} pushConstants;

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 viewProj;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inColor;
layout(location = 3) in vec2 inTexCoord;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec2 fragTexCoord;

void main() {
    gl_Position = ubo.viewProj * pushConstants.modelMatrix * vec4(inPosition, 1.0);
    fragColor = inColor;
    fragNormal = normalize(pushConstants.normalMatrix * vec4(inNormal, 0.0)).xyz;
    fragTexCoord = inTexCoord;
}
