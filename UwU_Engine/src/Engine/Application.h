#pragma once

#include "Core.h"
#include "Window.h"
#include "GameTimer.h"
#include "Renderer/IRenderer.h"
#include "Events/WindowEvents.h"
#include "GameState/GameStateManager.h" 

namespace UwU_Engine
{
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

		void ShowStats();

	private:
		bool m_isRunning = true;
		bool m_minimized = false;
		std::unique_ptr<Window> m_window;
		std::unique_ptr<IRenderer> m_renderer;
		GameTimer m_timer;
		GameStateManager m_stateManager;

	};

	//To be defined in CLIENT
	Application* CreateApplication();

}