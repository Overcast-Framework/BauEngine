#pragma once
#ifndef RENDERER_H
#define RENDERER_H
#include <iostream>
#include <GLFW/glfw3.h>
#include <BauEngine/Graphics/Vertex.h>
#include <BauEngine/Graphics/Texture.h>

class Renderer
{
public:
	virtual void Initialize(GLFWwindow* window, glm::vec2 size) = 0;
	virtual void BeginFrame() = 0;
	virtual void EndFrame() = 0;
	virtual void RenderFrame() = 0;
	virtual void Destroy() = 0;
};
#endif