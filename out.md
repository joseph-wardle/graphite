```
glfw/
├─ CMakeLists.txt
├─ graphite.glfw.core.cpp
├─ graphite.glfw.core.cppm
├─ graphite.glfw.cppm
├─ graphite.glfw.create.cppm
├─ graphite.glfw.error.cppm
├─ graphite.glfw.events.cppm
├─ graphite.glfw.types.cppm
├─ graphite.glfw.window.cpp
└─ graphite.glfw.window.cppm
```

---

# graphite.glfw.window.cpp

```cpp
module;
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
module graphite.glfw;
import :window;

import carbon.types;
import carbon.string_view;
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
        if (glfwGetMonitorWorkarea) {
            glfwGetMonitorWorkarea(mons[i], &mx, &my, &mw, &mh);
        } else {
            const GLFWvidmode* vm = glfwGetVideoMode(mons[i]);
            mx = 0; my = 0; mw = vm ? vm->width : 0; mh = vm ? vm->height : 0;
        }

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
    glfwSetWindowUserPointer(wptr, &out);
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
    if (h_) glfwSetWindowShouldClose(as_glfw(h_), v);
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

    const auto [min_w, min_h] = min;
    const auto [max_w, max_h] = max;

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
    if (glfwGetMonitorWorkarea) {
        glfwGetMonitorWorkarea(mon, &mx, &my, &mw, &mh);
    } else {
        const GLFWvidmode* vm = glfwGetVideoMode(mon);
        mx = 0; my = 0; mw = vm ? vm->width : 0; mh = vm ? vm->height : 0;
    }

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
```

---

# graphite.glfw.core.cpp

```cpp
module;
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
module graphite.glfw;
import :core;

import carbon.result;
import carbon.types;

import :error;
namespace {

thread_local auto t_last_error = graphite::glfw::Error::None;

constexpr graphite::glfw::Error map_glfw_error(const i32 code) noexcept {
    using E = graphite::glfw::Error;
    switch (code) {
        case GLFW_NO_ERROR:            return E::None;
        case GLFW_NOT_INITIALIZED:     return E::NotInitialized;
        case GLFW_INVALID_ENUM:        // fold into InvalidValue
        case GLFW_INVALID_VALUE:       return E::InvalidValue;
        case GLFW_OUT_OF_MEMORY:       return E::OutOfMemory;
        case GLFW_API_UNAVAILABLE:     // missing backend/driver
        case GLFW_VERSION_UNAVAILABLE: return E::ApiUnavailable;
        case GLFW_PLATFORM_ERROR:      // platform-specific failure
        case GLFW_FORMAT_UNAVAILABLE:  return E::Platform;
        case GLFW_NO_CURRENT_CONTEXT:  // GL-specific precondition miss → ApiUnavailable
        case GLFW_NO_WINDOW_CONTEXT:   return E::ApiUnavailable;
        default:                       return E::Unknown;
    }
}

// GLFW error callback – store the mapped enum (message omitted to stay no-alloc/no-std)
void glfw_error_cb(const i32 code, const char* /*desc*/) noexcept {
    t_last_error = map_glfw_error(code);
}

} // namespace

namespace graphite::glfw {

Result<void, Error> init() noexcept {
    glfwSetErrorCallback(glfw_error_cb);

    if (glfwInit() == GLFW_TRUE) return Result<void, Error>::ok();

    // If GLFW didn't signal anything yet, ensure a sensible error code.
    if (t_last_error == Error::None) {
        t_last_error = Error::NotInitialized;
    }
    return Result<void, Error>::err(t_last_error);
}

void shutdown() noexcept {
    glfwTerminate();
}

void poll_events() noexcept {
    glfwPollEvents();
}

Error last_error() noexcept {
    const Error e = t_last_error;
    t_last_error = Error::None;
    return e;
}

} // namespace graphite::glfw
```

---

# graphite.glfw.create.cppm

```cpp
export module graphite.glfw:create;

import carbon.option;
import carbon.string_view;
import carbon.types;

import :types;

using carbon::StringView, carbon::Option;

export namespace graphite::glfw {
    enum class Mode : u8 { Windowed, ExclusiveFullscreen, BorderlessFullscreen };

    struct WindowCreateInfo {
        StringView title;
        Size       size;
        Mode       mode  = Mode::Windowed;

        // Constraints (optional)
        Option<Size>  min_size {};
        Option<Size>  max_size {};
        Option<Aspect> aspect {};

        // Behavioral
        bool  start_focused   = true;
    };
}
```

---

# CMakeLists.txt

```
CPMAddPackage(
        NAME glfw
        GITHUB_REPOSITORY glfw/glfw
        GIT_TAG 3.4
        OPTIONS
            "GLFW_BUILD_TESTS OFF"
            "GLFW_BUILD_EXAMPLES OFF"
            "GLFW_BUILD_DOCS OFF"
            "BUILD_SHARED_LIBS OFF"
)

add_library(graphite_glfw STATIC)
add_library(graphite::glfw ALIAS graphite_glfw)

target_sources(graphite_glfw
    PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES
        FILES
            graphite.glfw.cppm
            graphite.glfw.core.cppm
            graphite.glfw.create.cppm
            graphite.glfw.error.cppm
            graphite.glfw.events.cppm
            graphite.glfw.types.cppm
            graphite.glfw.window.cppm
    PRIVATE
        graphite.glfw.core.cpp
        graphite.glfw.window.cpp
)

target_link_libraries(graphite_glfw PUBLIC carbon PRIVATE glfw)
target_compile_features(graphite_glfw PUBLIC cxx_std_23)
graphite_strict_warnings(graphite_glfw)
```

---

# graphite.glfw.types.cppm

```cpp
export module graphite.glfw:types;

import carbon.types;

export namespace graphite::glfw {
    struct Size  { i32 w, h; };
    struct Pos   { i32 x, y; };
    struct Scale { f32 x, y; };

    struct Aspect {
        i32 num, den;
        [[nodiscard]] constexpr bool is_valid() const noexcept { return num > 0 && den > 0; }
    };
} // namespace graphite::glfw
```

---

# graphite.glfw.window.cppm

```cpp
export module graphite.glfw:window;

import carbon.result;
import carbon.string_view;
import carbon.types;

using carbon::Result;

import :create;
import :error;
import :events;
import :types;

export namespace graphite::glfw {

    class Window {
        void* h_{nullptr};       // holds GLFWwindow* (opaque to users)
        void* user_{nullptr};    // user-attached pointer

        // Registered callbacks
        ResizeFn on_resize_{nullptr};
        ScaleFn  on_scale_{nullptr};
        CloseFn  on_close_{nullptr};
        void*    cb_user_{nullptr};

        bool resizing_{false};
        bool synthesize_guard_{false};

        void _emit_resize(Size logical, Size fb, ResizePhase phase) const noexcept;
        void _emit_scale (Scale s) const noexcept;
        [[nodiscard]] bool _emit_close () const noexcept;

        friend struct WindowCallbacks;

    public:
        Window() = default;
        ~Window() noexcept;
        Window(Window&& o) noexcept;
        Window& operator=(Window&& o) noexcept;
        Window(const Window&) = delete;
        Window& operator=(const Window&) = delete;



        [[nodiscard]] bool valid() const noexcept { return h_ != nullptr; }

        // ---- Creation ----
        static Result<Window, Error> create(const WindowCreateInfo& create_info) noexcept;

        // ---- Native escape hatch ----
        [[nodiscard]] void* native_handle() const noexcept { return h_; } // GLFWwindow*

        // ---- User pointer ----
        void  set_user_ptr(void* p) noexcept { user_ = p; }
        [[nodiscard]] void* user_ptr() const noexcept { return user_; }

        // ---- Visibility & focus ----
        void show() const noexcept;
        void hide() const noexcept;
        void focus() const noexcept;
        void request_attention() const noexcept;

        // ---- Lifecycle ----
        [[nodiscard]] bool should_close() const noexcept;
        void set_should_close(bool v) const noexcept;
        void close_now() noexcept;

        // ---- Window state ----
        void iconify() const noexcept;
        void restore() const noexcept;
        void maximize() const noexcept;

        // ---- Position & size ----
        [[nodiscard]] Pos position() const noexcept;
        void set_position(Pos p) const noexcept;

        [[nodiscard]] Size size() const noexcept;
        void set_size(Size s) noexcept;

        [[nodiscard]] Size framebuffer_size() const noexcept; // pixel size (for Vulkan swapchain)
        [[nodiscard]] Scale content_scale() const noexcept;   // DPI scale

        void set_size_limits(Size min, Size max) const noexcept; // {0,0} => unset
        void set_aspect_ratio(Aspect a) const noexcept;          // invalid => unset

        // ---- Title ----
        void set_title(StringView title) const noexcept;

        // ---- Cursor capture modes are in input module; omitted here ----

        // ---- Mode switching ----
        Result<void, Error> set_windowed() noexcept;                       // back to normal
        Result<void, Error> set_exclusive_fullscreen() noexcept;
        Result<void, Error> set_borderless_fullscreen() noexcept;

        // ---- Callbacks (single handler each; simple & cache-friendly) ----
        void on_resize(const ResizeFn fn, void* user) noexcept { on_resize_ = fn; cb_user_ = user; }
        void on_scale (const ScaleFn  fn, void* user) noexcept { on_scale_  = fn; cb_user_ = user; }
        void on_close (const CloseFn  fn, void* user) noexcept { on_close_  = fn; cb_user_ = user; }
    };
}
```

---

# graphite.glfw.error.cppm

```cpp
export module graphite.glfw:error;

import carbon.types;

export namespace graphite::glfw {
    enum class Error : i32 {
        None = 0,
        NotInitialized,
        InvalidValue,
        Platform,
        OutOfMemory,
        ApiUnavailable,
        MonitorUnavailable,
        WindowCreationFailed,
        Unknown,
    };
} // namespace graphite::glfw
```

---

# graphite.glfw.cppm

```cpp
export module graphite.glfw;

export import :core;
export import :create;
export import :error;
export import :events;
export import :input;
export import :types;
export import :window;
```

---

# graphite.glfw.events.cppm

```cpp
export module graphite.glfw:events;

import carbon.types;
import :types;

export namespace graphite::glfw {
    enum class ResizePhase : u8 { Begin, Step, End };

    struct ResizeEvent {
        Size logical;        // window size in screen coords
        Size framebuffer;    // pixel size (swapchain extent)
        ResizePhase phase;
    };

    struct ScaleEvent { Scale content_scale; }; // DPI scale changed
    struct CloseEvent { /* empty */ };

    using ResizeFn = void(*)(void* user, ResizeEvent);
    using ScaleFn  = void(*)(void* user, ScaleEvent);
    using CloseFn  = void(*)(void* user, CloseEvent);
}
```

---

# graphite.glfw.core.cppm

```cpp
export module graphite.glfw:core;

import carbon.result;
import carbon.types;
import :error;

using carbon::Result;

export namespace graphite::glfw {

    [[nodiscard]] Result<void, Error> init() noexcept;

    void shutdown() noexcept;

    void poll_events() noexcept;

    [[nodiscard]] Error last_error() noexcept;
}
```

