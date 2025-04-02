#pragma once
#include <SDL3/SDL.h>

namespace BB3D
{
	class Engine
	{
	public:
		void Init();
		void Run();
		void Destroy();

	private:
		void Input();

	private:

		// State
		bool m_IsRunning = true;
		bool m_IsIdle = false;

		// SDL Context
		SDL_Window* m_Window;
		SDL_GPUDevice* m_Device;

	};
}