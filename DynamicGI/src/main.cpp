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

//GLOBAL CONSTANTS
const float PI = 3.1415926f;
const float PI_TWO = 6.2831853;

//scene and model paths
const char* sponza_path = "models/sponza/sponza.obj";

//Number of turns for spiral quasi monte carlo sampling, pre computed by McGuire for max of N = 100 samples 
//Taken from the G3D DeepGBuffer radiosity samples
//https://casual-effects.com/g3d/www/index.html
static int minDiscrepancyArray[100] = {
	//  0   1   2   3   4   5   6   7   8   9
	1,  1,  1,  2,  3,  2,  5,  2,  3,  2,  // 0
	3,  3,  5,  5,  3,  4,  7,  5,  5,  7,  // 1
	9,  8,  5,  5,  7,  7,  7,  8,  5,  8,  // 2
	11, 12,  7, 10, 13,  8, 11,  8,  7, 14,  // 3
	11, 11, 13, 12, 13, 19, 17, 13, 11, 18,  // 4
	19, 11, 11, 14, 17, 21, 15, 16, 17, 18,  // 5
	13, 17, 11, 17, 19, 18, 25, 18, 19, 19,  // 6
	29, 21, 19, 27, 31, 29, 21, 18, 17, 29,  // 7
	31, 31, 23, 18, 25, 26, 25, 23, 19, 34,  // 8
	19, 27, 21, 25, 39, 29, 17, 21, 27, 29 }; // 9


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
	float n_val;
	float f_val;
};

struct shadow_map {	
	glm::mat4 proj;
	glm::mat4 view;
	glm::mat4 model;
	glm::mat4 light_space_mat;
	Shader *shader;
	/*---resolution---*/
	float s_w;
	float s_h;
	float s_near;
	float s_far;
	/*---Light Information---*/
	glm::vec3 lightDir;
	glm::vec3 lightColor;
	glm::vec3 lightPos;
};

scene sponza;
shadow_map shadowmap;
quad screen_quad;

/*---camera controls---*/
double lastX = 0, lastY = 0;
bool active_cam = true;
bool render_from_light = false;
bool debug_view = false;
glm::vec3 ref_up;
glm::vec3 ref_front;
glm::vec3 ref_pos;
glm::vec3 lightPos;
glm::vec3 lightDir;

/*----------------*/
bool rsm_shading = true;

struct RSM_parameters {
	std::vector<glm::vec2> pattern;
	const unsigned int SAMPLING_SIZE = 100; // rsm sampling
	float r_max = 0.03f;// max r for sampling pattern of rsm
	float rsm_w;
	float rsm_h;
} rsm_param;


/* ------ FUNCTION SIGNATURES ------ */
void generate_rsm_sampling_pattern(std::vector<glm::vec2>& p);

void render_debug_view(window& wnd, Shader& debug_program, quad& debug_quad, shadow_map& shadow_data, framebuffer& shadow_buffer);

//keyboard function
void kbfunc(GLFWwindow* window, int key, int scan, int action, int mods); 
//mouse function
void mfunc(GLFWwindow *window, double xpos, double ypos);

void shadow_pass(shadow_map& data, framebuffer& depth_buffer, mesh_loader& mesh);

void rsm_pass(Shader& RSM_pass, framebuffer& rsm, shadow_map& light_data);

void gbuffer_pass(framebuffer& gbuffer, Shader& gbuf_program, scene& s);

void rsm_shading_pass(quad& screen_quad, scene& s, shadow_map& light_data, RSM_parameters& rsm_data, framebuffer& gbuffer, framebuffer& depth_buffer, framebuffer& rsm_buffer);

void deep_g_buffer_pass(Shader& dgb_program, framebuffer& dgbuffer, scene& s, float delta);

void deep_g_buffer_debug(Shader& debug_view, framebuffer& dgb, quad& screen, shadow_map&  light_data, scene& s, framebuffer& depth_buffer);

void pre_radiosity_pass(Shader& radio, framebuffer& r_buffer, framebuffer& dgbuffer, shadow_map& light_data, quad& screen);


int main(int argc, char **argv) {
	
	//TODO : 
	//1 -MAKE LIGHT CALCULATIONS WITH DIRECTIONAL LIGHT - DONE X
	//2 -IMPLEMENT G BUFFER - DONE X
	//3 -IMPLEMENT REFLECTIVE SHADOW MAP ALGORITHM - DONE X
	//4 -IMPLEMENT DEEP G BUFFER - DONE X 

	//5 -EXPERIMENT WITH RADIOSITY + REFLECTIVE SHADOW MAP - WIP

	//6 -RENAME STRUCT SHADOW_MAP TO SHADOW_PASS_DATA
	
	/*-----parameters----*/
	float Wid = 1240.0f;
	float Hei = 720.0f;
	
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
	sponza.n_val = n_v;
	sponza.f_val = f_v;
	//sponza orthographic matrix for debug purposes
	//sponza.proj = glm::ortho(-20.f, 20.f, -20.f, 20.f, n_v, f_v);

	glm::vec3 eyePos = sponza.bb_mid + glm::vec3(0,20,0);
	sponza.shader = new Shader("shaders/screen_quad_vert.glsl", "shaders/rsm_shade_frag.glsl", nullptr);
	sponza.camera = new Camera(eyePos);
	sponza.view = sponza.camera->GetViewMatrix();	
		
	/*--- SHADOW MAP LIGHT TRANSFORMS---*/		
	shadowmap.s_w = 1024.0f;
	shadowmap.s_h = 1024.0f;
	shadowmap.s_near = 0.1f;
	shadowmap.s_far = 400.0f;

	shadowmap.proj = glm::ortho(-50.0f, 50.0f, -50.0f, 50.0f, shadowmap.s_near, shadowmap.s_far);
	
	glm::vec3 l_pos = sponza.bb_mid + glm::vec3(0, 100, 0); // light position
	shadowmap.lightPos = l_pos;
	glm::vec3 l_center = sponza.bb_mid; //light looking at
	glm::vec3 worldup(0.0, 1.0, 0.2); //movendo ligeiramente o vetor up, caso contrario teriamos direcao e up colineares, a matriz seria degenerada
	shadowmap.view = glm::lookAt(l_pos, l_center, worldup);
	lightDir = glm::normalize(l_pos - l_center);	
	shadowmap.lightDir = lightDir;
	shadowmap.lightColor = glm::vec3(0.65f);

	shadowmap.shader = new Shader("shaders/shadow_map_vert.glsl", "shaders/shadow_map_frag.glsl", nullptr);
	glm::mat4 lightspacemat;

	/*----SHADOW MAP ---- RSM -----*/
	Shader depthView("shaders/screen_quad_vert.glsl", "shaders/depth_view_frag.glsl", nullptr);
	framebuffer depthBuffer(fbo_type::SHADOW_MAP, shadowmap.s_w, shadowmap.s_h, 0);
	
	
	/*----OPENGL Enable/Disable functions----*/
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glClearColor(0.5, 0.8, 0.9, 1.0); 	

	/* ---------------- G-BUFFER ----------------*/
	framebuffer gbuffer(fbo_type::G_BUFFER, w.Wid, w.Hei, 0);
	Shader geometry_pass ("shaders/deferred_render_vert.glsl", "shaders/deferred_render_frag.glsl");
	
	/* -------------- RSM BUFFER ----------------*/
		
	rsm_param.rsm_w = shadowmap.s_w;
	rsm_param.rsm_h = shadowmap.s_h;

	framebuffer rsm_buffer(fbo_type::RSM, rsm_param.rsm_w, rsm_param.rsm_h, 0);
	Shader RSM_pass("shaders/rsm_vert.glsl", "shaders/rsm_frag.glsl");

	/* ---- STOCHASTIC SAMPLING OF RSM MAP ---- */
	generate_rsm_sampling_pattern(rsm_param.pattern);

	/* ------------ DEEP G BUFFER - 2 LAYER ----------*/
	
	float Delta = 1.0f;
	const int Layers = 2;
	framebuffer deepgbuffer(fbo_type::DEEP_G_BUFFER,  Wid, Hei, Layers);
	Shader dgb_program("shaders/deep_g_buffer_vert.glsl", "shaders/deep_g_buffer_frag.glsl", "shaders/deep_g_buffer_geom.glsl");
	Shader dgb_debug_view("shaders/screen_quad_vert.glsl","shaders/dgb_debug_view_frag.glsl");

	/* ------------RADIOSITY BUFFER ------------------*/
	framebuffer radiosity(fbo_type::RADIOSITY_PARAM, Wid, Hei, Layers);
	Shader deep_radiosity("shaders/screen_quad_vert.glsl", "shaders/radiosity_frag.glsl", "shaders/radiosity_geom.glsl");
	Shader radiosity_shading("shaders/screen_quad_vert.glsl", "shaders/radiosity_shade_frag.glsl");

	unsigned int N = 13;
	unsigned int turns = minDiscrepancyArray[N];
	float radius = 0.8f; //in world units

	//main loop
	while (!w.should_close()) {
			
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	
		
		shadowmap.model = sponza.model;

		/*---- SHADOW MAP PASS ----*/
		shadow_pass(shadowmap, depthBuffer, *sponza.mesh);

		/*------- LIGHT SPACE MATRIX -------*/
		shadowmap.light_space_mat = shadowmap.proj * shadowmap.view;
		
		/*---------RSM_Pass--------*/
		rsm_pass(RSM_pass, rsm_buffer, shadowmap);		

		if (!debug_view)
		{			
			
			///* -------G BUFFER PASS------*/
			//gbuffer_pass(gbuffer, geometry_pass, sponza);
			///* ------ SHADING PASS ------*/
			//rsm_shading_pass(screen_quad, sponza, shadowmap, rsm_param, gbuffer, depthBuffer, rsm_buffer);
						
			/*------- DEEP G BUFFER PASS ------*/
			deep_g_buffer_pass(dgb_program, deepgbuffer, sponza, Delta);

			/*-------------PRE-SHADING -------------*/
			//deep_g_buffer_debug(dgb_debug_view, deepgbuffer, screen_quad, shadowmap, sponza, depthBuffer);
			pre_radiosity_pass(deep_radiosity, radiosity, deepgbuffer, shadowmap, screen_quad);

			/* -----------GI -----------*/
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			
			radiosity_shading.use();
			glActiveTexture(GL_TEXTURE0);
			radiosity_shading.setInt("gposition", 0);
			glBindTexture(GL_TEXTURE_2D_ARRAY, deepgbuffer.pos);

			glActiveTexture(GL_TEXTURE1);
			radiosity_shading.setInt("gnormal", 1);
			glBindTexture(GL_TEXTURE_2D_ARRAY, deepgbuffer.normal);

			glActiveTexture(GL_TEXTURE2);
			radiosity_shading.setInt("galbedo", 2);
			glBindTexture(GL_TEXTURE_2D_ARRAY, deepgbuffer.albedo);

			glActiveTexture(GL_TEXTURE3);
			radiosity_shading.setInt("shadow_map", 3);
			glBindTexture(GL_TEXTURE_2D, depthBuffer.depth_map);

			glActiveTexture(GL_TEXTURE4);
			radiosity_shading.setInt("gradiosity", 4);
			glBindTexture(GL_TEXTURE_2D_ARRAY, radiosity.albedo);

			radiosity_shading.setVec3("eyePos", sponza.camera->Position);
			radiosity_shading.setVec3("lightDir",   shadowmap.lightDir);
			radiosity_shading.setVec3("lightColor", shadowmap.lightColor);
			radiosity_shading.setMat4("LightSpaceMat", shadowmap.light_space_mat);
			
			radiosity_shading.setInt("samples", N);
			radiosity_shading.setInt("layers",  Layers);
			radiosity_shading.setFloat("radius",radius);
			radiosity_shading.setFloat("turn",  turns);
			
			screen_quad.renderQuad();			
		}
		else
		{
			/* ---- DEBUG VIEW ---- */
			render_debug_view(w, depthView, screen_quad, shadowmap, depthBuffer);
			
		}
		
		w.swap();	
		w.poll_ev();
	}		
	
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
		std::cout << "value of r_max is = " << rsm_param.r_max << std::endl;
	}

	if (key == GLFW_KEY_P && (action == GLFW_PRESS)) {
		render_from_light = !render_from_light;
	}

	if (key == GLFW_KEY_O && (action == GLFW_PRESS)) {
		debug_view = !debug_view;
	}

	if (key == GLFW_KEY_U && (action == GLFW_PRESS)) {
		rsm_param.r_max += 0.5;
	}
	if (key == GLFW_KEY_J && (action == GLFW_PRESS)) {
		rsm_param.r_max -= 0.5;
	}

	if (key == GLFW_KEY_0 && (action == GLFW_PRESS)) {
		rsm_shading = !rsm_shading;
	}
}

//mouse function
void mfunc(GLFWwindow *window, double xpos, double ypos) {
	int state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);

	if (state == GLFW_PRESS) {
		if (active_cam)
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

void generate_rsm_sampling_pattern(std::vector<glm::vec2>& p) {

	/*-------- RSM SHADING PARAMETERS ----------*/
	std::uniform_real_distribution<float> random(0.0f, 1.0f);
	std::default_random_engine gen;

	//normalized random numbers
	float sig1; //first random
	float sig2; //second random

	p.resize(rsm_param.SAMPLING_SIZE);//precalculated sampling pattern

	for (int i = 0; i < rsm_param.SAMPLING_SIZE; ++i) {
		//normalized random numbers
		sig1 = random(gen); //first random
		sig2 = random(gen); //second random
		p[i].x = sig1 * sin(PI_TWO *sig2);
		p[i].y = sig1 * cos(PI_TWO *sig2);
	}
}

void render_debug_view(window& wnd, Shader& debug_program, quad& debug_quad, shadow_map& shadow_data, framebuffer& shadow_buffer) {
	//debug view, shadow map
	glViewport(0, 0, wnd.Wid, wnd.Hei);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	debug_program.use();
	debug_program.setFloat("near", shadow_data.s_near);
	debug_program.setFloat("far", shadow_data.s_far);
	GLCall(glActiveTexture(GL_TEXTURE0));
	debug_program.setInt("screen_tex", 0);
	GLCall(glBindTexture(GL_TEXTURE_2D, shadow_buffer.depth_map));
	//GLCall(glBindTexture(GL_TEXTURE_2D, rsm.albedo));
	debug_quad.renderQuad();
}

void shadow_pass(shadow_map& data, framebuffer& depth_buffer, mesh_loader& mesh) {

	glm::mat4 lightspacemat = data.proj * data.view;

	/*----- SHADOW MAP PASS-----*/
	glCullFace(GL_FRONT);
	data.shader->use();
	data.shader->setMat4("PV", lightspacemat);
	data.shader->setMat4("M", data.model);
	glViewport(0, 0, data.s_w, data.s_h);
	depth_buffer.bind();
	glClear(GL_DEPTH_BUFFER_BIT);
	mesh.Draw(*data.shader);
	depth_buffer.unbind();

	/*--------------------------*/

	glCullFace(GL_BACK);
}

void rsm_pass(Shader& RSM_pass, framebuffer& rsm, shadow_map& light_data) {
	/*---------RSM_Pass--------*/
	RSM_pass.use();
	RSM_pass.setMat4("P", light_data.proj);
	RSM_pass.setMat4("V", light_data.view);
	RSM_pass.setMat4("M", light_data.model);
	RSM_pass.setFloat("lightColor", 0.65f);
	rsm.bind();
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	sponza.mesh->Draw(RSM_pass);
	rsm.unbind();
}

void gbuffer_pass(framebuffer& gbuffer, Shader& gbuf_program, scene& s) {

	glViewport(0, 0, gbuffer.w_res, gbuffer.h_res);
	gbuffer.bind();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	s.view = s.camera->GetViewMatrix();
	gbuf_program.use();
	gbuf_program.setMat4("M", s.model);
	gbuf_program.setMat4("V", s.view);
	gbuf_program.setMat4("P", s.proj);
	s.mesh->Draw(gbuf_program);
	gbuffer.unbind();
}

void rsm_shading_pass(quad& screen_quad, scene& s, shadow_map& light_data, RSM_parameters& rsm_data, framebuffer& gbuffer, framebuffer& depth_buffer, framebuffer& rsm_buffer) {

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	s.view = s.camera->GetViewMatrix();
	s.shader->use();
	GLCall(glActiveTexture(GL_TEXTURE0));
	s.shader->setInt("gposition", 0);
	GLCall(glBindTexture(GL_TEXTURE_2D, gbuffer.pos));
	GLCall(glActiveTexture(GL_TEXTURE1));
	s.shader->setInt("gnormal", 1);
	GLCall(glBindTexture(GL_TEXTURE_2D, gbuffer.normal));
	GLCall(glActiveTexture(GL_TEXTURE2));
	s.shader->setInt("galbedo", 2);
	GLCall(glBindTexture(GL_TEXTURE_2D, gbuffer.albedo));
	GLCall(glActiveTexture(GL_TEXTURE3));
	s.shader->setInt("shadow_map", 3);
	GLCall(glBindTexture(GL_TEXTURE_2D, depth_buffer.depth_map));

	GLCall(glActiveTexture(GL_TEXTURE4));
	s.shader->setInt("rsm_position", 4);
	GLCall(glBindTexture(GL_TEXTURE_2D, rsm_buffer.pos));
	GLCall(glActiveTexture(GL_TEXTURE5));
	s.shader->setInt("rsm_normal", 5);
	GLCall(glBindTexture(GL_TEXTURE_2D, rsm_buffer.normal));
	GLCall(glActiveTexture(GL_TEXTURE6));
	s.shader->setInt("rsm_flux", 6);
	GLCall(glBindTexture(GL_TEXTURE_2D, rsm_buffer.albedo));

	s.shader->setMat4("LightSpaceMat", light_data.light_space_mat);
	s.shader->setVec3("eyePos", s.camera->Position);
	s.shader->setVec3("lightDir", light_data.lightDir);
	s.shader->setVec3("lightColor", light_data.lightColor);
	s.shader->setFloat("rmax", rsm_data.r_max);

	for (int i = 0; i < rsm_data.SAMPLING_SIZE; ++i) {
		std::string loc = "samples[" + std::to_string(i) + "]";
		s.shader->setVec2(loc, rsm_data.pattern[i]);
	}

	screen_quad.renderQuad();
}

void deep_g_buffer_pass(Shader& dgb_program, framebuffer& dgbuffer, scene& s, float delta) {
	
	glViewport(0 ,0 , dgbuffer.w_res, dgbuffer.h_res);
	dgbuffer.bind();
	dgbuffer.copy_fb_data();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	s.view = s.camera->GetViewMatrix();
	dgb_program.use();

	dgb_program.setMat4("M", s.model);
	dgb_program.setMat4("V", s.view);
	dgb_program.setMat4("P", s.proj);

	dgb_program.setInt("LAYERS", dgbuffer.layers);
	dgb_program.setFloat("DELTA", delta);
	dgb_program.setFloat("near", s.n_val);
	dgb_program.setFloat("far", s.f_val);
		
	GLCall(glActiveTexture(GL_TEXTURE4));
	dgb_program.setInt("compare_buffer", 4);
	GLCall(glBindTexture(GL_TEXTURE_2D_ARRAY, dgbuffer.compare_depth));

	s.mesh->Draw(dgb_program);	

	dgbuffer.unbind();

}

void deep_g_buffer_debug(Shader& debug_view, framebuffer& dgb, quad& screen, shadow_map&  light_data, scene& s, framebuffer& depth_buffer) {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	debug_view.use();

	glActiveTexture(GL_TEXTURE0);
	debug_view.setInt("gposition", 0);
	glBindTexture(GL_TEXTURE_2D_ARRAY, dgb.pos);

	glActiveTexture(GL_TEXTURE1);
	debug_view.setInt("gnormal", 1);
	glBindTexture(GL_TEXTURE_2D_ARRAY, dgb.normal);

	glActiveTexture(GL_TEXTURE2);
	debug_view.setInt("galbedo", 2);
	glBindTexture(GL_TEXTURE_2D_ARRAY, dgb.albedo);

	glActiveTexture(GL_TEXTURE3);
	debug_view.setInt("shadow_map", 3);
	glBindTexture(GL_TEXTURE_2D, depth_buffer.depth_map);


	debug_view.setVec3("lightDir", light_data.lightDir);
	debug_view.setVec3("lightColor", light_data.lightColor);
	debug_view.setVec3("eyePos", s.camera->Position);
	debug_view.setMat4("lightSpaceMat", light_data.light_space_mat);

	screen.renderQuad();
}

void pre_radiosity_pass(Shader& radio, framebuffer& r_buffer, framebuffer& dgbuffer, shadow_map& light_data, quad& screen) {

	r_buffer.bind();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	radio.use();

	glActiveTexture(GL_TEXTURE1);
	radio.setInt("gnormal", 1);
	glBindTexture(GL_TEXTURE_2D_ARRAY, dgbuffer.normal);

	glActiveTexture(GL_TEXTURE2);
	radio.setInt("galbedo", 2);
	glBindTexture(GL_TEXTURE_2D_ARRAY, dgbuffer.albedo);

	radio.setInt("LAYERS", r_buffer.layers);
	radio.setVec3("lightDir",   light_data.lightDir);
	radio.setVec3("lightColor", light_data.lightColor);

	screen.renderQuad();

	r_buffer.unbind();

}
