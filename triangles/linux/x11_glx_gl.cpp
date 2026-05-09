#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>

#define GL_GLEXT_PROTOTYPES
#include <GL/glx.h>
#include <GL/glxext.h>

#include <cstdio>
#include <cstdlib>
#include <cmath>

static Display*    display   = nullptr;
static Window      window    = 0;
static GLXContext  glx_ctx   = nullptr;
static GLuint      program   = 0;
static GLuint      vao       = 0;
static GLuint      vbo       = 0;
static Atom        wm_delete = 0;

// ─── shaders (inline) ───────────────────────────────────────────────────────
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
    GLuint vs = compile_shader(GL_VERTEX_SHADER, vert);
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

    int screen = DefaultScreen(display);
    Window root = RootWindow(display, screen);

    // ── GLX: choose FB config ──────────────────────────────────────────────
    static int vis_attribs[] = {
        GLX_X_RENDERABLE,  True,
        GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
        GLX_RENDER_TYPE,   GLX_RGBA_BIT,
        GLX_X_VISUAL_TYPE, GLX_TRUE_COLOR,
        GLX_RED_SIZE,      8,
        GLX_GREEN_SIZE,    8,
        GLX_BLUE_SIZE,     8,
        GLX_DEPTH_SIZE,    24,
        GLX_DOUBLEBUFFER,  True,
        None
    };

    int fbcount = 0;
    GLXFBConfig* fbc = glXChooseFBConfig(display, screen, vis_attribs, &fbcount);
    if (!fbc || fbcount == 0) {
        fprintf(stderr, "Failed to choose GLX FB config\n");
        return EXIT_FAILURE;
    }

    XVisualInfo* vi = glXGetVisualFromFBConfig(display, fbc[0]);

    // ── X11: create window with the GLX-chosen visual ──────────────────────
    XSetWindowAttributes swa;
    swa.colormap   = XCreateColormap(display, root, vi->visual, AllocNone);
    swa.event_mask = ExposureMask | KeyPressMask | StructureNotifyMask;

    window = XCreateWindow(display, root, 0, 0, 800, 600, 0,
                           vi->depth, InputOutput, vi->visual,
                           CWColormap | CWEventMask, &swa);
    XStoreName(display, window, "X11-GLX-GL Window");
    XMapWindow(display, window);

    wm_delete = XInternAtom(display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(display, window, &wm_delete, 1);

    // ── GLX: create context (ARB_create_context for 3.3 core) ──────────────
    PFNGLXCREATECONTEXTATTRIBSARBPROC glXCreateContextAttribsARB =
        (PFNGLXCREATECONTEXTATTRIBSARBPROC)glXGetProcAddress(
            (const GLubyte*)"glXCreateContextAttribsARB");
    if (!glXCreateContextAttribsARB) {
        fprintf(stderr, "glXCreateContextAttribsARB not available\n");
        return EXIT_FAILURE;
    }

    static int ctx_attribs[] = {
        GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
        GLX_CONTEXT_MINOR_VERSION_ARB, 3,
        GLX_CONTEXT_PROFILE_MASK_ARB,  GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
        None
    };

    glx_ctx = glXCreateContextAttribsARB(display, fbc[0], nullptr, True, ctx_attribs);
    if (!glx_ctx) {
        fprintf(stderr, "Failed to create GLX 3.3 core context\n");
        return EXIT_FAILURE;
    }

    glXMakeCurrent(display, window, glx_ctx);
    XFree(vi);
    XFree(fbc);

    printf("OpenGL %s  GLSL %s  (GLX 1.4)\n",
           glGetString(GL_VERSION), glGetString(GL_SHADING_LANGUAGE_VERSION));

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
        glXSwapBuffers(display, window);
    }

cleanup:
    glDeleteProgram(program);
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glXDestroyContext(display, glx_ctx);
    XDestroyWindow(display, window);
    XCloseDisplay(display);
    return EXIT_SUCCESS;
}
