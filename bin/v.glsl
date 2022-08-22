#version 330 core
layout (location = 0) in vec3 a_pos;
layout (location = 1) in vec3 a_color;
layout (location = 2) in vec2 a_tex_coord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform vec2 tex_offset;
uniform float tex_alpha;

out vec4 vertex_color; // specify a color output to the fragment shader
out vec2 tex_coord;

void main() {
    gl_Position = projection * view * model * vec4(a_pos, 1.0);
    vertex_color = vec4(a_color, tex_alpha);
    tex_coord = a_tex_coord + tex_offset;
}
