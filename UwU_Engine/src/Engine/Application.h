#pragma once

#include "Core.h"
#include "Window.h"
#include "GameTimer.h"
#include "Renderer/IRenderer.h"
#include "Events/WindowEvents.h"
#include "GameState/GameStateManager.h" 

#include "Assets/DX12Apps/DX12Triangle.h" 

namespace UwU_Engine
{
	class DX12Triangle;
	struct WindowContext
	{
		std::unique_ptr<Window>        window;
		std::unique_ptr<IRenderer>     renderer;
		std::unique_ptr<DX12Triangle>  triangle;
		std::array<float, 4>           clearColor = { 0.1f, 0.1f, 0.15f, 1.0f };
		bool                           minimized = false;
		bool                           active = true;
	};

	class UWU_API Application
	{
	public:
		Application();
		~Application();

		void Run();

	private:
		void OnEvent(Event& e);

		void OnWindowClose(WindowCloseEvent& e);
		void OnWindowMinimize(WindowMinimizeEvent& e);
		void OnWindowMaximize(WindowMaximizeEvent& e);
		void OnWindowRestore(WindowRestoreEvent& e);
		void OnWindowResize(WindowResizeEvent& e);

		void InitContext(int idx, const Window::WindowProps& props,
			std::array<float, 4> clearColor,
			const TriangleDesc& triDesc);

		void TickContext(WindowContext& ctx, float dt);

		void ShowStats();

	private:
		bool m_isRunning = true;
		bool m_minimized = false;
		std::array<WindowContext, 2> m_contexts;
		std::unique_ptr<Window> m_window;
		std::unique_ptr<IRenderer> m_renderer;
		GameTimer m_timer;
		GameStateManager m_stateManager;

		std::unique_ptr<DX12Triangle> m_triangle;

	};

	//To be defined in CLIENT
	Application* CreateApplication();

}