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