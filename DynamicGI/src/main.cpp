#include <iostream>
#include <string>
#include <gl/glew.h>
#include "window.h"
#include "GL_CALL.h"
#include "mesh_loader.h"
#include "camera.h"
#include "shader.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Quad.h"
#include "framebuffer.h"


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
quad screen_quad;

/*---camera controls---*/
double lastX = 0, lastY = 0;
bool active_cam = true;

//keyboard function
void kbfunc(GLFWwindow* window, int key, int scan, int action, int mods) {
	if (key == GLFW_KEY_ESCAPE && (action == GLFW_PRESS || action == GLFW_REPEAT))
		glfwSetWindowShouldClose(window, GLFW_TRUE);	

	if (key == GLFW_KEY_W && (action == GLFW_PRESS || action == GLFW_REPEAT))
		sponza.camera->ProcessKeyboard(FORWARD, 1);

	if (key == GLFW_KEY_A && (action == GLFW_PRESS || action == GLFW_REPEAT))
		sponza.camera->ProcessKeyboard(LEFT, 1);

	if (key == GLFW_KEY_S && (action == GLFW_PRESS || action == GLFW_REPEAT))
		sponza.camera->ProcessKeyboard(BACKWARD, 1);

	if (key == GLFW_KEY_D && (action == GLFW_PRESS || action == GLFW_REPEAT))
		sponza.camera->ProcessKeyboard(RIGHT, 1);
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
	
	float Wid = 1240.0f;
	float Hei = 720.0f;

	window w;
	w.create_window(Wid, Hei);
	auto err = glewInit();

	if (err != GLEW_OK){
		std::cout << "failed to init glew" << std::endl;
		return -1;
	}
	std::cout << "glew initialized succesfully" << std::endl;

	float fov = 90.0f;
	float n_v = 0.1f;
	float f_v = 500.0f;
		
	//Redimensionar sponza, o modelo é muito grande
	sponza.mesh = new mesh_loader(sponza_path);
	sponza.bb_mid = sponza.mesh->bb_mid * 0.05f;
	sponza.model = glm::scale(glm::mat4(1.0), glm::vec3(0.05f));
	sponza.proj = glm::perspective(glm::radians(fov), Wid/Hei, n_v, f_v);

	glm::vec3 eyePos = sponza.bb_mid;
	sponza.shader = new Shader("shaders/sponza_full_vert.glsl", "shaders/sponza_full_frag.glsl", nullptr);
	sponza.camera = new Camera(eyePos);
	sponza.view = sponza.camera->GetViewMatrix();
	glm::vec3 lightPos = sponza.bb_mid;//glm::vec3(-2, 4.0, -1.0);
	
	/*----OPENGL Enable/Disable functions----*/
	glEnable(GL_DEPTH_TEST);
	glClearColor(1.0, 1.0, 1.0, 1.0);
	glViewport(0, 0, Wid, Hei);
	
	/*----SET CALLBACKS----*/
	glfwSetKeyCallback(w.wnd, kbfunc);
	glfwSetCursorPosCallback(w.wnd, mfunc);
		
	/*--- SHADOW MAP LIGHT TRANSFORMS---*/
	shadow_map shadowmap;	
	shadowmap.proj = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, n_v, f_v);
	shadowmap.view = glm::lookAt(lightPos, glm::vec3(0.0, -1.0, 0.0), glm::vec3(1.0f, 0.0f, 0.0f));
	shadowmap.shader = new Shader("shaders/shadow_map_vert.glsl", "shaders/shadow_map_frag.glsl", nullptr);

	/*----SHADOW MAP ---- RSM -----*/
	Shader depthView("shaders/depth_view_vert.glsl", "shaders/depth_view_frag.glsl", nullptr);
	framebuffer depthBuffer(fbo_type::SHADOW_MAP, Wid, Hei);
	

	/*----- SCREEN QUAD ---- */

	while (!w.should_close()) {
			
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
		//sponza.view = sponza.camera->GetViewMatrix();
		//sponza.shader->use();
		//sponza.shader->setMat4("M", sponza.model);
		//sponza.shader->setMat4("V", sponza.view);
		//sponza.shader->setMat4("P", sponza.proj);
		//sponza.shader->setVec3("eyePos", sponza.camera->Position);
		//sponza.shader->setVec3("lightPos", lightPos);
		//sponza.mesh->Draw(*sponza.shader);	

		shadowmap.shader->use();
		shadowmap.shader->setMat4("PV", shadowmap.proj * shadowmap.view);
		shadowmap.shader->setMat4("M", sponza.model);

		glViewport(0, 0, Wid, Hei);
		depthBuffer.bind();
		glClear(GL_DEPTH_BUFFER_BIT);
		sponza.mesh->Draw(*shadowmap.shader);
		depthBuffer.unbind();

		glViewport(0,0, Wid, Hei);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		sponza.view = sponza.camera->GetViewMatrix();
		sponza.shader->use();
		sponza.shader->setMat4("M", sponza.model);
		sponza.shader->setMat4("V", sponza.view);
		sponza.shader->setMat4("P", sponza.proj);
		sponza.shader->setMat4("LightSpaceMat", shadowmap.proj * shadowmap.view);
		sponza.shader->setVec3("eyePos", sponza.camera->Position);
		sponza.shader->setVec3("lightPos", lightPos);
		sponza.shader->setInt("shadowMap", 1);
		GLCall(glActiveTexture(GL_TEXTURE1));
		GLCall(glBindTexture(GL_TEXTURE_2D, depthBuffer.depth_map));
		sponza.mesh->Draw(*sponza.shader);
		

		//debug view, shadow map
		//glViewport(0,0, Wid, Hei);
		//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		//depthView.use();
		//depthView.setInt("screen_tex", 0);
		//depthView.setFloat("near", n_v);
		//depthView.setFloat("far", f_v);
		//glActiveTexture(GL_TEXTURE0);
		//glBindTexture(GL_TEXTURE_2D, depthBuffer.depth_map);
		//screen_quad.renderQuad();


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
	return 0;
}

