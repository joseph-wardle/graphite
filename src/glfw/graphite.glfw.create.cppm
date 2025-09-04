export module graphite.glfw:create;

import carbon.option;
import carbon.string;
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
