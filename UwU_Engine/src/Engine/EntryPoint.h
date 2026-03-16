#pragma once

#ifdef UWU_PLATFORM_WINDOWS

extern UwU_Engine::Application* UwU_Engine::CreateApplication();

int main(int argc, char** argv)
{
	printf("UwU Engine is running!\n");
	auto app = UwU_Engine::CreateApplication();
	app->Run();
	delete app;
}

#endif