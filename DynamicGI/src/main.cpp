#include <iostream>
#include <string>
#include <random>

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

/*SAMPLING PARAMETERS */

const unsigned int SAMPLING_SIZE = 64; // rsm sampling
float r_max = 0.03f;// max r for sampling pattern of rsm
const float PI = 3.1415926f;
const float PI_TWO = 6.2831853;

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
		//lightDir = -sponza.camera->Front;
		//ref_pos = sponza.camera->Position;
		//ref_up = sponza.camera->Up;
		//ref_front = sponza.camera->Front;
		//shadowmap.view = glm::lookAt(ref_pos, ref_front, ref_up);
		std::cout << "value of r_max is = " << r_max << std::endl;
	}	

	if (key == GLFW_KEY_P && (action == GLFW_PRESS)) {
		render_from_light = !render_from_light;
	}

	if (key == GLFW_KEY_O && (action == GLFW_PRESS)) {
		render_complete = !render_complete;
	}

	if (key == GLFW_KEY_U && (action == GLFW_PRESS)) {
		r_max += 0.01;
	}
	if (key == GLFW_KEY_J && (action == GLFW_PRESS)) {
		r_max -= 0.01;
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
	//1 -MAKE LIGHT CALCULATIONS WITH DIRECTIONAL LIGHT - DONE X
	//2 -IMPLEMENT G BUFFER - DONE X
	//3 -IMPLEMENT REFLECTIVE SHADOW MAP ALGORITHM 
	//4 -IMPLEMENT DEEP G BUFFER
	//5 -EXPERIMENT WITH RADIOSITY + REFLECTIVE SHADOW MAP 
	
	/*-----parameters----*/
	float Wid = 1240.0f;
	float Hei = 720.0f;

	float s_w = 1024.f;
	float s_h = 1024.f;
	float s_near = 0.1f;//0.1f;
	float s_far = 400.f;//200.0f;

	float fov = 60.0f;
	float n_v = 0.1f;
	float f_v = 400.0f;

	/*----SET WINDOW AND CALLBACKS ----*/
	
	window w;
	w.create_window(Wid, Hei);
	auto err = glewInit();

	if (err != GLEW_OK){
		std::cout << "failed to init glew" << std::endl;
		return -1;
	}
	std::cout << "glew initialized succesfully" << std::endl;
		
	w.set_kb(kbfunc);
	w.set_mouse(mfunc);
	/*---------------------------------*/
			
	//Resize sponza
	sponza.mesh = new mesh_loader(sponza_path);
	sponza.bb_mid = sponza.mesh->bb_mid * 0.05f;
	sponza.model = glm::scale(glm::mat4(1.0), glm::vec3(0.05f));
	sponza.proj = glm::perspective(glm::radians(fov), Wid/Hei, n_v, f_v);
	//sponza orthographic matrix for debug purposes
	//sponza.proj = glm::ortho(-20.f, 20.f, -20.f, 20.f, n_v, f_v);

	glm::vec3 eyePos = sponza.bb_mid + glm::vec3(0,20,0);
	sponza.shader = new Shader("shaders/screen_quad_vert.glsl", "shaders/rsm_shade_frag.glsl", nullptr);
	sponza.camera = new Camera(eyePos);
	sponza.view = sponza.camera->GetViewMatrix();
	
	
		
	/*--- SHADOW MAP LIGHT TRANSFORMS---*/		
	shadowmap.proj = glm::ortho(-50.0f, 50.0f, -50.0f, 50.0f, s_near, s_far);
		
	glm::vec3 l_pos = sponza.bb_mid + glm::vec3(0, 110, 0); // light position
	glm::vec3 l_center = sponza.bb_mid; //light looking at
	glm::vec3 worldup(0.0, 1.0, 0.2); //movendo ligeiramente o vetor up, caso contrario teriamos direcao e up colineares, a matriz seria degenerada
	shadowmap.view = glm::lookAt(l_pos, l_center, worldup);
	lightDir = glm::normalize(l_pos - l_center);	

	shadowmap.shader = new Shader("shaders/shadow_map_vert.glsl", "shaders/shadow_map_frag.glsl", nullptr);
	glm::mat4 lightspacemat;

	/*----SHADOW MAP ---- RSM -----*/
	Shader depthView("shaders/screen_quad_vert.glsl", "shaders/depth_view_frag.glsl", nullptr);
	framebuffer depthBuffer(fbo_type::SHADOW_MAP, s_w, s_h);
	
	/*------- AXIS ---------*/
	//Axis3D origin = {0,0};
	//Shader *axis_program;
	//axis_program = new Shader("shaders/axis_view_vert.glsl", "shaders/axis_view_frag.glsl");
	//glm::mat4 axis_MVP;
	
	/*----OPENGL Enable/Disable functions----*/
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glClearColor(0.5, 0.8, 0.9, 1.0); 
	//glClearColor(1.0, 1.0, 1.0, 1.0);
		
	/* ---------------- G-BUFFER ----------------*/
	framebuffer gbuffer(fbo_type::G_BUFFER, Wid, Hei);
	Shader geometry_pass ("shaders/deferred_render_vert.glsl", "shaders/deferred_render_frag.glsl");
	
	/* -------------- RSM BUFFER ----------------*/

	unsigned int rsm_w = 1024;
	unsigned int rsm_h = 1024;
	
	framebuffer rsm(fbo_type::RSM, rsm_w, rsm_h);
	Shader RSM_pass("shaders/rsm_vert.glsl", "shaders/rsm_frag.glsl");

	/*-------- RSM SHADING PARAMETERS ----------*/
	std::uniform_real_distribution<float> random(0.0f, 1.0f);
	std::default_random_engine gen;

	//normalized random numbers
	float sig1; //first random
	float sig2; //second random

	std::vector <glm::vec2> pattern;
	pattern.resize(SAMPLING_SIZE);//precalculated sampling pattern

	for(int i = 0;i < SAMPLING_SIZE; ++i ){
		//normalized random numbers
		sig1 = random(gen); //first random
		sig2 = random(gen); //second random
		pattern[i].x = sig1 * sin(PI_TWO *sig2);
		pattern[i].y = sig1 * cos(PI_TWO *sig2);
	}

	while (!w.should_close()) {
			
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	

		lightspacemat = shadowmap.proj * shadowmap.view;

		/*----- SHADOW MAP PASS-----*/
		glCullFace(GL_FRONT);
		shadowmap.shader->use();
		shadowmap.shader->setMat4("PV", lightspacemat);
		shadowmap.shader->setMat4("M", sponza.model);		
		glViewport(0, 0, s_w, s_h);
		depthBuffer.bind();
		glClear(GL_DEPTH_BUFFER_BIT);
		sponza.mesh->Draw(*shadowmap.shader);
		depthBuffer.unbind();

		/*--------------------------*/

		glCullFace(GL_BACK);

		/*---------RSM_Pass--------*/

		RSM_pass.use();
		RSM_pass.setMat4("P", shadowmap.proj);
		RSM_pass.setMat4("V", shadowmap.view);
		RSM_pass.setMat4("M", sponza.model);
		RSM_pass.setFloat("lightColor", 0.65f);
		rsm.bind();
		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
		sponza.mesh->Draw(RSM_pass);
		rsm.unbind();

		if (render_complete)
		{						
			glViewport(0,0, Wid, Hei);
			gbuffer.bind();
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			sponza.view = sponza.camera->GetViewMatrix();
			geometry_pass.use();
			geometry_pass.setMat4("M", sponza.model);
			geometry_pass.setMat4("V", sponza.view);
			geometry_pass.setMat4("P", sponza.proj);
			sponza.mesh->Draw(*sponza.shader);
			gbuffer.unbind();	

			/* ------ SHADING PASS ------*/

			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			
			sponza.view = sponza.camera->GetViewMatrix();
			sponza.shader->use();			
			GLCall(glActiveTexture(GL_TEXTURE0));
			sponza.shader->setInt("gposition", 0);
			GLCall(glBindTexture(GL_TEXTURE_2D, gbuffer.pos));
			GLCall(glActiveTexture(GL_TEXTURE1));
			sponza.shader->setInt("gnormal", 1);
			GLCall(glBindTexture(GL_TEXTURE_2D, gbuffer.normal));
			GLCall(glActiveTexture(GL_TEXTURE2));
			sponza.shader->setInt("galbedo", 2);
			GLCall(glBindTexture(GL_TEXTURE_2D, gbuffer.albedo));
			GLCall(glActiveTexture(GL_TEXTURE3));
			sponza.shader->setInt("shadow_map", 3);
			GLCall(glBindTexture(GL_TEXTURE_2D, depthBuffer.depth_map));

			GLCall(glActiveTexture(GL_TEXTURE4));
			sponza.shader->setInt("rsm_position", 4);
			GLCall(glBindTexture(GL_TEXTURE_2D, rsm.pos));
			GLCall(glActiveTexture(GL_TEXTURE5));
			sponza.shader->setInt("rsm_normal", 5);
			GLCall(glBindTexture(GL_TEXTURE_2D, rsm.normal));
			GLCall(glActiveTexture(GL_TEXTURE6));
			sponza.shader->setInt("rsm_flux", 6);
			GLCall(glBindTexture(GL_TEXTURE_2D, rsm.albedo));


						
			sponza.shader->setMat4("LightSpaceMat", lightspacemat);
			sponza.shader->setVec3("eyePos", sponza.camera->Position);
			sponza.shader->setVec3("lightDir", lightDir);
			sponza.shader->setVec3("lightColor", glm::vec3(0.65f));
			sponza.shader->setFloat("rmax", r_max);

			for (int i = 0; i < SAMPLING_SIZE; ++i) {
				std::string loc = "samples["+std::to_string(i)+"]";
				sponza.shader->setVec2(loc, pattern[i]);
			}

			screen_quad.renderQuad();			
			
			/*---------------------------------------------------------------------------------------------------------------*/
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
			depthView.setFloat("near", s_near);
			depthView.setFloat("far", s_far);
			GLCall(glActiveTexture(GL_TEXTURE0));
			depthView.setInt("screen_tex", 0);
			//GLCall(glBindTexture(GL_TEXTURE_2D, depthBuffer.depth_map));
			GLCall(glBindTexture(GL_TEXTURE_2D, rsm.albedo));
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
	//if (axis_program)
	//	delete axis_program;
	return 0;
}

