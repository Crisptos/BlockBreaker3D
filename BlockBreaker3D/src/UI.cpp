#include "Engine.h"

#define UI_QUAD_LIMIT_300 57600 // 300 quads * 6 vertices = 1800 vertices | 1800 vertices * 32 bytes = 57600

namespace BB3D
{
	SDL_GPUBuffer* CreateUILayerBuffer(SDL_GPUDevice* device)
	{
		SDL_GPUBuffer* new_ui_buff = {};

		SDL_GPUBufferCreateInfo ui_buff_info = {};
		ui_buff_info.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
		ui_buff_info.size = UI_QUAD_LIMIT_300;
		new_ui_buff = SDL_CreateGPUBuffer(device, &ui_buff_info);

		return new_ui_buff;
	}

	void UI::PushElementToUIBuff(SDL_GPUDevice* device, SDL_GPUBuffer* ui_buff, UI_Element& elem)
	{

	}
}