// Shader constants header
// The actual shader content is generated at build time in shaders_generated.hpp

#pragma once

#ifdef SHADERS_GENERATED
    // Build time: use generated shader content
    #include "shaders_generated.hpp"
#else
    // Pre-build: provide placeholder declarations
    namespace shaders {
        constexpr const char* OCEAN_VERT_SHADER = "";
        constexpr const char* OCEAN_FRAG_SHADER = "";
        constexpr const char* IFFT_HORI_COMP_SHADER = "";
        constexpr const char* IFFT_VERT_COMP_SHADER = "";
        constexpr const char* WAVE_SPECTRUM_COMP_SHADER = "";
    }
#endif
