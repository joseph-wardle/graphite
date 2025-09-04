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
