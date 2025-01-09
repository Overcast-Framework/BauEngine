#pragma once
#include <iostream>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include "Logger.h"
#include <BauEngine/Graphics/D3D12/D3D12Renderer.h>

enum class BEGraphicsAPI
{
	D3D12,
	Vulkan
};

class ApplicationSettings
{
public:
	std::string AppName = " ";
	std::string Title = " ";
	glm::vec2 Size = {0, 0};
	BEGraphicsAPI GraphicsAPI = BEGraphicsAPI::D3D12;
};

class Application
{
public:
	void Initialize(ApplicationSettings& settings);
	void Run();
	void Cleanup();
	void Exit();

	inline GLFWwindow* GetWindow() { return m_Window; }
	inline std::shared_ptr<Renderer> GetRenderer() { return m_Renderer; }
private:
	GLFWwindow* m_Window = nullptr;
	ApplicationSettings* m_Settings = nullptr;
	std::shared_ptr<Logger> m_Logger = nullptr;
	std::shared_ptr<Renderer> m_Renderer = nullptr;
};