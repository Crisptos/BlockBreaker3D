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

	void UI::PushTextToUIBuff(SDL_GPUDevice* device, SDL_GPUBuffer* ui_buff, UI_TextField text_field, FontAtlas& atlas)
	{
		// UI Vertices
		// X, Y, U, V, R, G, B, A
		std::vector<Vertex> vertices;
		vertices.reserve(6 * text_field.text.size());
		unsigned int text_advance = 0;

		// TODO | need to investigate further, 32 is the needed offset to have text quads centered at the top left corner. Not entirely sure why
		float baseline = text_field.pos.y + 32.0f;

		// For each char, add a quad with the correct precomputed UV's
		for (char& c : text_field.text)
		{ 
			Glyph c_props = atlas.glyph_metadata[c];

			// 8x8 Text Atlas
			// Atlas is from 32 - 90 ASCII
			// Char - 31 = Num in 1d

			float w = 64.0f;
			float h = 64.0f;

			float final_x = text_field.pos.x + text_advance;
			float final_y = baseline - c_props.bearing.y;

			int offset = (c - 32);

			float v_column = std::floorf(offset / 16.0f) * (1.0f / 16.0f);
			float u_row = (offset % 16) * (1.0f / 16.0f);
			

			vertices.push_back({ final_x + w, final_y,															// tr 0
								 u_row + 0.0625f, v_column,
								 text_field.color[0], text_field.color[1], text_field.color[2], text_field.color[3]
			});
			vertices.push_back({ final_x + w, final_y + h,														// br 1
								 u_row + 0.0625f, v_column + 0.0625f,
								 text_field.color[0], text_field.color[1], text_field.color[2], text_field.color[3]
			});
			vertices.push_back({ final_x, final_y,																// tl 3
								 u_row, v_column,
								 text_field.color[0], text_field.color[1], text_field.color[2], text_field.color[3]
			});										
			vertices.push_back({ final_x + w, final_y + h,														// br 1
								 u_row + 0.0625f, v_column + 0.0625f,
								 text_field.color[0], text_field.color[1], text_field.color[2], text_field.color[3]
			});
			vertices.push_back({ final_x, final_y + h,															// bl 2
								 u_row, v_column + 0.0625f,
								 text_field.color[0], text_field.color[1], text_field.color[2], text_field.color[3]
			});
			vertices.push_back({ final_x, final_y,																// tl 3
								 u_row, v_column,
								 text_field.color[0], text_field.color[1], text_field.color[2], text_field.color[3]
			});

			text_advance += c_props.advance;
		}

		SDL_GPUTransferBufferCreateInfo ui_transfer_create_info = {};
		ui_transfer_create_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
		ui_transfer_create_info.size = sizeof(Vertex) * vertices.size();
		SDL_GPUTransferBuffer* ui_trans_buff = SDL_CreateGPUTransferBuffer(device, &ui_transfer_create_info);

		void* ui_trans_ptr = SDL_MapGPUTransferBuffer(device, ui_trans_buff, false);

		std::memcpy(ui_trans_ptr, vertices.data(), sizeof(Vertex) * vertices.size());

		SDL_UnmapGPUTransferBuffer(device, ui_trans_buff);

		SDL_GPUCommandBuffer* ui_copy_cmd_buff = SDL_AcquireGPUCommandBuffer(device);
		SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(ui_copy_cmd_buff);

		SDL_GPUTransferBufferLocation ui_trans_location = {};
		ui_trans_location.transfer_buffer = ui_trans_buff;
		ui_trans_location.offset = 0;
		SDL_GPUBufferRegion ui_region = {};
		ui_region.buffer = ui_buff;
		ui_region.offset = frame_offset;
		ui_region.size = sizeof(Vertex) * vertices.size();

		SDL_UploadToGPUBuffer(copy_pass, &ui_trans_location, &ui_region, false);

		SDL_EndGPUCopyPass(copy_pass);
		SDL_ReleaseGPUTransferBuffer(device, ui_trans_buff);

		SDL_GPUFence* upload_fence = SDL_SubmitGPUCommandBufferAndAcquireFence(ui_copy_cmd_buff);
		if (!upload_fence)
		{
			SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Failed to submit copy command buffer to GPU and acquire fence: %s\n", SDL_GetError());
			std::abort();
		}

		SDL_WaitForGPUFences(device, true, &upload_fence, 1);
		SDL_ReleaseGPUFence(device, upload_fence);

		frame_offset += sizeof(Vertex) * vertices.size();
	}

	void UI::PushElementToUIBuff(SDL_GPUDevice* device, SDL_GPUBuffer* ui_buff, UI_Element elem)
	{
		// UI Vertices
		// X, Y, U, V, R, G, B, A
		Vertex vertices[6] = {
			{elem.pos.x + elem.width, elem.pos.y, 1.0, 0.0, elem.color[0], elem.color[1], elem.color[2], elem.color[3]},				// tr 0
			{elem.pos.x + elem.width, elem.pos.y + elem.height, 1.0, 1.0, elem.color[0], elem.color[1], elem.color[2], elem.color[3]},  // br 1
			{elem.pos.x, elem.pos.y, 0.0, 0.0, elem.color[0], elem.color[1], elem.color[2], elem.color[3]},								// tl 3
			{elem.pos.x + elem.width, elem.pos.y + elem.height, 1.0, 1.0, elem.color[0], elem.color[1], elem.color[2], elem.color[3]},  // br 1
			{elem.pos.x, elem.pos.y + elem.height, 0.0, 1.0, elem.color[0], elem.color[1], elem.color[2], elem.color[3]},				// bl 2
			{elem.pos.x, elem.pos.y, 0.0, 0.0, elem.color[0], elem.color[1], elem.color[2], elem.color[3]},								// tl 3
		};

		SDL_GPUTransferBufferCreateInfo ui_transfer_create_info = {};
		ui_transfer_create_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
		ui_transfer_create_info.size = sizeof(vertices);
		SDL_GPUTransferBuffer* ui_trans_buff = SDL_CreateGPUTransferBuffer(device, &ui_transfer_create_info);

		void* ui_trans_ptr = SDL_MapGPUTransferBuffer(device, ui_trans_buff, false);

		std::memcpy(ui_trans_ptr, vertices, sizeof(vertices));

		SDL_UnmapGPUTransferBuffer(device, ui_trans_buff);

		SDL_GPUCommandBuffer* ui_copy_cmd_buff = SDL_AcquireGPUCommandBuffer(device);
		SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(ui_copy_cmd_buff);

		SDL_GPUTransferBufferLocation ui_trans_location = {};
		ui_trans_location.transfer_buffer = ui_trans_buff;
		ui_trans_location.offset = 0;
		SDL_GPUBufferRegion ui_region = {};
		ui_region.buffer = ui_buff;
		ui_region.offset = frame_offset;
		ui_region.size = sizeof(vertices);

		SDL_UploadToGPUBuffer(copy_pass, &ui_trans_location, &ui_region, false);

		SDL_EndGPUCopyPass(copy_pass);
		SDL_ReleaseGPUTransferBuffer(device, ui_trans_buff);

		SDL_GPUFence* upload_fence = SDL_SubmitGPUCommandBufferAndAcquireFence(ui_copy_cmd_buff);
		if(!upload_fence)
		{
			SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Failed to submit copy command buffer to GPU and acquire fence: %s\n", SDL_GetError());
			std::abort();
		}

		SDL_WaitForGPUFences(device, true, &upload_fence, 1);
		SDL_ReleaseGPUFence(device, upload_fence);

		frame_offset += sizeof(vertices);
	}

	void UI::FlushUIBuff(SDL_GPUDevice* device)
	{
		frame_offset = 0;
	}
}