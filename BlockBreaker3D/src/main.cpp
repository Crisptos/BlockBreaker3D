#include "Engine.h"

int main(int argc, char** argv)
{
	BB3D::Engine engine;
	engine.Init();
	engine.Run();
	engine.Destroy();
	return 0;
}