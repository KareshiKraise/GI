#pragma once
#include <GLFW/glfw3.h>

struct window {
	int init();
	void create_window(int w, int h);
	int should_close();
	void swap();
	void poll_ev();

	GLFWwindow *wnd;
};
