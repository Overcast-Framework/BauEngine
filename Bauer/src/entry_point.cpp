#include <BauEngine/BauEngine.h>

int main(void)
{
	ApplicationSettings settings { "Bauer", "Bauer v1.0-dev", {1280, 720}, BEGraphicsAPI::D3D12 };

	Application app;
	app.Initialize(settings);
	app.Run();

	return 0;
}