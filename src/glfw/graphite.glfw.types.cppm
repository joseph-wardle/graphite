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
