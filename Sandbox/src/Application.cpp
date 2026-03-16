#include "Application.h"

namespace UWU_Engine
{
	__declspec(dllimport) void Print();
}
int main()
{
	UWU_Engine::Print();
}