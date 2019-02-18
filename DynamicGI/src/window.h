#pragma once
#include <GL/glew.h>
#include <GLFW/glfw3.h>

typedef void(*key_func)(GLFWwindow* window, int key, int scan, int action, int mods);
typedef void(*mouse_func)(GLFWwindow *window, double xpos, double ypos);

struct window {
	int init();
	int create_window(int w, int h);
	int should_close();
	void swap();
	void poll_ev();

	void set_kb(key_func);
	void set_mouse(mouse_func);

	GLFWwindow *wnd;

	unsigned int Wid;
	unsigned int Hei;
};
