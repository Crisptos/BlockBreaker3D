#include "Engine.h"

namespace BB3D
{
	FT_Library ft;

	void InitFreeType()
	{
		if (FT_Init_FreeType(&ft) != 0)
		{
			SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Failed initialize FreeType\n");
			std::abort();
		}
	}

	SDL_GPUTexture* CreateFontAtlasFromFile(SDL_GPUDevice* device, const char* filepath)
	{
		// TODO test a few glyphs
		std::vector<unsigned char> atlas_buffer(64*64);

		FT_Face face;
		FT_Error err = FT_New_Face(ft, filepath, 0, &face);
		if (err)
		{
			SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Failed to load font file with FreeType: %s\n", FT_Error_String(err));
			std::abort();
		}
		FT_Set_Pixel_Sizes(face, 0, 64);

		err = FT_Load_Char(face, 'A', FT_LOAD_RENDER);
		if (err)
		{
			SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Failed to extract glyph from face: %s\n", FT_Error_String(err));
			std::abort();
		}



		SDL_GPUTexture* new_atlas_tex = CreateAndLoadFontAtlasTextureToGPU(device, atlas_buffer.data());
		return new_atlas_tex;
	}

	void DestroyFreeType()
	{
		if (FT_Done_FreeType(ft) != 0)
		{
			SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Failed destroy FreeType\n");
			std::abort();
		}
	}

}