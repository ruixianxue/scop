#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>

// Tell GLFW not to include any OpenGL header; we include them ourselves
#define GLFW_INCLUDE_NONE
// Ask the Linux OpenGL headers to declare modern OpenGL functions too
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>
#include <GLFW/glfw3.h>

// ============================================================
//  Helper functions
// ============================================================

// Read a whole text file into a string.
// Returns true on success, false if the file can't be opened.
bool readFile(const std::string& path, std::string& out)
{
    std::ifstream file(path);
    if (!file.is_open())
    {
        printf("Error: cannot open file '%s'\n", path.c_str());
        return false;
    }
    std::stringstream buffer;
    buffer << file.rdbuf();      // copy the entire file content into the buffer
    out = buffer.str();
    return true;
}

// Compile one shader. Returns its id, or 0 on failure.
GLuint compileShader(GLenum type, const char* source)
{
    GLuint shader = glCreateShader(type);        // create an empty shader object
    glShaderSource(shader, 1, &source, NULL);    // give it the source code
    glCompileShader(shader);                     // ask the GPU driver to compile it

    // Check whether compilation succeeded
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        char log[1024];
        glGetShaderInfoLog(shader, sizeof(log), NULL, log);
        printf("Error: %s shader compilation failed:\n%s\n",
               type == GL_VERTEX_SHADER ? "vertex" : "fragment", log);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

// Link a vertex shader and a fragment shader into one GPU program.
// Returns the program id, or 0 on failure.
GLuint createShaderProgram(const char* vertexSrc, const char* fragmentSrc)
{
    GLuint vs = compileShader(GL_VERTEX_SHADER, vertexSrc);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fragmentSrc);
    if (!vs || !fs)
    {
        if (vs) glDeleteShader(vs);
        if (fs) glDeleteShader(fs);
        return 0;
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);                      // connect the two shaders together

    // Check whether linking succeeded
    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success)
    {
        char log[1024];
        glGetProgramInfoLog(program, sizeof(log), NULL, log);
        printf("Error: shader program linking failed:\n%s\n", log);
        glDeleteProgram(program);
        program = 0;
    }

    // The individual shaders are no longer needed once linked into the program
    glDeleteShader(vs);
    glDeleteShader(fs);
    return program;
}

// Read two shader files from disk and build a shader program from them.
// Returns the program id, or 0 on failure.
GLuint loadShaderProgram(const std::string& vertexPath, const std::string& fragmentPath)
{
    std::string vertexSrc;
    std::string fragmentSrc;
    if (!readFile(vertexPath, vertexSrc) || !readFile(fragmentPath, fragmentSrc))
        return 0;
    return createShaderProgram(vertexSrc.c_str(), fragmentSrc.c_str());
}

// ============================================================
//  Main
// ============================================================

int main()
{
    // 1. Initialize GLFW
    if (!glfwInit())
    {
        printf("Error: failed to initialize GLFW\n");
        return 1;
    }

    // 2. Request an OpenGL 3.3 Core Profile context
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // 3. Create an 800x600 window titled "scop"
    GLFWwindow* window = glfwCreateWindow(800, 600, "scop", NULL, NULL);
    if (!window)
    {
        printf("Error: failed to create window\n");
        glfwTerminate();
        return 1;
    }

    // 4. Make this window's context current
    glfwMakeContextCurrent(window);

    printf("OpenGL version: %s\n", (const char*)glGetString(GL_VERSION));
    printf("Renderer:       %s\n", (const char*)glGetString(GL_RENDERER));

    // Load the shader program from files (paths are relative to where you run ./scop)
    GLuint shaderProgram = loadShaderProgram("shaders/vertex.glsl", "shaders/fragment.glsl");
    if (!shaderProgram)
    {
        glfwTerminate();
        return 1;
    }
    printf("Shader program created (id = %u)\n", shaderProgram);

    // ------------------------------------------------------------
    // Prepare square data and upload it to the GPU
    // ------------------------------------------------------------

    // 4 unique corners of the square, each = position (x, y) + color (r, g, b)
    // Screen coordinates go from -1 (left/bottom) to +1 (right/top)
    float vertices[] = {
    //    x      y       r     g     b
        -0.5f, -0.5f,   1.0f, 0.0f, 0.0f,   // 0: bottom-left  (red)
         0.5f, -0.5f,   0.0f, 1.0f, 0.0f,   // 1: bottom-right (green)
         0.5f,  0.5f,   0.0f, 0.0f, 1.0f,   // 2: top-right    (blue)
        -0.5f,  0.5f,   1.0f, 1.0f, 0.0f,   // 3: top-left     (yellow)
    };

    // Which corners make up each triangle
    //   3 ---- 2
    //   |    / |
    //   |  /   |
    //   0 ---- 1
    unsigned int indices[] = {
        0, 1, 2,    // triangle 1: bottom-left, bottom-right, top-right
        0, 2, 3,    // triangle 2: bottom-left, top-right, top-left
    };

    // VAO: the "instruction sheet" describing how to read the data
    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);                 // start recording into this VAO

    // VBO: the "box" that holds the vertex data in GPU memory
    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    // Copy our vertices from CPU memory into GPU memory
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // EBO: the "index list" telling the GPU which vertices form each triangle.
    // Must be bound while the VAO is bound, so the VAO remembers it.
    GLuint ebo;
    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Describe attribute 0 (aPos): 2 floats, starting at offset 0
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE,
                          5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Describe attribute 1 (aColor): 3 floats, starting after the 2 position floats
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
                          5 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);                   // stop recording (VAO now remembers VBO layout + EBO)

    // 5. Main loop
    while (!glfwWindowShouldClose(window))
    {
        // Close the window when ESC is pressed
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, 1);

        // Make the drawing area match the real window size
        // (handles window resizing and display scaling)
        int fbWidth, fbHeight;
        glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
        glViewport(0, 0, fbWidth, fbHeight);

        // Clear the screen with a background color
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Draw the square
        glUseProgram(shaderProgram);        // use our GPU program
        glBindVertexArray(vao);             // use our instruction sheet + data + index list
        // Draw using the index list: 6 indices = 2 triangles
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        // Show the frame and process events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // 6. Clean up
    glDeleteBuffers(1, &ebo);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(shaderProgram);
    glfwTerminate();
    return 0;
}