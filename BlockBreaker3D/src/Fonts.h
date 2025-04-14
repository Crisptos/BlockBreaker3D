#pragma once
#include <glm/glm.hpp>
#include <ft2build.h>
#include FT_FREETYPE_H 

namespace BB3D
{
    struct Glyph {
        unsigned int texture;
        glm::ivec2   size;       
        glm::ivec2   bearing;    
        unsigned int advance;    
    };

}