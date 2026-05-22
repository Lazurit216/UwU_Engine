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

//#include "Assets/DX12Apps/DX12Triangle.h" 

namespace UwU_Engine
{
	class ImGuiLayer;

	struct WindowContext
	{
		InputManager input;
		Window* window = nullptr;           
		std::unique_ptr<IRenderer> renderer;            

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
		virtual void OnUpdate(float /*dt*/) {}
		virtual void OnFixedUpdate(float /*fixedDt*/) {}
		virtual void OnShutdown() {}

		// Called per active context, inside BeginFrame/EndFrame.
		// Use to draw context-specific geometry that isn't owned by a state.
		// idx 0 = primary, 1 = secondary.
		virtual void OnContextRender(int /*idx*/, IRenderer* /*r*/) {}

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

		void TickContext(WindowContext& ctx, int idx);
		void InitImGuiLayer();
		void ShutdownImGuiLayer();
		void ShowStats();
	protected:
		WindowManager    m_windowManager;
		GameStateManager m_stateManager;
		GameTimer        m_timer;
		bool             m_isRunning = true;
		float m_fixedStep = 1.f / 60.f;
	private:
		std::vector<WindowContext> m_contexts;
		float m_fixedAccumulator = 0.f;
		std::unique_ptr<ImGuiLayer> m_imguiLayer;

	};

	//To be defined in CLIENT
	Application* CreateApplication();

}
