#version 330 core
out vec4 FragColor;

in vec4 vertex_color;
in vec2 tex_coord;

uniform vec4 our_color;
uniform sampler2D our_texture;

void main() {
   FragColor.rgb = texture(our_texture, tex_coord).rgb;
   FragColor.a = vertex_color.a;
}
