#version 430



layout(std430, binding = 0) buffer _Pos {
    vec4 Pos[];
};

out vec3 worldPos0;

void main() {
	vec3 position = Pos[gl_VertexID].xyz;
    gl_Position = vec4(position, 1.0);
    worldPos0 = position;
}
