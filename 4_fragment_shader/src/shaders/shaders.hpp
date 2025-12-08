// Shader constants header
// The actual shader content is generated at build time in shaders_generated.hpp

#pragma once

#ifdef SHADERS_GENERATED
    // Build time: use generated shader content
    #include "shaders_generated.hpp"
#else
    // Pre-build: provide placeholder declarations
    namespace shaders {
        constexpr const char *SIMPLE_VERT_SHADER = "";
        constexpr const char *BASE_FRAG_SHADER = "";
        constexpr const char *GOURAUD_VERT_SHADER = "";
        constexpr const char *PHONG_VERT_SHADER = "";
        constexpr const char *PHONG_FRAG_SHADER = "";
        constexpr const char *PHONGN_FRAG_SHADER = "";
        constexpr const char *SHADOW_DEPTH_VERT_SHADER = "";
        constexpr const char *SHADOW_DEPTH_FRAG_SHADER = "";
    }
#endif
