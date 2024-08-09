#version 430 core

#define amb 0.3

uniform vec3 u_color;

uniform vec3 u_camLight;

// pretend there's an area light straight up

in vec3 normal;
in vec3 worldPos;

out vec4 fragColor;

void main() {
	vec3 lightColor = vec3(1.0, 1.0, 1.0); // 白色光源
    vec3 objectColor = u_color;

    vec3 ambient = 0.3 * lightColor; // 环境光
    vec3 norm = normalize(normal);
    vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0)); // 光源方向
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    vec3 result = (ambient + diffuse) * objectColor;

    fragColor = vec4(result, 1.0);
}
