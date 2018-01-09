#version 450

uniform sampler2D tex;
in vec2 texCoord;
in vec3 normal;
out vec4 frag;

void main() {
	frag = texture(tex, texCoord) + normal.x * 0.01;
}
