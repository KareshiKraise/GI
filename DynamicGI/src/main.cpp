#include <iostream>
#include <string>

#include <GL/glew.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "window.h"
#include "GL_CALL.h"
#include "mesh_loader.h"
#include "camera.h"
#include "shader.h"
#include "Quad.h"
#include "framebuffer.h"
#include "Axis3D.h"

//scene and model paths
const char* sponza_path = "models/sponza/sponza.obj";

struct scene {
	mesh_loader *mesh;
	Shader *shader;
	Camera *camera;
	/*---pos---*/
	glm::vec3 bb_mid;
	/*---matrices---*/
	glm::mat4 view;
	glm::mat4 proj;
	glm::mat4 model;	
};

struct shadow_map {	
	glm::mat4 proj;
	glm::mat4 view;
	Shader *shader;
};

scene sponza;
shadow_map shadowmap;
quad screen_quad;

/*---camera controls---*/
double lastX = 0, lastY = 0;
bool active_cam = true;
bool render_from_light = false;
bool render_complete = true;
glm::vec3 ref_up;
glm::vec3 ref_front;
glm::vec3 ref_pos;
glm::vec3 lightPos;
glm::vec3 lightDir;

//keyboard function
void kbfunc(GLFWwindow* window, int key, int scan, int action, int mods) {
	if (key == GLFW_KEY_ESCAPE && (action == GLFW_PRESS || action == GLFW_REPEAT))
	{
		glfwSetWindowShouldClose(window, GLFW_TRUE);
	}

	if (key == GLFW_KEY_W && (action == GLFW_PRESS || action == GLFW_REPEAT))
	{
		sponza.camera->ProcessKeyboard(FORWARD, 1);
	}

	if (key == GLFW_KEY_A && (action == GLFW_PRESS || action == GLFW_REPEAT))
	{
		sponza.camera->ProcessKeyboard(LEFT, 1);
	}

	if (key == GLFW_KEY_S && (action == GLFW_PRESS || action == GLFW_REPEAT))
	{
		sponza.camera->ProcessKeyboard(BACKWARD, 1);
	}

	if (key == GLFW_KEY_D && (action == GLFW_PRESS || action == GLFW_REPEAT))
	{
		sponza.camera->ProcessKeyboard(RIGHT, 1);
	}
	
	
	if (key == GLFW_KEY_B && (action == GLFW_PRESS)) {
		//sponza.camera->print_camera_coords();
		//On pressing B, light will be updated to the current position, direction  and up of the camera
		lightDir = -sponza.camera->Front;
		ref_pos = sponza.camera->Position;
		ref_up = sponza.camera->Up;
		ref_front = sponza.camera->Front;
		shadowmap.view = glm::lookAt(ref_pos, ref_front, ref_up);
	}	

	if (key == GLFW_KEY_P && (action == GLFW_PRESS)) {
		render_from_light = !render_from_light;
	}

	if (key == GLFW_KEY_O && (action == GLFW_PRESS)) {
		render_complete = !render_complete;
	}
}

//mouse function
void mfunc(GLFWwindow *window, double xpos, double ypos) {
	int state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
	
	if (state == GLFW_PRESS) {
		if(active_cam)
		{
			lastX = xpos;
			lastY = ypos;
			active_cam = false;
		}
		double offsetx = xpos - lastX;
		double offsety = lastY - ypos;

		sponza.camera->ProcessMouseMovement(offsetx, offsety);
		lastX = xpos;
		lastY = ypos;

	}	
	else if (state == GLFW_RELEASE) {
		active_cam = true;
	}
}

int main(int argc, char **argv) {
	
	//TODO : 
	//1 -MAKE LIGHT CALCULATIONS WITH DIRECTIONAL LIGHT
	//2 -IMPLEMENT G BUFFER
	//3 -IMPLEMENT REFLECTIVE SHADOW MAP ALGORITHM
	//4 -IMPLEMENT DEEP G BUFFER
	//5 -EXPERIMENT WITH RADIOSITY + REFLECTIVE SHADOW MAP 
	

	float Wid = 1240.0f;
	float Hei = 720.0f;

	float s_w = 1024.f;
	float s_h = 1024.f;
	float s_near = 0.1f;
	float s_far = 200.0f;

	window w;
	w.create_window(Wid, Hei);
	auto err = glewInit();

	if (err != GLEW_OK){
		std::cout << "failed to init glew" << std::endl;
		return -1;
	}
	std::cout << "glew initialized succesfully" << std::endl;

	float fov = 60.0f;
	float n_v = 0.1f;
	float f_v = 200.0f;
		
	//Resize sponza
	sponza.mesh = new mesh_loader(sponza_path);
	sponza.bb_mid = sponza.mesh->bb_mid * 0.05f;
	sponza.model = glm::scale(glm::mat4(1.0), glm::vec3(0.05f));
	sponza.proj = glm::perspective(glm::radians(fov), Wid/Hei, n_v, f_v);
	//sponza orthographic matrix for debug purposes
	//sponza.proj = glm::ortho(-100.f, 100.f, -100.f, 100.f, n_v, f_v);

	glm::vec3 eyePos = sponza.bb_mid;
	sponza.shader = new Shader("shaders/sponza_full_vert.glsl", "shaders/sponza_full_frag.glsl", nullptr);
	sponza.camera = new Camera(eyePos);
	sponza.view = sponza.camera->GetViewMatrix();
	
	/*---- LIGHT DATA----*/
	lightPos = glm::vec3(34.5808f, 112.202f, 0.45444f);//sponza.bb_mid;
	
	
	/*----OPENGL Enable/Disable functions----*/
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glClearColor(1.0, 1.0, 1.0, 1.0);	
	
	/*----SET CALLBACKS----*/
	glfwSetKeyCallback(w.wnd, kbfunc);
	glfwSetCursorPosCallback(w.wnd, mfunc);
		
	/*--- SHADOW MAP LIGHT TRANSFORMS---*/
		
	shadowmap.proj = glm::ortho(-100.0f, 100.0f, -100.0f, 100.0f, s_near, s_far);
	//starting light position
	glm::vec3 lightFront = glm::vec3(-0.397029f, -0.917755f, -0.00970367f);
	glm::vec3 lightUp = glm::vec3(-0.917481f, 0.397147f, -0.0224239f);
	shadowmap.view = glm::lookAt(lightPos, lightFront, lightUp);
	lightDir = -glm::normalize(lightFront);	
	shadowmap.shader = new Shader("shaders/shadow_map_vert.glsl", "shaders/shadow_map_frag.glsl", nullptr);
	glm::mat4 lightspacemat;

	/*----SHADOW MAP ---- RSM -----*/
	Shader depthView("shaders/depth_view_vert.glsl", "shaders/depth_view_frag.glsl", nullptr);
	framebuffer depthBuffer(fbo_type::SHADOW_MAP, s_w, s_h);
	
	/*------- AXIS ---------*/
	Axis3D origin = {0,0};
	Shader *axis_program;
	axis_program = new Shader("shaders/axis_view_vert.glsl", "shaders/axis_view_frag.glsl");
	glm::mat4 axis_MVP;

	while (!w.should_close()) {
			
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	

		lightspacemat = shadowmap.proj * shadowmap.view;

		glCullFace(GL_FRONT);
		shadowmap.shader->use();
		shadowmap.shader->setMat4("PV", lightspacemat);
		shadowmap.shader->setMat4("M", sponza.model);		
		glViewport(0, 0, s_w, s_h);
		depthBuffer.bind();
		glClear(GL_DEPTH_BUFFER_BIT);
		sponza.mesh->Draw(*shadowmap.shader);
		depthBuffer.unbind();
		glCullFace(GL_BACK);
		
		if (render_complete)
		{
			glViewport(0, 0, Wid, Hei);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			sponza.view = sponza.camera->GetViewMatrix();
			sponza.shader->use();
			sponza.shader->setMat4("M", sponza.model);
			sponza.shader->setMat4("V", sponza.view);
			sponza.shader->setMat4("P", sponza.proj);
			sponza.shader->setMat4("LightSpaceMat", lightspacemat);
			sponza.shader->setVec3("eyePos", sponza.camera->Position);
			sponza.shader->setVec3("lightDir", lightDir);
			sponza.shader->setInt("shadow_Map", 5);
			GLCall(glActiveTexture(GL_TEXTURE5));
			GLCall(glBindTexture(GL_TEXTURE_2D, depthBuffer.depth_map));
			sponza.mesh->Draw(*sponza.shader);
			//3D axis to serve as spatial reference, set at the origin {0, 0, 0} of the world, representing the canonical basis 
			//GLCall(glLineWidth(5));
			//axis_program->use();
			//axis_MVP = sponza.proj * sponza.view * glm::scale(glm::mat4(1), glm::vec3(15));
			//axis_program->setMat4("MVP", axis_MVP);
			//origin.renderAxis();
		}
		else
		{
			//debug view, shadow map
			glViewport(0,0, Wid, Hei);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			depthView.use();
			depthView.setInt("screen_tex", 0);
			depthView.setFloat("near", s_near);
			depthView.setFloat("far", s_far);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, depthBuffer.depth_map);
			screen_quad.renderQuad();	
		}
		
		w.swap();	
		w.poll_ev();
	}	
	
	
	//std::cout << "press enter to terminate" << std::endl;
	//std::cin.get();
	glfwTerminate();	
	if (sponza.mesh)
		delete sponza.mesh;
	if (sponza.shader)
		delete sponza.shader;
	if (sponza.camera)
		delete sponza.camera;
	if (shadowmap.shader)
		delete shadowmap.shader;
	if (axis_program)
		delete axis_program;
	return 0;
}

