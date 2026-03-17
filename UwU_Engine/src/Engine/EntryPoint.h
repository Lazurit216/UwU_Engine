#pragma once

#ifdef UWU_PLATFORM_WINDOWS

extern UwU_Engine::Application* UwU_Engine::CreateApplication();

int main(int argc, char** argv)
{
	UwU_Engine::Logger::Init();
	UWU_ENGINE_INFO("U a Bebra");
	auto app = UwU_Engine::CreateApplication();
	app->Run();
	delete app;
	UwU_Engine::Logger::Shutdown();
}

#endif