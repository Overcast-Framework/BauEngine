#include "bepch.h"
#include "Application.h"

void Application::Initialize(ApplicationSettings& settings)
{
	m_Settings = &settings;
	m_Logger = std::make_shared<Logger>();

	std::string apiName = m_Settings->GraphicsAPI == BEGraphicsAPI::D3D12 ? "Direct3D 12" : "Vulkan";

	m_Logger->Info
	(
		"Application initialization process started...\n\tApplication Name: " +
		m_Settings->AppName +
		"\n\tWindow Title: " +
		m_Settings->Title   +
		"\n\tWindow Size: "  +
		"[" + std::to_string((int)m_Settings->Size.x) + ", " + std::to_string((int)m_Settings->Size.y) + "]" +
		"\n\tGraphics API: " +
		apiName
	);

	if (!glfwInit())
	{
		m_Logger->CritErr("Could not initialize GLFW");
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	m_Window = glfwCreateWindow(m_Settings->Size.x, m_Settings->Size.y, m_Settings->Title.c_str(), nullptr, nullptr);

	if (m_Settings->GraphicsAPI == BEGraphicsAPI::D3D12)
	{
		m_Renderer = std::make_shared<D3D12Renderer>();
	}

	m_Renderer->Initialize(m_Window, m_Settings->Size);
}

void Application::Run()
{
	while (!glfwWindowShouldClose(m_Window))
	{
		glfwPollEvents();

		m_Renderer->BeginFrame();
		m_Renderer->RenderFrame();
		m_Renderer->EndFrame();
	}

	m_Renderer->Destroy();
}