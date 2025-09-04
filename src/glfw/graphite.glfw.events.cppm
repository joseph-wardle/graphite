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
    using CloseFn  = bool(*)(void* user, CloseEvent);
}
