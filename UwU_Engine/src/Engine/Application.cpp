#include "uwupch.h"
#include "Application.h"
#include "GameState/States/LoadingState.h"
#include "GameState/States/MainMenuState.h"
#include "GameState/States/GameplayState.h"
#include "GameState/States/GamePauseState.h"
namespace UwU_Engine 
{
	Application::Application()
	{
		m_window = std::unique_ptr<Window>(Window::Create({ 1280, 720, "UwU Engine" }));

		m_window->SetEventCallback(std::bind(&Application::OnEvent, this, std::placeholders::_1));

		m_renderer = IRenderer::Create();
		m_renderer->Init(
			m_window->GetNativeHandle(),
			m_window->GetWidth(),
			m_window->GetHeight());

		// State chain: Loading (2s) - MainMenu (3s auto) - Gameplay - GamePause
		auto makePause = []() -> std::shared_ptr<IGameState>
			{
				return std::make_shared<GamePauseState>();
			};

		auto makeGameplay = [makePause]() -> std::shared_ptr<IGameState>
			{
				return std::make_shared<GameplayState>(makePause);  // inject factory
			};

		auto makeMainMenu = [makeGameplay]() -> std::shared_ptr<IGameState>
			{
				return std::make_shared<MainMenuState>(makeGameplay);
			};

		StateContext ctx{ m_renderer.get() };
		m_stateManager.Init(
			std::make_shared<LoadingState>(makeMainMenu, 2.0f), ctx);
	}

	Application::~Application()
	{
		if (m_renderer)
			m_renderer->Shutdown();
	}

	void Application::Run()
	{
		UWU_ENGINE_INFO("Application is running...");
		m_timer.Reset();

		while (m_isRunning) 
		{
			m_timer.Tick();
			m_window->PollEvents();

			if (m_minimized) continue;
			if (!m_renderer->IsReady()) continue;

			float t = static_cast<float>(m_timer.TotalTime());
			float r = (sinf(t * 2.0f) + 1.0f) * 0.5f;
			float g = (sinf(t * 2.0f + 2.09f) + 1.0f) * 0.5f; 
			float b = (sinf(t * 2.0f + 4.19f) + 1.0f) * 0.5f; 
			m_renderer->SetClearColor(r * 0.3f, g * 0.3f, b * 0.3f);

			float dt = m_timer.DeltaTime();

			// Build context fresh each frame — renderer pointer may be
			// reseated after a future hot-reload or device reset.
			StateContext ctx{ m_renderer.get() };

			// Update states — returns false if stack is empty → quit
			if (!m_stateManager.Update(ctx, dt))
			{
				m_isRunning = false;
				break;
			}

			m_renderer->BeginFrame();
			m_stateManager.Render(ctx);
			m_renderer->EndFrame();

			ShowStats();
		}

		UWU_ENGINE_INFO("Cycle end");
	}

	void Application::OnEvent(Event& e)
	{
		UWU_ENGINE_TRACE("Event: {}", e.GetName());

		EventDispatcher d(e);
		d.Dispatch<WindowCloseEvent>(std::bind(&Application::OnWindowClose, this, std::placeholders::_1), EventType::WindowClose);
		d.Dispatch<WindowMinimizeEvent>(std::bind(&Application::OnWindowMinimize, this, std::placeholders::_1), EventType::WindowMinimize);
		d.Dispatch<WindowMaximizeEvent>(std::bind(&Application::OnWindowMaximize, this, std::placeholders::_1), EventType::WindowMaximize);
		d.Dispatch<WindowRestoreEvent>(std::bind(&Application::OnWindowRestore, this, std::placeholders::_1), EventType::WindowRestore);
		d.Dispatch<WindowResizeEvent>(std::bind(&Application::OnWindowResize, this, std::placeholders::_1), EventType::WindowResize);

		if (!e.Handled)
		{
			StateContext ctx{ m_renderer.get() };
			m_stateManager.OnEvent(ctx, e);
		}
	}

	void Application::OnWindowClose(WindowCloseEvent& e)
	{
		UWU_ENGINE_INFO("Window closed — stopping");
		m_isRunning = false;
	}

	void Application::OnWindowMinimize(WindowMinimizeEvent& e)
	{
		UWU_ENGINE_INFO("Window minimized — pausing render");
		m_minimized = true;
		m_timer.Pause();
	}

	void Application::OnWindowMaximize(WindowMaximizeEvent& e)
	{
		UWU_ENGINE_INFO("Window maximized");
		m_minimized = false;
		m_timer.Resume();
	}

	void Application::OnWindowRestore(WindowRestoreEvent& e)
	{
		UWU_ENGINE_INFO("Window restored");
		m_minimized = false;
		m_timer.Resume();
	}

	void Application::OnWindowResize(WindowResizeEvent& e)
	{
		UWU_ENGINE_INFO("Window resized -> {}x{}", e.GetWidth(), e.GetHeight());
		if (m_renderer)
			m_renderer->OnResize(e.GetWidth(), e.GetHeight());
	}

	void Application::ShowStats()
	{
		// Update title once per second to avoid spam
		static double timeAccum = 0.0;
		timeAccum += m_timer.DeltaTimeD();

		if (timeAccum >= 1.0)
		{
			float fps = m_timer.FPS();
			float dtMs = m_timer.DeltaTime() * 1000.0f;

			//UWU_ENGINE_TRACE(std::format("FPS: {} | dt: {} ms", fps, dtMs));

			std::string title = "UwU Engine  |  FPS: "
				+ std::to_string(static_cast<int>(fps))
				+ "  dt: "
				+ std::to_string(dtMs).substr(0, 5) + "ms";

			m_window->SetTitle(title);
			timeAccum = 0.0;
		}
	}
}