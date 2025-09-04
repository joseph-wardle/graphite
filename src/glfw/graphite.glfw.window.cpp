module;
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
module graphite.glfw;
import :window;

import carbon.types;
import carbon.string;
import carbon.result;
import carbon.math;
import :create;
import :error;
import :events;
import :types;

namespace graphite::glfw {

static Window* from(GLFWwindow* w) noexcept {
    return static_cast<Window*>(glfwGetWindowUserPointer(w));
}

struct WindowCallbacks {
    static void window_size(GLFWwindow* w, const i32 width, const i32 height) noexcept {
        if (auto* self = from(w)) {
            if (self->synthesize_guard_) return;
            int fbw, fbh; glfwGetFramebufferSize(w, &fbw, &fbh);
            if (!self->resizing_) {
                self->resizing_ = true;
                self->_emit_resize(Size{width, height}, Size{fbw, fbh}, ResizePhase::Begin);
            }
            self->_emit_resize(Size{width, height}, Size{fbw, fbh}, ResizePhase::Step);
        }
    }

    static void framebuffer_size(GLFWwindow* w, const i32 fbw, const i32 fbh) noexcept {
        if (auto* self = from(w)) {
            if (self->synthesize_guard_) return;
            int ww, wh; glfwGetWindowSize(w, &ww, &wh);
            if (self->resizing_) {
                self->_emit_resize(Size{ww, wh}, Size{fbw, fbh}, ResizePhase::End);
                self->resizing_ = false;
            } else {
                self->_emit_resize(Size{ww, wh}, Size{fbw, fbh}, ResizePhase::Step);
            }
        }
    }

    static void content_scale(GLFWwindow* w, const f32 sx, const f32 sy) noexcept {
        if (const auto* self = from(w)) self->_emit_scale(Scale{sx, sy});
    }

    static void window_close(GLFWwindow* w) noexcept {
        if (const auto* self = from(w)) {
            if (!self->_emit_close()) glfwSetWindowShouldClose(w, 0);
        }
    }
};

[[nodiscard]] static GLFWwindow* as_glfw(void* h) noexcept {
    return static_cast<GLFWwindow*>(h);
}

[[nodiscard]] static GLFWmonitor* pick_monitor_for(GLFWwindow* w) noexcept {
    i32 wx, wy, ww, wh;
    glfwGetWindowPos(w, &wx, &wy);
    glfwGetWindowSize(w, &ww, &wh);

    i32 count = 0;
    GLFWmonitor** mons = glfwGetMonitors(&count);
    if (!mons || count == 0) return glfwGetPrimaryMonitor();

    GLFWmonitor* best = mons[0];
    long best_area = -1;

    for (i32 i = 0; i < count; ++i) {
        i32 mx, my, mw, mh;
        glfwGetMonitorWorkarea(mons[i], &mx, &my, &mw, &mh);

        const i32 ix = carbon::max(wx, mx);
        const i32 iy = carbon::max(wy, my);
        const i32 ax = carbon::min(wx + ww, mx + mw) - ix;
        const i32 ay = carbon::min(wy + wh, my + mh) - iy;

        if (const long area = (ax > 0 && ay > 0) ? static_cast<long>(ax) * static_cast<long>(ay) : 0; area > best_area)
        {
            best_area = area; best = mons[i];
        }
    }
    return best ? best : glfwGetPrimaryMonitor();
}

// ---- Window methods ----

Window::~Window() noexcept {
    if (h_) {
        glfwSetWindowUserPointer(as_glfw(h_), nullptr);
        glfwDestroyWindow(as_glfw(h_));
        h_ = nullptr;
    }
}

Window::Window(Window&& o) noexcept {
    h_ = o.h_;      o.h_ = nullptr;
    user_ = o.user_; o.user_ = nullptr;
    on_resize_ = o.on_resize_; o.on_resize_ = nullptr;
    on_scale_  = o.on_scale_;  o.on_scale_  = nullptr;
    on_close_  = o.on_close_;  o.on_close_  = nullptr;
    cb_user_   = o.cb_user_;   o.cb_user_   = nullptr;
    resizing_  = o.resizing_;  o.resizing_  = false;
    if (h_) glfwSetWindowUserPointer(as_glfw(h_), this);
}

Window& Window::operator=(Window&& o) noexcept {
    if (this == &o) return *this;
    // destroy current
    if (h_) {
        glfwSetWindowUserPointer(as_glfw(h_), nullptr);
        glfwDestroyWindow(as_glfw(h_));
    }
    h_ = o.h_;      o.h_ = nullptr;
    user_ = o.user_; o.user_ = nullptr;
    on_resize_ = o.on_resize_; o.on_resize_ = nullptr;
    on_scale_  = o.on_scale_;  o.on_scale_  = nullptr;
    on_close_  = o.on_close_;  o.on_close_  = nullptr;
    cb_user_   = o.cb_user_;   o.cb_user_   = nullptr;
    resizing_  = o.resizing_;  o.resizing_  = false;
    if (h_) glfwSetWindowUserPointer(as_glfw(h_), this);
    return *this;
}

Result<Window, Error> Window::create(const WindowCreateInfo& create_info) noexcept {
    glfwDefaultWindowHints();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_FOCUS_ON_SHOW, create_info.start_focused ? 1 : 0);

    const char* title = create_info.title.data();

    GLFWmonitor* monitor_for_creation = nullptr;
    i32 w = create_info.size.w, h = create_info.size.h;

    if (create_info.mode == Mode::ExclusiveFullscreen) {
        monitor_for_creation = glfwGetPrimaryMonitor();
        if (const GLFWvidmode* vm = glfwGetVideoMode(monitor_for_creation)) {
            w = vm->width; h = vm->height;
        }
    }

    GLFWwindow* wptr = glfwCreateWindow(w, h, title, monitor_for_creation, nullptr);
    if (!wptr) {
        return Result<Window, Error>::err(Error::WindowCreationFailed);
    }

    Window out{};
    out.h_ = wptr;

    glfwSetWindowUserPointer(wptr, &out);

    // Apply size constraints / aspect if provided
    const Size min = create_info.min_size.value_or(Size{GLFW_DONT_CARE, GLFW_DONT_CARE});
    const Size max = create_info.max_size.value_or(Size{GLFW_DONT_CARE, GLFW_DONT_CARE});
    out.set_size_limits(min, max);

    if (create_info.aspect.is_some()) {
        out.set_aspect_ratio(create_info.aspect.value());
    }

    // Wire core callbacks
    glfwSetWindowSizeCallback        (wptr, &WindowCallbacks::window_size);
    glfwSetFramebufferSizeCallback   (wptr, &WindowCallbacks::framebuffer_size);
    glfwSetWindowContentScaleCallback(wptr, &WindowCallbacks::content_scale);
    glfwSetWindowCloseCallback       (wptr, &WindowCallbacks::window_close);

    if (create_info.mode == Mode::BorderlessFullscreen) {
        (void)out.set_borderless_fullscreen();
    }

    return Result<Window, Error>::ok(static_cast<Window&&>(out));
}

// ---- basic controls ----

void Window::show() const noexcept {
    if (h_) glfwShowWindow(as_glfw(h_));
}
void Window::hide() const noexcept {
    if (h_) glfwHideWindow(as_glfw(h_));
}
void Window::focus() const noexcept {
    if (h_) glfwFocusWindow(as_glfw(h_));
}
void Window::request_attention() const noexcept {
    if (h_) glfwRequestWindowAttention(as_glfw(h_));
}

// ---- lifecycle ----

bool Window::should_close() const noexcept {
    return h_ ? glfwWindowShouldClose(as_glfw(h_)) == 1 : true;
}
void Window::set_should_close(const bool v) const noexcept {
    if (h_) glfwSetWindowShouldClose(as_glfw(h_), v ? 1 : 0);
}
void Window::close_now() noexcept {
    if (!h_) return;
    glfwSetWindowUserPointer(as_glfw(h_), nullptr);
    glfwDestroyWindow(as_glfw(h_));
    h_ = nullptr;
}

// ---- state ----

void Window::iconify() const noexcept {
    if (h_) glfwIconifyWindow(as_glfw(h_));
}
void Window::restore() const noexcept {
    if (h_) glfwRestoreWindow(as_glfw(h_));
}
void Window::maximize() const noexcept {
    if (h_) glfwMaximizeWindow(as_glfw(h_));
}

// ---- geometry / DPI ----

Pos Window::position() const noexcept {
    if (!h_) return Pos{0,0};
    i32 x=0,y=0; glfwGetWindowPos(as_glfw(h_), &x, &y);
    return Pos{ x, y };
}
void Window::set_position(const Pos p) const noexcept {
    if (h_) glfwSetWindowPos(as_glfw(h_), p.x, p.y);
}

Size Window::size() const noexcept {
    if (!h_) return Size{0,0};
    i32 w=0,h=0; glfwGetWindowSize(as_glfw(h_), &w, &h);
    return Size{ w, h };
}

void Window::set_size(const Size s) noexcept {
    if (!h_) return;
    synthesize_guard_ = true;
    resizing_ = true;
    glfwSetWindowSize(as_glfw(h_), s.w, s.h);

    i32 fbw=0, fbh=0; glfwGetFramebufferSize(as_glfw(h_), &fbw, &fbh);
    _emit_resize(s, Size{fbw, fbh}, ResizePhase::Begin);
    _emit_resize(s, Size{fbw, fbh}, ResizePhase::Step);
    _emit_resize(s, Size{fbw, fbh}, ResizePhase::End);

    resizing_ = false;
    synthesize_guard_ = false;
}

Size Window::framebuffer_size() const noexcept {
    if (!h_) return Size{0,0};
    i32 w=0,h=0; glfwGetFramebufferSize(as_glfw(h_), &w, &h);
    return Size{ w, h };
}
Scale Window::content_scale() const noexcept {
    if (!h_) return Scale{1.0f, 1.0f};
    f32 sx=1.0f, sy=1.0f; glfwGetWindowContentScale(as_glfw(h_), &sx, &sy);
    return Scale{ sx, sy };
}

void Window::set_size_limits(const Size min, const Size max) const noexcept {
    if (!h_) return;
    const int min_w = (min.w > 0) ? min.w : GLFW_DONT_CARE;
    const int min_h = (min.h > 0) ? min.h : GLFW_DONT_CARE;
    const int max_w = (max.w > 0) ? max.w : GLFW_DONT_CARE;
    const int max_h = (max.h > 0) ? max.h : GLFW_DONT_CARE;
    glfwSetWindowSizeLimits(as_glfw(h_), min_w, min_h, max_w, max_h);
}

void Window::set_aspect_ratio(const Aspect a) const noexcept {
    if (!h_) return;
    if (a.is_valid()) {
        glfwSetWindowAspectRatio(as_glfw(h_), a.num, a.den);
    } else {
        glfwSetWindowAspectRatio(as_glfw(h_), GLFW_DONT_CARE, GLFW_DONT_CARE);
    }
}

void Window::set_title(const StringView title) const noexcept {
    if (!h_) return;
    glfwSetWindowTitle(as_glfw(h_), title.data());
}

// ---- mode switching ----

Result<void, Error> Window::set_windowed() noexcept {
    if (!h_) return Result<void, Error>::ok();

    i32 ww, wh; glfwGetWindowSize(as_glfw(h_), &ww, &wh);
    i32 wx, wy; glfwGetWindowPos(as_glfw(h_), &wx, &wy);
    glfwSetWindowMonitor(as_glfw(h_), nullptr, wx, wy, ww, wh, 0);

    glfwSetWindowAttrib(as_glfw(h_), GLFW_DECORATED, 1);
    return Result<void, Error>::ok();
}

Result<void, Error> Window::set_exclusive_fullscreen() noexcept {
    if (!h_) return Result<void, Error>::ok();

    GLFWmonitor* mon = pick_monitor_for(as_glfw(h_));
    if (!mon) return Result<void, Error>::err(Error::Platform);

    const GLFWvidmode* vm = glfwGetVideoMode(mon);
    if (!vm) return Result<void, Error>::err(Error::Platform);

    glfwSetWindowMonitor(as_glfw(h_), mon, 0, 0, vm->width, vm->height, vm->refreshRate);
    return Result<void, Error>::ok();
}

Result<void, Error> Window::set_borderless_fullscreen() noexcept {
    if (!h_) return Result<void, Error>::ok();

    GLFWmonitor* mon = pick_monitor_for(as_glfw(h_));
    if (!mon) mon = glfwGetPrimaryMonitor();
    if (!mon) return Result<void, Error>::err(Error::Platform);

    i32 mx, my, mw, mh;
    glfwGetMonitorWorkarea(mon, &mx, &my, &mw, &mh);

    glfwSetWindowAttrib(as_glfw(h_), GLFW_DECORATED, 0);
    glfwSetWindowMonitor(as_glfw(h_), nullptr, mx, my, mw, mh, 0);
    return Result<void, Error>::ok();
}

// ---- event emission ----

void Window::_emit_resize(const Size logical, const Size fb, const ResizePhase phase) const noexcept {
    if (on_resize_) {
        const ResizeEvent e{logical, fb, phase};
        on_resize_(cb_user_, e);
    }
}

void Window::_emit_scale(const Scale s) const noexcept {
    if (on_scale_) {
        const ScaleEvent e{s};
        on_scale_(cb_user_, e);
    }
}

bool Window::_emit_close() const noexcept {
    return on_close_ ? on_close_(cb_user_, CloseEvent{}) : true;
}

} // namespace graphite::glfw