#pragma once
#include <GLFW/glfw3.h>

constexpr uint32_t k_screenWidth = 800;
constexpr uint32_t k_screenHeight = 600;

class Window
{
public:
	Window() : pWindow(nullptr)
	{
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		pWindow = glfwCreateWindow(k_screenWidth, k_screenHeight, "WebGpu", nullptr, nullptr);
	}

	Window(Window const&) = delete;
	Window& operator=(Window const&) = delete;

	~Window()
	{
		if (pWindow) glfwDestroyWindow(pWindow);
	}

	inline bool ShouldClose() { return glfwWindowShouldClose(pWindow); }

	inline GLFWwindow* get() { return pWindow; }

private:
	GLFWwindow* pWindow;
};

class Renderer {
public: 
	Renderer();
	~Renderer();


private:

	void InitializeDevice();


	Window window;
};