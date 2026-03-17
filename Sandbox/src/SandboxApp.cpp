#include <UwU.h>
class Sandbox : public UwU_Engine::Application
{
public:
	Sandbox() {}
	~Sandbox() {}
};

UwU_Engine::Application* UwU_Engine::CreateApplication()
{
	return new Sandbox();
}