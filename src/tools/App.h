#pragma once

#include <stdio.h>
#include <map>
#include <functional>

#include <windows.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

	class App {


		typedef std::function<void(int, int)> keyfunc;
		typedef std::pair<int, keyfunc> keyAction;
		std::map<int, keyfunc> inputs;

		void Keycallback(int, int, int, int);

		void ScrollCallback(double xoffset, double yoffset) {
			scrolloffset = yoffset;
		}

		static void onKey(GLFWwindow* window, int key, int scancode, int actions, int mods) {
			App* obj = static_cast<App*>(glfwGetWindowUserPointer(window));
			obj->Keycallback(key, scancode, actions, mods);
		}

		static void ScrollCallback(GLFWwindow* window,double xoffset, double yoffset) {
			App* obj = static_cast<App*>(glfwGetWindowUserPointer(window));
			obj->ScrollCallback(xoffset, yoffset);
		}
	protected:
		GLFWwindow *window;
		float ticks = 0;
		int fps = 0;
		bool running = true;
		int width, height;

		double scrolloffset;

		void viewportinit(GLFWwindow *window);

		virtual void init() = 0;
		virtual void initGL() = 0;
		virtual void onClose() = 0;

		virtual void update(float delta) = 0;
		virtual void render(float delta) = 0;
		virtual void inputListener(float delta) = 0;
	public:

		App();
		App(int w, int h, const char *title);
		~App();


		void addInput(int, keyfunc);

		void mainLoop();

		void setSize() {}
		void setPosition() {}

		void start();

	};
/*

* App:
*	-render
*	-init
*	-setIcon
*	-fps_utils
*	-
* 
* 
*	
* 
* */
