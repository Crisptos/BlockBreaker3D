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

	void DestroyFreeType()
	{
		if (FT_Done_FreeType(ft) != 0)
		{
			SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Failed destroy FreeType\n");
			std::abort();
		}
	}

}