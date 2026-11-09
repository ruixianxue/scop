#version 330 core

// Fragment shader: runs once PER PIXEL covered by a triangle.
// Its job: decide the color of this pixel.

in vec3 vColor;         // input: color interpolated from the triangle's vertices
out vec4 FragColor;     // output: final pixel color (r, g, b, alpha)

void main()
{
    FragColor = vec4(vColor, 1.0);
}
