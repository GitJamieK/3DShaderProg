#include "shader.h"
#include <iostream>
#include <vector>

// ------------------------------------------------------------
// GLuint (int for opengl)
// ------------------------------------------------------------
static GLuint Compile(GLenum type, const char* src) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint ok = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        GLint len = 0; glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
        std::vector<char> log(len);
        glGetShaderInfoLog(shader, len, nullptr, log.data());

        std::cerr << (type == GL_VERTEX_SHADER ? "Vertex" : "Fragment")
            << " shader compile error:\n" << log.data() << "\n";

        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

static GLuint Link(GLuint vs, GLuint fs) {
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);

    GLint ok = GL_FALSE;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        GLint len = 0; glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &len);
        std::vector<char> log(len);
        glGetProgramInfoLog(prog, len, nullptr, log.data());

        std::cerr << "Program link error:\n" << log.data() << "\n";

        glDeleteProgram(prog);
        return 0;
    }
    return prog;
}

// ------------------------------------------------------------
// Shaders
// draw TWO triangles (A-B-M) and (A-M-C) to form one big triangle
// a proper specular hotspot while the crease remains sharp.
// ------------------------------------------------------------

static const char* vertexShaderSource = R"GLSL(
#version 330 core

const float h = 0.8660254; // sqrt(3)/2

// Triangle vertices
const vec3 A = vec3( 0.0,  2.0*h/3.0, 0.0);
const vec3 B = vec3(-0.5, -h/3.0,     0.0);
const vec3 C = vec3( 0.5, -h/3.0,     0.0);
const vec3 M = vec3( 0.0, -h/3.0,     0.0);

//left face (A,B,M) then right face (A,M,C)
const vec3 positions[6] = vec3[](
    A, B, M,
    A, M, C
);

const vec3 normals[6] = vec3[](
    normalize(vec3(-0.25,  0.70, 1.0)), // A (left face)
    normalize(vec3(-0.80, -0.10, 1.0)), // B (left face)
    normalize(vec3(-0.40, -0.20, 1.0)), // M (left face)

    normalize(vec3( 0.25,  0.70, 1.0)), // A (right face)
    normalize(vec3( 0.40, -0.20, 1.0)), // M (right face)
    normalize(vec3( 0.80, -0.10, 1.0))  // C (right face)
);

out vec3 vNormal;

void main() {
    gl_Position = vec4(positions[gl_VertexID], 1.0);
    vNormal     = normals[gl_VertexID]; // will interpolate within each face
}
)GLSL";

static const char* fragmentShaderSource = R"GLSL(
#version 330 core
in  vec3 vNormal;
out vec4 FragColor;

// Blinn-Phong
void main() {

    vec3 N = normalize(vNormal);

    // Light from above-left-front
    vec3 L = normalize(vec3(-0.5, 0.8, 0.6));
    vec3 V = normalize(vec3( 0.0, 0.0, 1.0));

    //base color
    vec3 base = vec3(0.85);

    // Ambient (constant)
    vec3 ambient = 0.12 * base;

    // Diffuse (Lambert)
    float NdotL = max(dot(N, L), 0.0);
    vec3 diffuse = 0.88 * base * NdotL;

    //Specular (Blinn)
    vec3  H = normalize(L + V);
    float NdotH = max(dot(N, H), 0.0);
    float shininess = 96.0;                 //  hotspot
    vec3  specular  = vec3(0.9) * pow(NdotH, shininess);

    FragColor = vec4(ambient + diffuse + specular, 1.0);
}
)GLSL";

// create, destroy
GLuint CreateTriangle() {
    GLuint vs = Compile(GL_VERTEX_SHADER, vertexShaderSource);
    if (!vs) return 0;
    GLuint fs = Compile(GL_FRAGMENT_SHADER, fragmentShaderSource);
    if (!fs) { glDeleteShader(vs); return 0; }

    GLuint prog = Link(vs, fs);
    glDeleteShader(vs);
    glDeleteShader(fs);
    return prog;
}

void DestroyTriangle(GLuint program) {
    if (program) glDeleteProgram(program);
}