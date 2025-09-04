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