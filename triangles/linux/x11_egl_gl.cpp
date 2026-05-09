#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>

#include <EGL/egl.h>

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>

#include <cstdio>
#include <cstdlib>
#include <cmath>

static Display*   display     = nullptr;
static Window     window      = 0;
static EGLDisplay egl_display = EGL_NO_DISPLAY;
static EGLContext egl_context = EGL_NO_CONTEXT;
static EGLSurface egl_surface = EGL_NO_SURFACE;
static Atom       wm_delete   = 0;

static GLuint program = 0;
static GLuint vao     = 0;
static GLuint vbo     = 0;

// ─── shaders (Desktop GL 3.3 core) ──────────────────────────────────────────
static const char* vert_src = R"(#version 330 core
layout(location = 0) in vec3 aPos;
void main() {
    gl_Position = vec4(aPos, 1.0);
}
)";

static const char* frag_src = R"(#version 330 core
out vec4 fragColor;
uniform float uTime;
void main() {
    fragColor = vec4(0.5 + 0.5*sin(uTime), 0.3, 0.7 + 0.3*cos(uTime*0.7), 1.0);
}
)";

// ─── helpers ─────────────────────────────────────────────────────────────────
static GLuint compile_shader(GLenum type, const char* src) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);
    GLint ok;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        fprintf(stderr, "Shader compile error: %s\n", log);
    }
    return shader;
}

static GLuint create_program(const char* vert, const char* frag) {
    GLuint vs = compile_shader(GL_VERTEX_SHADER,   vert);
    GLuint fs = compile_shader(GL_FRAGMENT_SHADER, frag);
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    GLint ok;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetProgramInfoLog(prog, sizeof(log), nullptr, log);
        fprintf(stderr, "Program link error: %s\n", log);
    }
    glDeleteShader(vs);
    glDeleteShader(fs);
    return prog;
}

// ─── main ────────────────────────────────────────────────────────────────────
int main() {
    // ── X11: open display & create window ──────────────────────────────────
    display = XOpenDisplay(nullptr);
    if (!display) {
        fprintf(stderr, "Failed to open X display\n");
        return EXIT_FAILURE;
    }

    Window root = DefaultRootWindow(display);

    XSetWindowAttributes swa;
    swa.event_mask = ExposureMask | KeyPressMask | StructureNotifyMask;
    window = XCreateWindow(display, root, 0, 0, 800, 600, 0,
                           CopyFromParent, InputOutput, CopyFromParent,
                           CWEventMask, &swa);
    XStoreName(display, window, "X11-EGL-GL Window");
    XMapWindow(display, window);

    wm_delete = XInternAtom(display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(display, window, &wm_delete, 1);

    // ── EGL: init display ──────────────────────────────────────────────────
    egl_display = eglGetDisplay((EGLNativeDisplayType)display);
    if (egl_display == EGL_NO_DISPLAY) {
        fprintf(stderr, "Failed to get EGL display\n");
        return EXIT_FAILURE;
    }
    if (!eglInitialize(egl_display, nullptr, nullptr)) {
        fprintf(stderr, "Failed to initialize EGL\n");
        return EXIT_FAILURE;
    }

    // ── EGL: choose config (Desktop OpenGL) ────────────────────────────────
    EGLint config_attribs[] = {
        EGL_SURFACE_TYPE,    EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
        EGL_RED_SIZE,        8,
        EGL_GREEN_SIZE,      8,
        EGL_BLUE_SIZE,       8,
        EGL_DEPTH_SIZE,      24,
        EGL_NONE
    };
    EGLConfig config;
    EGLint num_configs;
    eglChooseConfig(egl_display, config_attribs, &config, 1, &num_configs);
    if (num_configs == 0) {
        fprintf(stderr, "No matching EGL config\n");
        return EXIT_FAILURE;
    }

    // ── EGL: create context (OpenGL 3.3 core profile) ──────────────────────
    eglBindAPI(EGL_OPENGL_API);
    EGLint ctx_attribs[] = {
        EGL_CONTEXT_MAJOR_VERSION,              3,
        EGL_CONTEXT_MINOR_VERSION,              3,
        EGL_CONTEXT_OPENGL_PROFILE_MASK,        EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT,
        EGL_NONE
    };
    egl_context = eglCreateContext(egl_display, config, EGL_NO_CONTEXT, ctx_attribs);
    if (egl_context == EGL_NO_CONTEXT) {
        fprintf(stderr, "Failed to create EGL context\n");
        return EXIT_FAILURE;
    }

    // ── EGL: create window surface & make current ──────────────────────────
    egl_surface = eglCreateWindowSurface(egl_display, config, (EGLNativeWindowType)window, nullptr);
    if (egl_surface == EGL_NO_SURFACE) {
        fprintf(stderr, "Failed to create EGL surface\n");
        return EXIT_FAILURE;
    }
    eglMakeCurrent(egl_display, egl_surface, egl_surface, egl_context);

    printf("OpenGL %s  GLSL %s  (EGL %s)\n",
           glGetString(GL_VERSION), glGetString(GL_SHADING_LANGUAGE_VERSION),
           eglQueryString(egl_display, EGL_VERSION));

    // ── Compile shaders ────────────────────────────────────────────────────
    program = create_program(vert_src, frag_src);
    GLint uTime_loc = glGetUniformLocation(program, "uTime");

    // ── Triangle geometry (VAO + VBO) ──────────────────────────────────────
    float vertices[] = {
         0.0f,  0.5f, 0.0f,
        -0.5f, -0.5f, 0.0f,
         0.5f, -0.5f, 0.0f,
    };

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // ── Render loop ────────────────────────────────────────────────────────
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    XEvent event;
    for (int frame = 0; ; frame++) {
        while (XPending(display)) {
            XNextEvent(display, &event);
            switch (event.type) {
            case KeyPress:
                goto cleanup;
            case ClientMessage:
                if ((Atom)event.xclient.data.l[0] == wm_delete)
                    goto cleanup;
            }
        }

        float t = (float)frame * 0.016f;
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(program);
        glUniform1f(uTime_loc, t);
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        eglSwapBuffers(egl_display, egl_surface);
    }

cleanup:
    glDeleteProgram(program);
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    eglDestroySurface(egl_display, egl_surface);
    eglDestroyContext(egl_display, egl_context);
    eglTerminate(egl_display);
    XDestroyWindow(display, window);
    XCloseDisplay(display);
    return EXIT_SUCCESS;
}
