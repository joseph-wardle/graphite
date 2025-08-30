function(graphite_strict_warnings target)
    target_compile_options(${target} PRIVATE
            # Apply to C sources compiled with Clang/AppleClang
            $<$<COMPILE_LANG_AND_ID:C,Clang,AppleClang>:-Wall -Wextra -Wpedantic -Werror>
            # Apply to C++ sources compiled with Clang/AppleClang
            $<$<COMPILE_LANG_AND_ID:CXX,Clang,AppleClang>:-Wall -Wextra -Wpedantic -Werror>
    )
endfunction()