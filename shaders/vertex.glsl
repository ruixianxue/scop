#version 330 core

// Vertex shader: runs once PER VERTEX.
// Its job: decide where this vertex appears on the screen.

layout(location = 0) in vec2 aPos;    // input: vertex position (x, y)
layout(location = 1) in vec3 aColor;  // input: vertex color (r, g, b)

out vec3 vColor;                      // output: passed on to the fragment shader

void main()
{
    vColor = aColor;
    // gl_Position is a built-in output: the final position of this vertex
    gl_Position = vec4(aPos, 0.0, 1.0);
}
