export module graphite.glfw:window;
import :create;
import :error;
import :events;
import :types;

import carbon.result;
import carbon.string_view;
import carbon.types;

using carbon::Result;

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
