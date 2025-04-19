#include "Engine.h"

#define FONT_CELL_SIZE 64

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

	FontAtlas CreateFontAtlasFromFile(SDL_GPUDevice* device, const char* filepath)
	{
		FontAtlas new_atlas = {};
		new_atlas.glyph_metadata.reserve(126);

		std::vector<unsigned char> atlas_buffer((FONT_CELL_SIZE * FONT_CELL_SIZE) * 64); // 8 x 8 Font Cell Size

		FT_Face face;
		FT_Error err = FT_New_Face(ft, filepath, 0, &face);
		if (err)
		{
			SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Failed to load font file with FreeType: %s\n", FT_Error_String(err));
			std::abort();
		}
		FT_Set_Pixel_Sizes(face, 0, 48);

		int glyph_count = 0;

		for (unsigned char c = 32; c < 91; c++)
		{
			err = FT_Load_Char(face, c, FT_LOAD_RENDER);
			if (err)
			{
				SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Failed to extract glyph from face: %s\n", FT_Error_String(err));
				std::abort();
			}

			size_t col = glyph_count % 8;
			size_t row = glyph_count / 8;

			size_t x_offset = col * FONT_CELL_SIZE;
			size_t y_offset = row * FONT_CELL_SIZE;

			FT_Bitmap& bmp = face->glyph->bitmap;
			
			for (int y = 0; y < bmp.rows; y++)
			{
				size_t dst_y = y_offset + y;
				size_t dst_offset = dst_y * 512 + x_offset;

				std::memcpy(&atlas_buffer[dst_offset], &bmp.buffer[y * bmp.pitch], sizeof(unsigned char) * bmp.width);
			}

			glyph_count++;
		}

		FT_Done_Face(face);

		SDL_GPUTexture* new_atlas_tex = CreateAndLoadFontAtlasTextureToGPU(device, atlas_buffer.data(), {FONT_CELL_SIZE * 8, FONT_CELL_SIZE * 8, 1});

		new_atlas.atlas_texture = new_atlas_tex;

		return new_atlas;
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