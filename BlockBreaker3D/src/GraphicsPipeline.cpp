#pragma once
#include "Engine.h"
#include <iostream>

namespace BB3D
{
	SDL_GPUGraphicsPipeline* CreateGraphicsPipelineForSkybox(SDL_GPUDevice* device, SDL_GPUTextureFormat color_target_format, SDL_GPUShader* vert_shader, SDL_GPUShader* frag_shader)
	{
		SDL_GPUGraphicsPipeline* new_pipeline = {};

		SDL_GPUColorTargetDescription color_target_dscr = {};
		color_target_dscr.format = color_target_format;

		SDL_GPUGraphicsPipelineTargetInfo target_info_pipeline = {};
		target_info_pipeline.num_color_targets = 1;
		target_info_pipeline.color_target_descriptions = &color_target_dscr;

		SDL_GPUGraphicsPipelineCreateInfo create_info_pipeline = {};
		create_info_pipeline.vertex_shader = vert_shader;
		create_info_pipeline.fragment_shader = frag_shader;
		create_info_pipeline.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
		create_info_pipeline.target_info = target_info_pipeline;

		new_pipeline = SDL_CreateGPUGraphicsPipeline(device, &create_info_pipeline);
		if (!new_pipeline)
		{
			SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Failed to create GPU pipeline for skybox: %s\n", SDL_GetError());
			std::abort();
		}

		return new_pipeline;
	}

	SDL_GPUGraphicsPipeline* CreateGraphicsPipelineForModels(SDL_GPUDevice* device, SDL_GPUTextureFormat color_target_format, SDL_GPUShader* vert_shader, SDL_GPUShader* frag_shader)
	{
		SDL_GPUGraphicsPipeline* new_pipeline = {};

		SDL_GPUColorTargetDescription color_target_dscr = {};
		color_target_dscr.format = color_target_format;

		SDL_GPUGraphicsPipelineTargetInfo target_info_pipeline = {};
		target_info_pipeline.num_color_targets = 1;
		target_info_pipeline.color_target_descriptions = &color_target_dscr;
		target_info_pipeline.has_depth_stencil_target = true;
		target_info_pipeline.depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D24_UNORM;

		SDL_GPUVertexAttribute attribs[3] = {};
		attribs[0].location = 0;
		attribs[0].buffer_slot = 0;
		attribs[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
		attribs[0].offset = 0;
		attribs[1].location = 1;
		attribs[1].buffer_slot = 0;
		attribs[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
		attribs[1].offset = sizeof(float) * 3;
		attribs[2].location = 2;
		attribs[2].buffer_slot = 0;
		attribs[2].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
		attribs[2].offset = sizeof(float) * 6;

		SDL_GPUVertexBufferDescription vbo_descr = {};
		vbo_descr.slot = 0;
		vbo_descr.pitch = sizeof(Vertex);
		vbo_descr.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
		vbo_descr.instance_step_rate = 0;

		SDL_GPUVertexInputState vert_input_state = {};
		vert_input_state.num_vertex_attributes = 3;
		vert_input_state.vertex_attributes = attribs;
		vert_input_state.vertex_buffer_descriptions = &vbo_descr;
		vert_input_state.num_vertex_buffers = 1; // number of descriptions, not number of existing vbo's on the GPU

		SDL_GPUDepthStencilState depth_state = {};
		depth_state.enable_depth_test = true;
		depth_state.enable_depth_write = true;
		depth_state.compare_op = SDL_GPU_COMPAREOP_LESS;

		SDL_GPUGraphicsPipelineCreateInfo create_info_pipeline = {};
		create_info_pipeline.vertex_shader = vert_shader;
		create_info_pipeline.fragment_shader = frag_shader;
		create_info_pipeline.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
		create_info_pipeline.target_info = target_info_pipeline;
		create_info_pipeline.vertex_input_state = vert_input_state;
		create_info_pipeline.depth_stencil_state = depth_state;

		new_pipeline = SDL_CreateGPUGraphicsPipeline(device, &create_info_pipeline);
		if (!new_pipeline)
		{
			SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Failed to create GPU pipeline for models: %s\n", SDL_GetError());
			std::abort();
		}

		return new_pipeline;
	}

	SDL_GPUGraphicsPipeline* CreateGraphicsPipelineForUI(SDL_GPUDevice* device, SDL_GPUTextureFormat color_target_format, SDL_GPUShader* vert_shader, SDL_GPUShader* frag_shader)
	{
		SDL_GPUGraphicsPipeline* new_pipeline = {};

		SDL_GPUColorTargetDescription color_target_dscr = {};
		color_target_dscr.format = color_target_format;

		SDL_GPUGraphicsPipelineTargetInfo target_info_pipeline = {};
		target_info_pipeline.num_color_targets = 1;
		target_info_pipeline.color_target_descriptions = &color_target_dscr;
		target_info_pipeline.has_depth_stencil_target = true;
		target_info_pipeline.depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D24_UNORM;

		SDL_GPUVertexAttribute attribs[3] = {};
		attribs[0].location = 0;
		attribs[0].buffer_slot = 0;
		attribs[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
		attribs[0].offset = 0;
		attribs[1].location = 1;
		attribs[1].buffer_slot = 0;
		attribs[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
		attribs[1].offset = sizeof(float) * 2;
		attribs[2].location = 2;
		attribs[2].buffer_slot = 0;
		attribs[2].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4;
		attribs[2].offset = sizeof(float) * 4;

		SDL_GPUVertexBufferDescription vbo_descr = {};
		vbo_descr.slot = 0;
		vbo_descr.pitch = sizeof(Vertex);
		vbo_descr.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
		vbo_descr.instance_step_rate = 0;

		SDL_GPUVertexInputState vert_input_state = {};
		vert_input_state.num_vertex_attributes = 3;
		vert_input_state.vertex_attributes = attribs;
		vert_input_state.vertex_buffer_descriptions = &vbo_descr;
		vert_input_state.num_vertex_buffers = 1; // number of descriptions, not number of existing vbo's on the GPU

		SDL_GPUGraphicsPipelineCreateInfo create_info_pipeline = {};
		create_info_pipeline.vertex_shader = vert_shader;
		create_info_pipeline.fragment_shader = frag_shader;
		create_info_pipeline.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
		create_info_pipeline.target_info = target_info_pipeline;
		create_info_pipeline.vertex_input_state = vert_input_state;

		new_pipeline = SDL_CreateGPUGraphicsPipeline(device, &create_info_pipeline);
		if (!new_pipeline)
		{
			SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Failed to create GPU pipeline for UI layer: %s\n", SDL_GetError());
			std::abort();
		}

		return new_pipeline;

	}

}