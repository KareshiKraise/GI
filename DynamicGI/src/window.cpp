#include "window.h"
#include <iostream>


int window::init() {
	return glfwInit();
}

//creates window and makes context current
void window::create_window(int w, int h) {

	if (!init()) {
		std::cout << "failed to initialize glfw" << std::endl;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);

	this->wnd = glfwCreateWindow(w, h, "DynamicGI", NULL, NULL);
	Wid = w;
	Hei = h;
	if (!this->wnd) {
		glfwTerminate();
		std::cout << "could not create window" << std::endl;
		std::cin.get();
	}

	glfwMakeContextCurrent(this->wnd);
}

int window::should_close() {
	return glfwWindowShouldClose(this->wnd);
}

void window::swap() {
	glfwSwapBuffers(this->wnd);
}

void window::poll_ev() {
	glfwPollEvents();
}

void window::set_kb(key_func func) {
	glfwSetKeyCallback(wnd, func);
}

void window::set_mouse(mouse_func func) {
	glfwSetCursorPosCallback(wnd, func);
}
