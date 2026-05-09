#include <libdecor.h>
#include <wayland-client.h>
#include <wayland-egl.h>

#include <EGL/egl.h>
#include <GLES2/gl2.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>

// ─── Wayland / libdecor ───────────────────────────────────────────────────────
static struct wl_display*    wl_dpy    = nullptr;
static struct wl_compositor* compositor = nullptr;
static struct wl_surface*    surface   = nullptr;
static struct libdecor*      decor_ctx = nullptr;
static struct libdecor_frame* decor_frame = nullptr;

static struct wl_egl_window* egl_win = nullptr;
static int     content_w = 800, content_h = 600;
static bool    configured = false;
static bool    closing    = false;

// ─── EGL / GLES globals ───────────────────────────────────────────────────────
static EGLDisplay egl_display = EGL_NO_DISPLAY;
static EGLContext egl_context = EGL_NO_CONTEXT;
static EGLSurface egl_surface = EGL_NO_SURFACE;
static GLuint program = 0;
static GLuint vbo     = 0;

// ─── shaders (GLES 2.0) ─────────────────────────────────────────────────────
static const char* vert_src = R"(
attribute vec3 aPos;
varying vec3 vColor;
void main() {
    gl_Position = vec4(aPos, 1.0);
    vColor = aPos * 0.5 + 0.5;
}
)";

static const char* frag_src = R"(
precision mediump float;
varying vec3 vColor;
uniform float uTime;
void main() {
    gl_FragColor = vec4(vColor.r + 0.2*sin(uTime), vColor.g, vColor.b + 0.2*cos(uTime*0.7), 1.0);
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

// ─── Registry ───────────────────────────────────────────────────────────────
static void registry_handler(void*, struct wl_registry* registry,
                             uint32_t name, const char* iface, uint32_t) {
    if (strcmp(iface, wl_compositor_interface.name) == 0)
        compositor = (wl_compositor*)wl_registry_bind(registry, name,
                                                      &wl_compositor_interface, 4);
}

static void registry_remover(void*, struct wl_registry*, uint32_t) {}

static const struct wl_registry_listener registry_listener = {
    registry_handler, registry_remover
};

// ─── libdecor callbacks ───────────────────────────────────────────────────────
static void libdecor_on_error(struct libdecor*, enum libdecor_error,
                             const char* message) {
    fprintf(stderr, "libdecor error: %s\n", message ? message : "(null)");
}

static void decor_frame_configure(struct libdecor_frame* frame,
                                  struct libdecor_configuration* cfg, void*) {
    int w = content_w, h = content_h;
    if (libdecor_configuration_get_content_size(cfg, frame, &w, &h)) {
        content_w = w;
        content_h = h;
    }
    struct libdecor_state* state = libdecor_state_new(content_w, content_h);
    libdecor_frame_commit(frame, state, cfg);
    libdecor_state_free(state);
    if (egl_win)
        wl_egl_window_resize(egl_win, content_w, content_h, 0, 0);
    configured = true;
}

static void decor_frame_close(struct libdecor_frame*, void*) {
    closing = true;
}

static void decor_frame_commit_cb(struct libdecor_frame*, void*) {
    wl_surface_commit(surface);
}

static void decor_frame_dismiss_popup(struct libdecor_frame*, const char*, void*) {}

static struct libdecor_interface libdecor_iface = {};
static struct libdecor_frame_interface frame_iface = {};

// ─── EGL init ───────────────────────────────────────────────────────────────
static bool init_egl_gles() {
    egl_display = eglGetDisplay((EGLNativeDisplayType)wl_dpy);
    if (egl_display == EGL_NO_DISPLAY) {
        fprintf(stderr, "eglGetDisplay failed\n");
        return false;
    }
    if (!eglInitialize(egl_display, nullptr, nullptr)) {
        fprintf(stderr, "eglInitialize failed\n");
        return false;
    }

    EGLint config_attribs[] = {
        EGL_SURFACE_TYPE,    EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_RED_SIZE,        8, EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE,       8, EGL_DEPTH_SIZE, 24,
        EGL_NONE
    };
    EGLConfig config;
    EGLint n;
    eglChooseConfig(egl_display, config_attribs, &config, 1, &n);
    if (n == 0) {
        fprintf(stderr, "No EGL config\n");
        return false;
    }

    EGLint ctx_attribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
    egl_context = eglCreateContext(egl_display, config, EGL_NO_CONTEXT, ctx_attribs);
    if (egl_context == EGL_NO_CONTEXT) {
        fprintf(stderr, "eglCreateContext failed\n");
        return false;
    }

    egl_win = wl_egl_window_create(surface, content_w, content_h);
    egl_surface = eglCreateWindowSurface(egl_display, config, (EGLNativeWindowType)egl_win, nullptr);
    if (egl_surface == EGL_NO_SURFACE) {
        fprintf(stderr, "eglCreateWindowSurface failed\n");
        return false;
    }
    eglMakeCurrent(egl_display, egl_surface, egl_surface, egl_context);
    return true;
}

static void pump_until_configured() {
    int guard = 0;
    while (!configured && wl_display_get_error(wl_dpy) == 0) {
        if (++guard > 100000) {
            fprintf(stderr,
                    "Timeout: no libdecor configure event (install a libdecor plugin, e.g. "
                    "libdecor-0-plugin-1-cairo)\n");
            std::exit(EXIT_FAILURE);
        }
        wl_display_dispatch(wl_dpy);
        libdecor_dispatch(decor_ctx, 0);
    }
}

// ─── main ────────────────────────────────────────────────────────────────────
int main() {
    fprintf(stderr,
            "Note: if the compositor does not expose zxdg_decoration_manager_v1 (see "
            "wayland-info), server-side decorations are unavailable; this demo uses "
            "libdecor for title bar and window controls.\n");

    libdecor_iface.error = libdecor_on_error;
    frame_iface.configure = decor_frame_configure;
    frame_iface.close = decor_frame_close;
    frame_iface.commit = decor_frame_commit_cb;
    frame_iface.dismiss_popup = decor_frame_dismiss_popup;

    wl_dpy = wl_display_connect(nullptr);
    if (!wl_dpy) {
        fprintf(stderr, "Failed to connect to Wayland display\n");
        return EXIT_FAILURE;
    }

    struct wl_registry* registry = wl_display_get_registry(wl_dpy);
    wl_registry_add_listener(registry, &registry_listener, nullptr);
    wl_display_roundtrip(wl_dpy);
    wl_registry_destroy(registry);

    if (!compositor) {
        fprintf(stderr, "Missing wl_compositor\n");
        return EXIT_FAILURE;
    }

    surface = wl_compositor_create_surface(compositor);
    decor_ctx = libdecor_new(wl_dpy, &libdecor_iface);
    if (!decor_ctx) {
        fprintf(stderr, "libdecor_new failed\n");
        return EXIT_FAILURE;
    }

    decor_frame = libdecor_decorate(decor_ctx, surface, &frame_iface, nullptr);
    if (!decor_frame) {
        fprintf(stderr, "libdecor_decorate failed\n");
        return EXIT_FAILURE;
    }

    libdecor_frame_set_title(decor_frame, "Wayland-EGL-GLES Window");
    libdecor_frame_set_app_id(decor_frame, "org.example.wayland-egl-gles");
    libdecor_frame_set_capabilities(decor_frame,
        static_cast<libdecor_capabilities>(
            LIBDECOR_ACTION_MOVE | LIBDECOR_ACTION_RESIZE |
            LIBDECOR_ACTION_MINIMIZE | LIBDECOR_ACTION_FULLSCREEN |
            LIBDECOR_ACTION_CLOSE));

    libdecor_frame_map(decor_frame);
    pump_until_configured();

    if (!init_egl_gles()) return EXIT_FAILURE;

    printf("GLES %s  GLSL %s  (EGL %s)\n",
           glGetString(GL_VERSION), glGetString(GL_SHADING_LANGUAGE_VERSION),
           eglQueryString(egl_display, EGL_VERSION));

    program = create_program(vert_src, frag_src);
    GLint uTime_loc = glGetUniformLocation(program, "uTime");
    GLint aPos_loc  = glGetAttribLocation(program, "aPos");

    float vertices[] = {
         0.0f,  0.5f, 0.0f,
        -0.5f, -0.5f, 0.0f,
         0.5f, -0.5f, 0.0f,
    };

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    for (int frame = 0; !closing; frame++) {
        wl_display_dispatch_pending(wl_dpy);
        libdecor_dispatch(decor_ctx, 0);

        float t = (float)frame * 0.016f;
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(program);
        glUniform1f(uTime_loc, t);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glVertexAttribPointer(aPos_loc, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(aPos_loc);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        eglSwapBuffers(egl_display, egl_surface);
    }

    glDeleteProgram(program);
    glDeleteBuffers(1, &vbo);
    eglDestroySurface(egl_display, egl_surface);
    wl_egl_window_destroy(egl_win);
    eglDestroyContext(egl_display, egl_context);
    eglTerminate(egl_display);
    libdecor_frame_unref(decor_frame);
    wl_surface_destroy(surface);
    libdecor_unref(decor_ctx);
    wl_compositor_destroy(compositor);
    wl_display_disconnect(wl_dpy);
    return EXIT_SUCCESS;
}
