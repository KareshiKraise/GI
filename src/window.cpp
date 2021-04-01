#include "window.h"

#define KEYS 349
bool key_state[KEYS];
#define MBUTTON 8
bool mouse_state[MBUTTON];
double mouse_pos[2];

window w;

int window::init() {
	return glfwInit();
}

void resizefunc(GLFWwindow* wnd, int a, int b) {
	glViewport(0, 0, a, b);
	set_window_dim(&w, a, b);
}

//creates window and makes context current
int window::create_window(int w, int h, int vsync) {

	if (!init()) {
		std::cout << "failed to initialize glfw" << std::endl;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	this->wnd = glfwCreateWindow(w, h, "SSVP", NULL, NULL);
	Wid = w;
	Hei = h;
	if (!this->wnd) {
		glfwTerminate();
		std::cout << "could not create window" << std::endl;
		std::cin.get();
	}

	glfwMakeContextCurrent(this->wnd);

	auto err = glewInit();

	if (err != GLEW_OK) {
		std::cout << "failed to init glew" << std::endl;
		return -1;
	}
	std::cout << "glew initialized succesfully" << std::endl;

	glfwSwapInterval(vsync);
		
	glfwSetWindowSizeCallback(wnd, resizefunc);
	set_kb(kbfunc);
	set_mouse(mfunc);

	return 0;
}

void window::close() {
	return glfwTerminate(); 
}

void kbfunc(GLFWwindow* window, int key, int scan, int action, int mods)
{
	if (action == GLFW_PRESS)
		key_state[key] = true;
	else if (action == GLFW_RELEASE)
		key_state[key] = false;

	switch (key) {
	case GLFW_KEY_ESCAPE:
		if (action == GLFW_PRESS)
			glfwSetWindowShouldClose(window, true);
	}
}

void mfunc(GLFWwindow* window, double xpos, double ypos){
	int lstate = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
	int rstate = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT);
	if (lstate == GLFW_PRESS)
		mouse_state[GLFW_MOUSE_BUTTON_LEFT] = true;
	if (lstate == GLFW_RELEASE)
		mouse_state[GLFW_MOUSE_BUTTON_LEFT] = false;
	if (rstate == GLFW_PRESS)
		mouse_state[GLFW_MOUSE_BUTTON_RIGHT] = true;
	if (rstate == GLFW_RELEASE)
		mouse_state[GLFW_MOUSE_BUTTON_RIGHT] = false;

	mouse_pos[0] = xpos;
	mouse_pos[1] = ypos;

}


void set_window_dim(window* w, int a, int b)
{
	w->Wid = a;
	w->Hei = b;
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

//average fps over FRAME_AVG frames
double avg_fps(const frametime_data& dat)
{
	double sum = 0;
	for (int i = 0; i < FRAME_AVG; i++)
	{
		sum += dat.data[i];
	}
	sum /= FRAME_AVG;
	sum = 1000.0 / sum;
	return sum;
}

//average frametime over FRAME_AVG frames
double avg_ms(frametime_data& dat)
{
	double sum = 0;
	for (int i = 0; i < FRAME_AVG; i++)
	{
		sum += dat.data[i];
	}
	sum /= FRAME_AVG;
	return sum;
}

/* GUI STUFF*/
static void glfw_error_callback(int error, const char* description)
{
	fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

void init_gui(GLFWwindow* w) {
	glfwSetErrorCallback(glfw_error_callback);
	// Setup Dear ImGui context
	const char* glsl_version = "#version 430";
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGui_ImplGlfw_InitForOpenGL(w, true);
	ImGui_ImplOpenGL3_Init(glsl_version);
	// Setup Style
	ImGui::StyleColorsLight();
}
void render_gui(double delta, double fps) {
	// Start the Dear ImGui frame
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	//IMGUI scope
	{
		ImGui::Begin("CPU Timer");
		ImGui::Text("%f miliseconds per frame", delta);
		ImGui::Text("%f frames per second", fps);
		ImGui::End();
	}
	//IMGUI render
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
void destroy_gui() {
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}