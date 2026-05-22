#include "uwupch.h"
#include "Application.h"
#include "GameState/States/LoadingState.h"
#include "GameState/States/MainMenuState.h"
#include "GameState/States/GameplayState.h"
#include "GameState/States/GamePauseState.h"
#include "Renderer/DirectX12/DX12Renderer.h" 
namespace UwU_Engine 
{
	Application::Application() = default;

	Application::~Application()
	{	}

	void Application::Run()
	{
		UWU_ENGINE_INFO("Application is running...");
		if (!OnInit())
		{
			UWU_ENGINE_ERROR("OnInit() failed - aborting");
			return;
		}

		if (m_contexts.empty())
		{
			UWU_ENGINE_ERROR("No WindowContexts registered - aborting");
			return;
		}

		UWU_ENGINE_INFO("Application is running ({} window(s))", m_contexts.size());
		m_timer.Reset();

		while (m_isRunning)
		{
			for (auto& ctx : m_contexts)
				ctx.input.BeginFrame();
			m_windowManager.PollAll();

			// Variable timestep
			m_timer.Tick();
			const float dt = m_timer.DeltaTime();

			// Fixed timestep accumulator
			m_fixedAccumulator += dt;
			while (m_fixedAccumulator >= m_fixedStep)
			{
				OnFixedUpdate(m_fixedStep);
				m_fixedAccumulator -= m_fixedStep;
			}

			OnUpdate(dt);

			if (!m_contexts.empty() && m_contexts[0].active)
			{
				StateContext sCtx{ m_contexts[0].renderer.get(), &m_contexts[0].input };
				if (!m_stateManager.Update(sCtx, dt))
				{
					m_isRunning = false;
					break;
				}
			}

			for (int i = 0; i < static_cast<int>(m_contexts.size()); ++i)
				TickContext(m_contexts[i], i);

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
		UWU_ENGINE_INFO("Context {} registered", idx);
	}

	void Application::InitStateManager(std::shared_ptr<IGameState> initialState)
	{
		if (m_contexts.empty())
		{
			UWU_ENGINE_ERROR("InitStateManager: no contexts registered yet");
			return;
		}
		StateContext ctx{ m_contexts[0].renderer.get(), &m_contexts[0].input };
		m_stateManager.Init(std::move(initialState), ctx);
	}

	void Application::BindContextEvents(WindowContext& ctx, int idx)
	{
		ctx.window->SetEventCallback([this, idx](Event& e)
			{
				OnEvent(e, idx);
			});
	}

	void Application::TickContext(WindowContext& ctx, int idx)
	{
		if (!ctx.active || ctx.minimized)              return;
		if (!ctx.renderer || !ctx.renderer->IsReady()) return;

		ctx.renderer->BeginFrame();

		// State system renders only on the primary context
		if (idx == 0)
		{
			StateContext sCtx{ ctx.renderer.get(), &ctx.input };
			m_stateManager.Render(sCtx);
		}

		// Per-context custom geometry hook (secondary triangle, UI, etc.)
		OnContextRender(idx, ctx.renderer.get());

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

		m_contexts[idx].input.OnEvent(e);

		if (!e.Handled && idx == 0 && !m_contexts.empty())
		{
			StateContext sCtx{ m_contexts[0].renderer.get(), &m_contexts[0].input };
			m_stateManager.OnEvent(sCtx, e);
		}
	}

	void Application::OnWindowClose(WindowCloseEvent& e, int idx)
	{
		UWU_ENGINE_INFO("[App] Window {} closed", idx);
		m_contexts[idx].active = false;

		if (idx == 0)
		{
			m_isRunning = false;
			UWU_ENGINE_INFO("[App] Primary window closed - shutting down");
		}
	}

	void Application::OnWindowMinimize(WindowMinimizeEvent& e, int idx)
	{
		m_contexts[idx].minimized = true;
		UWU_ENGINE_INFO("[App] Window {} minimized", idx);
	}

	void Application::OnWindowMaximize(WindowMaximizeEvent& e, int idx)
	{
		m_contexts[idx].minimized = false;
	}

	void Application::OnWindowRestore(WindowRestoreEvent& e, int idx)
	{
		m_contexts[idx].minimized = false;
	}

	void Application::OnWindowResize(WindowResizeEvent& e, int idx)
	{
		UWU_ENGINE_INFO("[App] Window {} resized - {}x{}", idx, e.GetWidth(), e.GetHeight());
		if (m_contexts[idx].renderer)
			m_contexts[idx].renderer->OnResize(e.GetWidth(), e.GetHeight());
	}

	void Application::ShowStats()
	{
		static double timeAccum = 0.0;
		timeAccum += m_timer.DeltaTimeD();

		if (timeAccum >= 1.0)
		{
			float fps = m_timer.FPS();
			float dtMs = m_timer.DeltaTime() * 1000.0f;

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
