#include <iostream>
#include <sstream>
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
#include "uniform_buffer.h"

#include "third_party/imgui.h"
#include "third_party/imgui_impl_glfw.h"
#include "third_party/imgui_impl_opengl3.h"

#include "techniques.h"


//GLOBAL CONSTANTS
const float PI = 3.1415926f;
const float PI_TWO = 6.2831853;
const int VPL_SAMPLES = 8 * 8;

//scene and model paths
const char* sponza_path = "models/sponza/sponza.obj";
const char* sphere_path = "models/sphere/sphere.obj";

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

/*
struct scene {
	mesh_loader *mesh;
	Shader *shader;
	Camera *camera;	
	glm::vec3 bb_mid;	
	glm::mat4 view;
	glm::mat4 proj;
	glm::mat4 model;	
	float n_val;
	float f_val;
};
*/

/*
struct shadow_data {	
	glm::mat4 proj;
	glm::mat4 view;
	glm::mat4 model;
	glm::mat4 light_space_mat;
	Shader *shader;
	float s_w;
	float s_h;
	float s_near;
	float s_far;	
	glm::vec3 lightDir;
	glm::vec3 lightColor;
	glm::vec3 lightPos;
};
*/

scene sponza;
shadow_data shadowmap;
quad screen_quad;
scene sphere_scene;

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
	const unsigned int SAMPLING_SIZE = 64; // rsm sampling
	float r_max = 0.05f;// max r for sampling pattern of rsm
	float rsm_w;
	float rsm_h;
} rsm_param;

/* ------ FUNCTION SIGNATURES ------ */
void generate_rsm_sampling_pattern(std::vector<glm::vec2>& p);

void render_debug_view(window& wnd, Shader& debug_program, quad& debug_quad, shadow_data& shadow_data, framebuffer& shadow_buffer);

//keyboard function
void kbfunc(GLFWwindow* window, int key, int scan, int action, int mods); 
//mouse function
void mfunc(GLFWwindow *window, double xpos, double ypos);

void shadow_pass(shadow_data& data, framebuffer& depth_buffer, mesh_loader& mesh);

void rsm_pass(Shader& RSM_pass, framebuffer& rsm, shadow_data& light_data);

void gbuffer_pass(framebuffer& gbuffer, Shader& gbuf_program, scene& s);

void deep_g_buffer_pass(Shader& dgb_program, framebuffer& dgbuffer, scene& s, float delta);

void deep_g_buffer_debug(Shader& debug_view, framebuffer& dgb, quad& screen, shadow_data&  light_data, scene& s, framebuffer& depth_buffer);

void blur_pass(quad& screen, unsigned int source_id, Shader& blur_program, framebuffer& blur_buffer);

/* GUI STUFF*/
static void glfw_error_callback(int error, const char* description)
{
	fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}
void init_gui(GLFWwindow *w) {
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
void render_gui(double delta) {
	// Start the Dear ImGui frame
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	//IMGUI scope
	{
		ImGui::Begin("CPU Timer");
		ImGui::Text("%f miliseconds per frame", delta);	
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

//debug instancing pos
std::vector<glm::vec3> gen_instance_positions() {
	std::vector<glm::vec3> pos(3);
	pos.emplace_back(glm::vec3(-44.1766, 10.5945, -0.764266));
	pos.emplace_back(glm::vec3(6.55214, 6.49787, -0.374851));
	pos.emplace_back(glm::vec3(48.5382, 5.71253, 0.289969));

	return pos;
}

//generate uniform distribution for RSM sampling
std::vector<glm::vec2> gen_uniform_samples(unsigned int s ) {
	std::uniform_real_distribution<float> randomFloats(0.0, 0.9);
	std::default_random_engine gen;
	std::vector<glm::vec2> samples(s);
	
	for (int i = 0; i < s; i++) {
		glm::vec2 val{0,0};
		val.x = randomFloats(gen);
		val.y = randomFloats(gen);
		samples[i]=val;
	}
	return samples;
}

void draw_stenciled_spheres(framebuffer& gbuffer, framebuffer& rsm_buffer, scene& sphere_scene, Model& sphere, Shader& sphereShader, GLuint sphereVAO, glm::vec3 mid, glm::vec2 coord) {
	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	sphereShader.use();

	glActiveTexture(GL_TEXTURE0);
	sphereShader.setInt("rsmposition", 0);
	glBindTexture(GL_TEXTURE_2D, rsm_buffer.pos);

	glActiveTexture(GL_TEXTURE1);
	sphereShader.setInt("rsmnormal", 1);
	glBindTexture(GL_TEXTURE_2D, rsm_buffer.normal);

	glActiveTexture(GL_TEXTURE2);
	sphereShader.setInt("rsmflux", 2);
	glBindTexture(GL_TEXTURE_2D, rsm_buffer.albedo);

	glActiveTexture(GL_TEXTURE3);
	sphereShader.setInt("gposition", 3);
	glBindTexture(GL_TEXTURE_2D, gbuffer.pos);

	glActiveTexture(GL_TEXTURE4);
	sphereShader.setInt("gnormal", 4);
	glBindTexture(GL_TEXTURE_2D, gbuffer.normal);

	glActiveTexture(GL_TEXTURE5);
	sphereShader.setInt("galbedo", 5);
	glBindTexture(GL_TEXTURE_2D, gbuffer.albedo);

	sphereShader.setMat4("P", sphere_scene.proj);
	sphereShader.setMat4("V", sphere_scene.view);

	sphereShader.setVec2("Coords", coord);

	glm::mat4 mod = glm::scale(glm::mat4(1.0), glm::vec3(1.0, 1.0, 1.0));
	glm::translate(mod, -mid);

	sphereShader.setMat4("M", mod);

	GLCall(glBindVertexArray(sphereVAO));
	GLCall(glDrawElementsInstanced(GL_TRIANGLES, sphere.get_indices().size(), GL_UNSIGNED_INT, 0, VPL_SAMPLES));
}

//Draw instanced spheres
void draw_spheres(framebuffer& gbuffer, framebuffer& rsm_buffer, scene& sphere_scene, Model& sphere, Shader& sphereShader, GLuint sphereVAO, glm::vec3 mid, glm::vec2 coord) {
	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	sphereShader.use();

	glActiveTexture(GL_TEXTURE0);
	sphereShader.setInt("rsmposition", 0);
	glBindTexture(GL_TEXTURE_2D, rsm_buffer.pos);

	glActiveTexture(GL_TEXTURE1);
	sphereShader.setInt("rsmnormal", 1);
	glBindTexture(GL_TEXTURE_2D, rsm_buffer.normal);

	glActiveTexture(GL_TEXTURE2);
	sphereShader.setInt("rsmflux", 2);
	glBindTexture(GL_TEXTURE_2D, rsm_buffer.albedo);

	glActiveTexture(GL_TEXTURE3);
	sphereShader.setInt("gposition", 3);
	glBindTexture(GL_TEXTURE_2D, gbuffer.pos);

	glActiveTexture(GL_TEXTURE4);
	sphereShader.setInt("gnormal", 4);
	glBindTexture(GL_TEXTURE_2D, gbuffer.normal);

	glActiveTexture(GL_TEXTURE5);
	sphereShader.setInt("galbedo", 5);
	glBindTexture(GL_TEXTURE_2D, gbuffer.albedo);

	sphereShader.setMat4("P", sphere_scene.proj);
	sphereShader.setMat4("V", sphere_scene.view);

	sphereShader.setVec2("Coords", coord);

	glm::mat4 mod = glm::scale(glm::mat4(1.0), glm::vec3(1.0, 1.0, 1.0));
	glm::translate(mod, -mid);

	

	sphereShader.setMat4("M", mod);

	GLCall(glBindVertexArray(sphereVAO));
	GLCall(glDrawElementsInstanced(GL_TRIANGLES, sphere.get_indices().size(), GL_UNSIGNED_INT, 0, VPL_SAMPLES));
}

void create_sphere_vao(GLuint& sphereVAO, GLuint& sphereVBO, GLuint& sphereIBO, GLuint& instanceVBO, Model& sphere, std::vector<glm::vec2>& samples);

int main(int argc, char **argv) {	

	//TODO:
	//1 - SAMPLE RSM
	//2 - DRAW INSTANCED SPHERES
	//3 - RENDER INDIRECT LIGHTING (DACHSBACHER2006) or Tiled Shading
	//4 - RENDER CUBEMAP
	//5 - REFLECT VPLS
		
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
	glfwSwapInterval(0);
	
	/*--------IMGUI INITIALIZATION---------*/	
	init_gui(w.wnd);

	/*------- LOAD SPHERICAL MESH---------*/
	mesh_loader m(sphere_path);
	Model sphere = m.get_mesh();
	Shader sphere_shader("shaders/spheres_vert.glsl", "shaders/spheres_debug.glsl");
	glm::vec3 sphere_mid = m.bb_mid;
	std::cout << "center of sphere is " << sphere_mid.x << " " << sphere_mid.y << " " << sphere_mid.z  << std::endl;

	//Resize sponza
	sponza.mesh = new mesh_loader(sponza_path);
	sponza.bb_mid = sponza.mesh->bb_mid * 0.05f;
	sponza.model = glm::scale(glm::mat4(1.0), glm::vec3(0.05f));
	sponza.proj = glm::perspective(glm::radians(fov), Wid/Hei, n_v, f_v);
	sponza.n_val = n_v;
	sponza.f_val = f_v;

	//sponza.bb_mid + glm::vec3(0, 20, 0)
	glm::vec3 eyePos = glm::vec3(0, 0, 0);
	sponza.shader = new Shader("shaders/screen_quad_vert.glsl", "shaders/sponza_frag.glsl", nullptr);
	sponza.camera = new Camera(eyePos);
	sponza.view = sponza.camera->GetViewMatrix();
		
	/*--- SHADOW MAP LIGHT TRANSFORMS---*/		
	shadowmap.s_w = 512.0f;
	shadowmap.s_h = 512.0f;
	shadowmap.s_near = 0.1f;
	shadowmap.s_far = 400.0f;

	//previous -48 / 48 -15 / 15 or -60 60 -50 50
	shadowmap.proj = glm::ortho(-48.0f, 48.0f, -15.0f, 15.0f, shadowmap.s_near, shadowmap.s_far);
	
	glm::vec3 l_pos = sponza.bb_mid + glm::vec3(0, 100, 0); // light position
	shadowmap.lightPos = l_pos;
	glm::vec3 l_center = sponza.bb_mid; //light looking at
	glm::vec3 worldup(0.0, 1.0, 0.2); //movendo ligeiramente o vetor up, caso contrario teriamos direcao e up colineares, a matriz seria degenerada
	shadowmap.view = glm::lookAt(l_pos, l_center, worldup);
	lightDir = glm::normalize(l_pos - l_center);	
	shadowmap.lightDir = lightDir;
	shadowmap.lightColor = glm::vec3(0.5f);

	shadowmap.shader = new Shader("shaders/shadow_map_vert.glsl", "shaders/shadow_map_frag.glsl", nullptr);
	glm::mat4 lightspacemat;

	/*----SHADOW MAP ---- RSM -----*/
	Shader depthView("shaders/screen_quad_vert.glsl", "shaders/depth_view_frag.glsl", nullptr);
	framebuffer depthBuffer(fbo_type::SHADOW_MAP, shadowmap.s_w, shadowmap.s_h, 0);	
	
	/*----OPENGL Enable/Disable functions----*/
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	//glClearColor(0.5, 0.8, 0.9, 1.0); 	
	glClearColor(0.0, 0.0, 0.0, 1.0); 

	/* ---------------- G-BUFFER ----------------*/
	framebuffer gbuffer(fbo_type::G_BUFFER_V2, w.Wid, w.Hei, 0);
	Shader geometry_pass ("shaders/deferred_render_vert.glsl", "shaders/deferred_render_frag.glsl");
	
	/* -------------- RSM BUFFER ----------------*/		
	rsm_param.rsm_w = 512.0f;
	rsm_param.rsm_h = 512.0f;
	
	framebuffer rsm_buffer(fbo_type::RSM, rsm_param.rsm_w, rsm_param.rsm_h, 0);
	Shader RSM_pass("shaders/rsm_vert.glsl", "shaders/rsm_frag.glsl");

	/* ---- STOCHASTIC SAMPLING OF RSM MAP ---- */
	generate_rsm_sampling_pattern(rsm_param.pattern);

	/* ------------ DEEP G BUFFER - 2 LAYER ----------*/
	float Delta = 0.5f;
	const int Layers = 2;
	framebuffer deepgbuffer(fbo_type::DEEP_G_BUFFER,  Wid, Hei, Layers);
	Shader dgb_program("shaders/deep_g_buffer_vert.glsl", "shaders/deep_g_buffer_frag.glsl", "shaders/deep_g_buffer_geom.glsl");
	Shader dgb_debug_view("shaders/screen_quad_vert.glsl","shaders/dgb_debug_view_frag.glsl");
	
	/* ---- BLUR ----*/
	Shader blur_program("shaders/screen_quad_vert.glsl", "shaders/blur_pass_frag.glsl");
	framebuffer blur(fbo_type::COLOR_BUFFER, Wid, Hei, 0);

	/*-----STENCIL RENDERING-PROGRAM-----*/
	Shader stencil_pass("shaders/mvp_pass_vert.glsl", "shaders/dummy_rendering_frag.glsl");
	
	/*----FILL SSBO WITH LIGHTS ----*/
	GLuint lightSSBO;
	bool is_filled = false;
	GLCall(glGenBuffers(1, &lightSSBO));
	GLCall(glBindBuffer(GL_SHADER_STORAGE_BUFFER, lightSSBO));
	GLCall(glBufferData(GL_SHADER_STORAGE_BUFFER, VPL_SAMPLES * sizeof(struct point_light) , NULL , GL_DYNAMIC_COPY));
		
	Shader gen_light_buffer(nullptr, nullptr, nullptr, "shaders/gen_light_ssbo.glsl");
	
	//Shader tiled_shading(nullptr, nullptr, nullptr, "shaders/compute_nothing.glsl");
	
	shadowmap.model = sponza.model;

	/*--- SPHERE VBO CREATION ---*/
	GLuint sphereVAO;
	GLuint sphereVBO;
	GLuint sphereIBO;
	GLuint instanceVBO;
	GLuint samplesBuffer;
	GLuint samplesTBO;

	std::vector<glm::vec2> samples = gen_uniform_samples(VPL_SAMPLES);
	create_sphere_vao(sphereVAO, sphereVBO, sphereIBO, instanceVBO ,sphere, samples);

	GLCall(glGenBuffers(1, &samplesBuffer));
	GLCall(glBindBuffer(GL_TEXTURE_BUFFER, samplesBuffer));
	GLCall(glBufferData(GL_TEXTURE_BUFFER, sizeof(glm::vec2)*samples.size(), samples.data(), GL_STATIC_READ));

	GLCall(glGenTextures(1, &samplesTBO));
	GLCall(glBindTexture(GL_TEXTURE_BUFFER, samplesTBO));
	GLCall(glTexBuffer(GL_TEXTURE_BUFFER, GL_RG32F, samplesBuffer));
	/*------------------------------------------------------*/
	
	double t0, tf, delta_t;
	t0 = tf = delta_t = 0;
	t0 = glfwGetTime();

	Shader dLightProgram("shaders/screen_quad_vert.glsl", "shaders/gbuffer_shade_frag.glsl");
	//main loop
	while (!w.should_close()) {			
				
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);				
				
		//glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		glDepthMask(GL_TRUE);
		glEnable(GL_DEPTH_TEST);		
				
		/*---- SHADOW MAP PASS ----*/
		shadow_pass(shadowmap, depthBuffer, *sponza.mesh);
		/*------- LIGHT SPACE MATRIX -------*/
		shadowmap.light_space_mat = shadowmap.proj * shadowmap.view;				
		/*---------RSM_PASS--------*/
		rsm_pass(RSM_pass, rsm_buffer, shadowmap);					
		/* -------G BUFFER PASS------*/			
		gbuffer_pass(gbuffer, geometry_pass, sponza);

		glDepthMask(GL_FALSE);
		glDisable(GL_DEPTH_TEST);

		/*-----ACTUAL RENDERING-----*/
		if (!debug_view)
		{			
			
			//COMPUTE LIGHT BUFFER STEP
			if (!is_filled) 
			{
				std::cout << "ssbo is empty, will generate vpl buffer" << std::endl;
				gen_light_buffer.use();

				GLCall(glActiveTexture(GL_TEXTURE0));
				GLCall(glBindTexture(GL_TEXTURE_2D, rsm_buffer.pos));
				gen_light_buffer.setInt("rsm_position", 0);				
				
				GLCall(glActiveTexture(GL_TEXTURE1));
				GLCall(glBindTexture(GL_TEXTURE_2D, rsm_buffer.albedo));
				gen_light_buffer.setInt("rsm_flux", 1);
				
				GLCall(glBindImageTexture(0, samplesTBO, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RG32F));
				GLCall(glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, lightSSBO));

				GLCall(glDispatchCompute(VPL_SAMPLES, 1 ,1));
				GLCall(glMemoryBarrier(GL_ALL_BARRIER_BITS));
				is_filled = true;
			}

			//BEGIN LIGHT PASS
		
			//STENCIL PASS
			//glEnable(GL_STENCIL_TEST);	
			//gbuffer.set_stencil_pass();
			//glEnable(GL_DEPTH_TEST);
			//glDisable(GL_CULL_FACE);
			//glClear(GL_STENCIL_BUFFER_BIT);
			//glStencilFunc(GL_ALWAYS, 0, 0);			
			//glStencilOpSeparate(GL_BACK, GL_KEEP, GL_INCR_WRAP, GL_KEEP);
			//glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_DECR_WRAP, GL_KEEP);	
			//draw_stenciled_spheres(gbuffer, rsm_buffer, sponza, sphere, stencil_pass, sphereVAO, sphere_mid, glm::vec2(Wid, Hei));	

			/*----------------------------------------------------------------------------------*/

			//void do_instanced_sphere_pass();
			//do sphere light pass here
			gbuffer.set_intermediate_pass();
			glStencilFunc(GL_NOTEQUAL, 0, 0xFF);
			glDisable(GL_DEPTH_TEST);			
			glEnable(GL_BLEND);
			glBlendEquation(GL_FUNC_ADD);
			glBlendFunc(GL_ONE, GL_ONE);
			glEnable(GL_CULL_FACE);
			glCullFace(GL_FRONT);
			draw_spheres(gbuffer, rsm_buffer, sponza, sphere, sphere_shader, sphereVAO, sphere_mid, glm::vec2(Wid, Hei));
			glCullFace(GL_BACK);
			glDisable(GL_BLEND);
			glDisable(GL_STENCIL_TEST);										
						
			/*------------------------------------------------------------------------*/
			//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			//Directional light shading pass
			gbuffer.set_intermediate_pass();
			glDisable(GL_DEPTH_TEST);
			glEnable(GL_BLEND);
			glBlendEquation(GL_FUNC_ADD);
			glBlendFunc(GL_ONE, GL_ONE);
			dLightProgram.use();
			dLightProgram.setVec3("lightDir", shadowmap.lightDir);
			dLightProgram.setVec3("eyePos", sponza.camera->Position);
			dLightProgram.setMat4("LightSpaceMat", shadowmap.light_space_mat);
			//gbuffer pos
			GLCall(glActiveTexture(GL_TEXTURE0));
			dLightProgram.setInt("gposition", 0);
			glBindTexture(GL_TEXTURE_2D, gbuffer.pos);
			//gbuffer normal
			GLCall(glActiveTexture(GL_TEXTURE1));
			dLightProgram.setInt("gnormal", 1);
			glBindTexture(GL_TEXTURE_2D, gbuffer.normal);
			//gbuffer albedo + spec
			GLCall(glActiveTexture(GL_TEXTURE2));
			dLightProgram.setInt("galbedo", 2);
			glBindTexture(GL_TEXTURE_2D, gbuffer.albedo);
			//shadow map
			GLCall(glActiveTexture(GL_TEXTURE3));
			dLightProgram.setInt("shadow_map", 3);
			glBindTexture(GL_TEXTURE_2D, depthBuffer.depth_map);
			screen_quad.renderQuad();
			glDisable(GL_BLEND);

			
			/*-------------------------------------------------------------------------*/
						
			glBindFramebuffer(GL_READ_FRAMEBUFFER, gbuffer.fbo);
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
			//intermediate buffer
			glReadBuffer(GL_COLOR_ATTACHMENT4);
			glBlitFramebuffer(0, 0, Wid, Hei, 0, 0, Wid, Hei, GL_COLOR_BUFFER_BIT, GL_NEAREST);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
						
			

			
		}
		else		
		{
			/* ---- DEBUG VIEW ---- */
			render_debug_view(w, depthView, screen_quad, shadowmap, depthBuffer);			
		}
		
		//delta time and gui frametime renderer;
		{
			tf = glfwGetTime();
			delta_t = (tf - t0) * 1000.0;
			t0 = tf;
			render_gui(delta_t);
		}

		w.swap();	
		w.poll_ev();
	}		
	

	destroy_gui();
	
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
		sponza.camera->print_camera_coords();
		//On pressing B, light will be updated to the current position, direction  and up of the camera
		//lightDir = -sponza.camera->Front;
		//ref_pos = sponza.camera->Position;
		//ref_up = sponza.camera->Up;
		//ref_front = sponza.camera->Front;
		//shadowmap.view = glm::lookAt(ref_pos, ref_front, ref_up);
		
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

void render_debug_view(window& wnd, Shader& debug_program, quad& debug_quad, shadow_data& shadow_data, framebuffer& shadow_buffer) {
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

void shadow_pass(shadow_data& data, framebuffer& depth_buffer, mesh_loader& mesh) {

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

void rsm_pass(Shader& RSM_pass, framebuffer& rsm, shadow_data& light_data) {
	/*---------RSM_Pass--------*/
	RSM_pass.use();
	RSM_pass.setMat4("P", light_data.proj);
	RSM_pass.setMat4("V", light_data.view);
	RSM_pass.setMat4("M", light_data.model);
	RSM_pass.setFloat("lightColor", 0.65f);
	RSM_pass.setVec3("lightDir", light_data.lightDir);
	rsm.bind();
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	sponza.mesh->Draw(RSM_pass);
	rsm.unbind();
}

void gbuffer_pass(framebuffer& gbuffer, Shader& gbuf_program, scene& s) {

	glViewport(0, 0, gbuffer.w_res, gbuffer.h_res);
	gbuffer.bind();
	gbuffer.set_intermediate_pass();
	glClear(GL_COLOR_BUFFER_BIT);
	gbuffer.set_geometry_pass();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	s.view = s.camera->GetViewMatrix();
	gbuf_program.use();
	gbuf_program.setMat4("M", s.model);
	gbuf_program.setMat4("V", s.view);
	gbuf_program.setMat4("P", s.proj);
	s.mesh->Draw(gbuf_program);
	//gbuffer.unbind();
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

void deep_g_buffer_debug(Shader& debug_view, framebuffer& dgb, quad& screen, shadow_data&  light_data, scene& s, framebuffer& depth_buffer) {
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

void blur_pass(quad& screen,unsigned int source_id, Shader& blur_program, framebuffer& blur_buffer) {
	
	blur_buffer.bind();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	blur_program.use();

	GLCall(glActiveTexture(GL_TEXTURE0));
	blur_program.setInt("source", 0);
	GLCall(glBindTexture(GL_TEXTURE_2D, source_id));

	screen.renderQuad();
	blur_buffer.unbind();
}

void create_sphere_vao(GLuint& sphereVAO, GLuint& sphereVBO, GLuint& sphereIBO, GLuint& instanceVBO , Model& sphere, std::vector<glm::vec2>& samples) {
	GLCall(glGenVertexArrays(1, &sphereVAO));
	GLCall(glGenBuffers(1, &sphereVBO));
	GLCall(glGenBuffers(1, &sphereIBO));

	GLCall(glBindVertexArray(sphereVAO));
	int size = 0;
	GLCall(glBindBuffer(GL_ARRAY_BUFFER, sphereVBO));
	size = sphere.get_verts().size() * sizeof(vertex);
	GLCall(glBufferData(GL_ARRAY_BUFFER, size, sphere.get_verts().data(), GL_STATIC_DRAW));

	GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereIBO));
	size = sphere.get_indices().size() * sizeof(unsigned int);
	GLCall(glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, sphere.get_indices().data(), GL_STATIC_DRAW));

	GLCall(glEnableVertexAttribArray(0));
	GLCall(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)0));

	GLCall(glEnableVertexAttribArray(1));
	GLCall(glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)offsetof(vertex, uv)));

	GLCall(glEnableVertexAttribArray(2));
	GLCall(glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)offsetof(vertex, normal)));
	GLCall(glBindVertexArray(0));
	
	//std::vector<glm::vec2> samples = gen_uniform_samples(VPL_SAMPLES);
	//if (samples.size() > 0) {
	//	std::cout << "generated " << samples.size() << " samples succesfully" << std::endl; 
	//}
		
	GLCall(glGenBuffers(1, &instanceVBO));
	GLCall(glBindBuffer(GL_ARRAY_BUFFER, instanceVBO));
	GLCall(glBufferData(GL_ARRAY_BUFFER, samples.size() * sizeof(glm::vec2), samples.data(), GL_STATIC_DRAW));
	GLCall(glBindBuffer(GL_ARRAY_BUFFER, 0));

	GLCall(glBindVertexArray(sphereVAO));
	GLCall(glEnableVertexAttribArray(3));
	GLCall(glBindBuffer(GL_ARRAY_BUFFER, instanceVBO));
	GLCall(glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0));
	GLCall(glBindBuffer(GL_ARRAY_BUFFER, 0));
	GLCall(glVertexAttribDivisor(3, 1));
	GLCall(glBindVertexArray(0));
}