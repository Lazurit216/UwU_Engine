#include "Application.h"
#include "uwupch.h"
#include "Platforms/Windows/Win32Window.h"

namespace UwU_Engine 
{
	Application::Application()
	{

	}

	Application::~Application()
	{

	}

	void Application::Run()
	{
		UWU_ENGINE_INFO("Application is running...");

		auto window = Window::Create({ 1280, 720, "UwU Engine" });
		auto logWindow = Window::Create({ 720, 480, "Logs" });

		while (!window->ShouldClose()) {
			window->PollEvents();
			if (!logWindow->ShouldClose())
				logWindow->PollEvents();
		}

		UWU_ENGINE_INFO("Cycle end");
	}
}