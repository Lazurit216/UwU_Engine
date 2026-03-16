#pragma once

#include "Core.h"

namespace UwU_Engine
{
	class UWU_API Application
	{
	public:
		Application();
		~Application();

		void Run();
	};

	//To be defined in CLIENT
	Application* CreateApplication();

}