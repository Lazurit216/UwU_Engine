#include "uwupch.h"
#include "Application.h"
#include "GameState/States/LoadingState.h"
#include "GameState/States/MainMenuState.h"
#include "GameState/States/GameplayState.h"
#include "GameState/States/GamePauseState.h"
#include "Renderer/DirectX12/DX12Renderer.h" 
namespace UwU_Engine 
{
	static DX12Renderer* AsDX12(IRenderer* r)
	{
		return static_cast<DX12Renderer*>(r);
	}

	Application::Application() = default;

	Application::~Application()
	{	}

	void Application::Run()
	{
		UWU_ENGINE_INFO("Application is running...");
		if (!OnInit())
		{
			UWU_ENGINE_ERROR("OnInit() failed — aborting");
			return;
		}

		if (m_contexts.empty())
		{
			UWU_ENGINE_ERROR("No WindowContexts registered — aborting");
			return;
		}

		UWU_ENGINE_INFO("Application is running ({} window(s))", m_contexts.size());
		m_timer.Reset();

		while (m_isRunning)
		{
			m_windowManager.PollAll();
			m_timer.Tick();
			float dt = m_timer.DeltaTime();

			OnUpdate(dt);

			if (!m_contexts.empty() && m_contexts[0].active)
			{
				StateContext sCtx{ m_contexts[0].renderer.get() };
				if (!m_stateManager.Update(sCtx, dt))
				{
					m_isRunning = false;
					break;
				}
			}

			for (auto& ctx : m_contexts)
				TickContext(ctx);

			ShowStats();
		}

		OnShutdown();
		UWU_ENGINE_INFO("=== Application shut down ===");
	}

	void Application::RegisterContext(WindowContext&& ctx)
	{
		int idx = static_cast<int>(m_contexts.size());
		m_contexts.emplace_back(std::move(ctx));// push_back(std::move(ctx));
		BindContextEvents(m_contexts.back(), idx);
		UWU_ENGINE_INFO("[App] Context {} registered", idx);
	}

	void Application::InitStateManager(std::shared_ptr<IGameState> initialState)
	{
		if (m_contexts.empty())
		{
			UWU_ENGINE_ERROR("[App] InitStateManager: no contexts registered yet");
			return;
		}
		StateContext ctx{ m_contexts[0].renderer.get() };
		m_stateManager.Init(std::move(initialState), ctx);
	}

	void Application::BindContextEvents(WindowContext& ctx, int idx)
	{
		ctx.window->SetEventCallback([this, idx](Event& e)
			{
				OnEvent(e, idx);
			});
	}

	void Application::TickContext(WindowContext& ctx)
	{
		if (!ctx.active || ctx.minimized)    return;
		if (!ctx.renderer || !ctx.renderer->IsReady()) return;

		ctx.renderer->BeginFrame();

		if (ctx.triangle)
		{
			auto* dx12 = AsDX12(ctx.renderer.get());
			ctx.triangle->Draw(dx12->GetCommandList());
		}

		StateContext sCtx{ ctx.renderer.get() };
		m_stateManager.Render(sCtx);

		ctx.renderer->EndFrame();
	}

	void Application::OnEvent(Event& e, int idx)
	{
		UWU_ENGINE_TRACE("Event [{}]: {}", idx, e.GetName());

		EventDispatcher d(e);
		d.Dispatch<WindowCloseEvent>(std::bind(&Application::OnWindowClose, this, std::placeholders::_1, idx), EventType::WindowClose);
		d.Dispatch<WindowMinimizeEvent>(std::bind(&Application::OnWindowMinimize, this, std::placeholders::_1, idx), EventType::WindowMinimize);
		d.Dispatch<WindowMaximizeEvent>(std::bind(&Application::OnWindowMaximize, this, std::placeholders::_1, idx), EventType::WindowMaximize);
		d.Dispatch<WindowRestoreEvent>(std::bind(&Application::OnWindowRestore, this, std::placeholders::_1, idx), EventType::WindowRestore);
		d.Dispatch<WindowResizeEvent>(std::bind(&Application::OnWindowResize, this, std::placeholders::_1, idx), EventType::WindowResize);

		// Unhandled events go to the state manager (primary window only)
		if (!e.Handled && idx == 0 && !m_contexts.empty())
		{
			StateContext sCtx{ m_contexts[0].renderer.get() };
			m_stateManager.OnEvent(sCtx, e);
		}
	}

	void Application::OnWindowClose(WindowCloseEvent& e, int idx)
	{
		UWU_ENGINE_INFO("[App] Window {} closed", idx);
		m_contexts[idx].active = false;

		// Closing the primary window quits everything.
		// Secondary windows can be closed independently.
		if (idx == 0)
		{
			m_isRunning = false;
			UWU_ENGINE_INFO("[App] Primary window closed — shutting down");
		}
	}

	void Application::OnWindowMinimize(WindowMinimizeEvent& e, int idx)
	{
		m_contexts[idx].minimized = true;
		if (idx == 0) m_timer.Pause();
		UWU_ENGINE_INFO("[App] Window {} minimized", idx);
	}

	void Application::OnWindowMaximize(WindowMaximizeEvent& e, int idx)
	{
		m_contexts[idx].minimized = false;
		if (idx == 0) m_timer.Resume();
	}

	void Application::OnWindowRestore(WindowRestoreEvent& e, int idx)
	{
		m_contexts[idx].minimized = false;
		if (idx == 0) m_timer.Resume();
	}

	void Application::OnWindowResize(WindowResizeEvent& e, int idx)
	{
		UWU_ENGINE_INFO("[App] Window {} resized - {}x{}", idx, e.GetWidth(), e.GetHeight());
		if (m_contexts[idx].renderer)
			m_contexts[idx].renderer->OnResize(e.GetWidth(), e.GetHeight());
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

			std::string stats = " |  FPS: "
				+ std::to_string(static_cast<int>(fps))
				+ "  dt: "
				+ std::to_string(dtMs).substr(0, 5) + "ms";

			for (auto& ctx : m_contexts)
				if (ctx.active && ctx.window)
					ctx.window->SetTitle(ctx.window->GetOriginalTitle() + stats);
			timeAccum = 0.0;
		}
	}
}