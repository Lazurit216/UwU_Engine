#pragma once

#include "Core.h"
#include "Window.h"
#include "GameTimer.h"
#include "Renderer/IRenderer.h"
#include "Events/WindowEvents.h"
#include "Events/KeyboardEvents.h"
#include "Events/MouseEvents.h" 
#include "Input/InputManager.h"  
#include "GameState/GameStateManager.h"
#include "WindowManager.h"

#include "Assets/DX12Apps/DX12Triangle.h" 

namespace UwU_Engine
{
	struct WindowContext
	{
		InputManager input;
		Window* window = nullptr;           
		std::unique_ptr<IRenderer> renderer;       
		std::unique_ptr<DX12Triangle> triangle;           

		bool minimized = false;
		bool active = true;

		WindowContext() = default;
		WindowContext(const WindowContext&) = delete;            
		WindowContext& operator=(const WindowContext&) = delete;
		WindowContext(WindowContext&&) noexcept = default;   
		WindowContext& operator=(WindowContext&&) noexcept = default;
	};

	class UWU_API Application
	{
	public:
		Application();
		~Application();

		void Run();
	protected:
		virtual bool OnInit() { return true; }
		virtual void OnUpdate(float dt) {}
		virtual void OnShutdown() {}

		void RegisterContext(WindowContext&& ctx);
		WindowContext& GetContext(int idx) { return m_contexts[idx]; }
		void InitStateManager(std::shared_ptr<IGameState> initialState);

	private:
		void BindContextEvents(WindowContext& ctx, int idx);

		void OnEvent(Event& e, int idx);
		void OnWindowClose(WindowCloseEvent& e, int idx);
		void OnWindowMinimize(WindowMinimizeEvent& e, int idx);
		void OnWindowMaximize(WindowMaximizeEvent& e, int idx);
		void OnWindowRestore(WindowRestoreEvent& e, int idx);
		void OnWindowResize(WindowResizeEvent& e, int idx);

		void TickContext(WindowContext& ctx);
		void ShowStats();
	protected:
		WindowManager    m_windowManager;
		GameStateManager m_stateManager;
		GameTimer        m_timer;
		bool             m_isRunning = true;
	private:
		std::vector<WindowContext> m_contexts;

	};

	//To be defined in CLIENT
	Application* CreateApplication();

}