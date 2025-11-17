#include <emscripten.h>
#include <emscripten/html5.h>
#include <GLES2/gl2.h>
#include <string>
#include <vector>
#include <cstring>
#include <cmath>
#include <iostream>
#include <sstream>

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

GLuint program = 0, vbo = 0, ibo = 0;
int indexCount = 0;

// Transform state
float rotationX = 0, rotationY = 0;
float scaleFactor = 1.0f;
float transX = 0, transY = 0, transZ = -3;

bool dragging = false;
double lastX, lastY;

// ---------- Matrix helpers ----------
struct Mat4 { float m[16]; };
Mat4 I() { Mat4 r; memset(r.m,0,64); r.m[0]=r.m[5]=r.m[10]=r.m[15]=1; return r; }

Mat4 mul(const Mat4 &a, const Mat4 &b) {
    Mat4 r;
    for(int i=0;i<16;i++) r.m[i]=0;
    for(int r1=0;r1<4;r1++)
        for(int c=0;c<4;c++)
            for(int k=0;k<4;k++)
                r.m[c*4+r1] += a.m[k*4+r1] * b.m[c*4+k];
    return r;
}

Mat4 translate(Mat4 a, float x,float y,float z) {
    a.m[12]+=x; a.m[13]+=y; a.m[14]+=z;
    return a;
}

Mat4 scaleM(Mat4 a, float s) {
    a.m[0]*=s; a.m[5]*=s; a.m[10]*=s;
    return a;
}

Mat4 rotX(float a) {
    Mat4 r = I();
    float c = cosf(a), s = sinf(a);
    r.m[5]=c; r.m[6]=s;
    r.m[9]=-s; r.m[10]=c;
    return r;
}
Mat4 rotY(float a) {
    Mat4 r = I();
    float c = cosf(a), s = sinf(a);
    r.m[0]=c; r.m[2]=-s;
    r.m[8]=s; r.m[10]=c;
    return r;
}

Mat4 perspective(float fovy, float aspect, float zn, float zf) {
    float f = 1.0f / tanf(fovy*0.5f);
    Mat4 r; memset(r.m,0,64);
    r.m[0] = f/aspect;
    r.m[5] = f;
    r.m[10] = (zf+zn)/(zn-zf);
    r.m[11] = -1;
    r.m[14] = (2*zf*zn)/(zn-zf);
    return r;
}

// ---------- Shader ----------
const char* vs_src =
"attribute vec3 aPos;"
"attribute vec3 aNorm;"
"uniform mat4 uMVP;"
"uniform mat4 uModel;"
"varying vec3 vN;"
"void main(){"
"  vN = (uModel * vec4(aNorm,0.0)).xyz;"
"  gl_Position = uMVP * vec4(aPos,1.0);"
"}";

const char* fs_src =
"precision mediump float;"
"varying vec3 vN;"
"uniform vec3 uLight;"
"vec3 light2 = vec3(0.0, 1.0, 0.0);"
"void main(){"
"  float d = max(dot(normalize(vN), -uLight), 0.0);"
"  float d2 = max(dot(normalize(vN), -light2), 0.0);"
"  vec3 c = vec3(0.8,0.7,0.6)*(0.2 + d*0.6 + d2*0.3);"
"  gl_FragColor = vec4(c, 1.0);"
"}";

GLuint compile(GLenum t, const char* s){
    GLuint id = glCreateShader(t);
    glShaderSource(id,1,&s,0);
    glCompileShader(id);
    return id;
}

void makeProgram(){
    GLuint vs = compile(GL_VERTEX_SHADER, vs_src);
    GLuint fs = compile(GL_FRAGMENT_SHADER, fs_src);
    program = glCreateProgram();
    glAttachShader(program,vs);
    glAttachShader(program,fs);
    glBindAttribLocation(program,0,"aPos");
    glBindAttribLocation(program,1,"aNorm");
    glLinkProgram(program);
    glDeleteShader(vs);
    glDeleteShader(fs);
}

// ---------- Upload OBJ to GL ----------
void buildMesh(
    const tinyobj::attrib_t &attrib,
    const std::vector<tinyobj::shape_t> &shapes)
{
    struct V { float px,py,pz, nx,ny,nz; };
    std::vector<V> verts;
    std::vector<unsigned int> idx;

    for(auto &shape : shapes){
        for(auto &i : shape.mesh.indices){
            V v;
            v.px = attrib.vertices[3*i.vertex_index+0];
            v.py = attrib.vertices[3*i.vertex_index+1];
            v.pz = attrib.vertices[3*i.vertex_index+2];

            if(!attrib.normals.empty() && i.normal_index >= 0){
                v.nx = attrib.normals[3*i.normal_index+0];
                v.ny = attrib.normals[3*i.normal_index+1];
                v.nz = attrib.normals[3*i.normal_index+2];
            } else {
                v.nx = v.ny = 0; v.nz = 1;
            }
            idx.push_back(verts.size());
            verts.push_back(v);
        }
    }

    indexCount = idx.size();

    if(!vbo) glGenBuffers(1,&vbo);
    if(!ibo) glGenBuffers(1,&ibo);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, verts.size()*sizeof(V), verts.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, idx.size()*sizeof(unsigned int), idx.data(), GL_STATIC_DRAW);
}

// ---------- Load OBJ from memory (called from JS) ----------
extern "C" EMSCRIPTEN_KEEPALIVE
void LoadOBJFromMemory(const uint8_t* ptr, int len) {
    std::string text((const char*)ptr, len);
    std::istringstream ss(text);

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    bool ok = tinyobj::LoadObj(
        &attrib, &shapes, &materials,
        &warn, &err,
        &ss, nullptr
    );

    if(!ok){
        std::cout << "OBJ load error: " << err << "\n";
        return;
    }

    buildMesh(attrib, shapes);
}

// ---------- Callbacks ----------
EM_BOOL md(int, const EmscriptenMouseEvent* e, void*){ dragging=true; lastX=e->clientX; lastY=e->clientY; return 1; }
EM_BOOL mu(int, const EmscriptenMouseEvent*, void*){ dragging=false; return 1; }
EM_BOOL mm(int, const EmscriptenMouseEvent* e, void*){
    if(dragging){
        float dx = (e->clientX-lastX)*0.01f;
        float dy = (e->clientY-lastY)*0.01f;
        rotationY += dx; rotationX += dy;
        lastX=e->clientX; lastY=e->clientY;
    }
    return 1;
}
EM_BOOL wheel(int, const EmscriptenWheelEvent* e, void*) {
    scaleFactor *= (e->deltaY > 0 ? 0.95f : 1.05f);
    return 1;
}
EM_BOOL key(int, const EmscriptenKeyboardEvent* e, void*) {
    const char* k = e->key;
    if(!strcmp(k,"a")) transX -= 0.05f;
    if(!strcmp(k,"d")) transX += 0.05f;
    if(!strcmp(k,"w")) transY += 0.05f;
    if(!strcmp(k,"s")) transY -= 0.05f;
    if(!strcmp(k,"q")) transZ += 0.05f;
    if(!strcmp(k,"e")) transZ -= 0.05f;
    return 1;
}

// ---------- Draw ----------
void frame(){
    int w=800, h=600;
    glViewport(0,0,w,h);
    glClearColor(0.1,0.1,0.12,1);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);
    glUseProgram(program);

    Mat4 M = I();
    M = scaleM(M, scaleFactor);
    M = mul(rotX(rotationX), M);
    M = mul(rotY(rotationY), M);
    M = translate(M, transX, transY, transZ);

    Mat4 P = perspective(3.14159f/4, float(w)/h, 0.1f, 100.f);
    Mat4 MVP = mul(P, M);

    GLint uMVP = glGetUniformLocation(program,"uMVP");
    GLint uModel = glGetUniformLocation(program,"uModel");
    GLint uLight = glGetUniformLocation(program,"uLight");

    glUniformMatrix4fv(uMVP,1,GL_FALSE,MVP.m);
    glUniformMatrix4fv(uModel,1,GL_FALSE,M.m);
    glUniform3f(uLight,0.3,0.5,0.8);

    glBindBuffer(GL_ARRAY_BUFFER,vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,ibo);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0,3,GL_FLOAT,0,24,(void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1,3,GL_FLOAT,0,24,(void*)12);

    glDrawElements(GL_TRIANGLES,indexCount,GL_UNSIGNED_INT,0);
}

void buildDefaultCube() {
    struct V { float px,py,pz, nx,ny,nz; };
    V verts[] = {
        // Front face
        {-1,-1, 1, 0,0,1}, {1,-1, 1, 0,0,1}, {1,1, 1, 0,0,1}, {-1,1,1,0,0,1},
        // Back face
        {-1,-1,-1, 0,0,-1}, {-1,1,-1,0,0,-1}, {1,1,-1,0,0,-1}, {1,-1,-1,0,0,-1},
        // Left face
        {-1,-1,-1,-1,0,0}, {-1,-1,1,-1,0,0}, {-1,1,1,-1,0,0}, {-1,1,-1,-1,0,0},
        // Right face
        {1,-1,-1,1,0,0}, {1,1,-1,1,0,0}, {1,1,1,1,0,0}, {1,-1,1,1,0,0},
        // Top face
        {-1,1,-1,0,1,0}, {-1,1,1,0,1,0}, {1,1,1,0,1,0}, {1,1,-1,0,1,0},
        // Bottom face
        {-1,-1,-1,0,-1,0}, {1,-1,-1,0,-1,0}, {1,-1,1,0,-1,0}, {-1,-1,1,0,-1,0}
    };

    unsigned int idx[] = {
        0,1,2, 0,2,3,       // front
        4,5,6, 4,6,7,       // back
        8,9,10, 8,10,11,    // left
        12,13,14, 12,14,15, // right
        16,17,18, 16,18,19, // top
        20,21,22, 20,22,23  // bottom
    };

    indexCount = sizeof(idx)/sizeof(idx[0]);

    if(!vbo) glGenBuffers(1,&vbo);
    if(!ibo) glGenBuffers(1,&ibo);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);
}

EMSCRIPTEN_WEBGL_CONTEXT_HANDLE ctx;

int main(){
    EmscriptenWebGLContextAttributes attr;
    emscripten_webgl_init_context_attributes(&attr);
    attr.alpha = false;
    attr.depth = true;
    attr.stencil = false;
    attr.antialias = true;
    attr.majorVersion = 2; // WebGL2
    attr.minorVersion = 0;

    ctx = emscripten_webgl_create_context("#canvas", &attr);
    emscripten_webgl_make_context_current(ctx);

    emscripten_set_mousedown_callback("#canvas",0,1,md);
    emscripten_set_mouseup_callback("#canvas",0,1,mu);
    emscripten_set_mousemove_callback("#canvas",0,1,mm);
    emscripten_set_wheel_callback("#canvas",0,1,wheel);
    emscripten_set_keydown_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW,0,1,key);

    makeProgram();
    buildDefaultCube();
    emscripten_set_main_loop(frame,0,0);
    return 0;
}
