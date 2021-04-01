#pragma once
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include "IMGUI/imgui.h"
#include "IMGUI/imgui_impl_glfw.h"
#include "IMGUI/imgui_impl_opengl3.h"

typedef void(*key_func)(GLFWwindow* window, int key, int scan, int action, int mods);
typedef void(*mouse_func)(GLFWwindow *window, double xpos, double ypos);

#define KEYS 349
extern bool key_state[KEYS];
#define MBUTTON 8
extern bool mouse_state[MBUTTON];

extern double mouse_pos[2];

struct window {
	int init();
	int create_window(int w, int h, int vsync);
	int should_close();
	void swap();
	void poll_ev();
	void close();
	void set_kb(key_func);
	void set_mouse(mouse_func);

	GLFWwindow *wnd;

	unsigned int Wid;
	unsigned int Hei;
};


void kbfunc(GLFWwindow* window, int key, int scan, int action, int mods);
void mfunc(GLFWwindow* window, double xpos, double ypos);
void set_window_dim(window* w, int a, int b);

//struct for storing frametime data
#define FRAME_AVG 128
struct frametime_data
{
	int frame_count;
	double data[FRAME_AVG];
};

static void glfw_error_callback(int error, const char* description);
void init_gui(GLFWwindow* w);
void render_gui(double delta, double fps);
void destroy_gui();
double avg_fps(const frametime_data& dat);
double avg_ms(frametime_data& dat);



