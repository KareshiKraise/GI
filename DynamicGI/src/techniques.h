#pragma once
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

extern const float PI;
extern const float PI_TWO;
extern const int VPL_SAMPLES;

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
	glm::vec3 bb_mid;	
	glm::mat4 view;
	glm::mat4 proj;
	glm::mat4 model;
	float n_val;
	float f_val;
};

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

struct point_light {
	glm::vec4 p; //position	 + radius
	glm::vec4 n;
	glm::vec4 c;//color	
};

void do_SSVP(Shader& SSVP, GLuint lightSSBO, GLuint samplesTBO, const framebuffer& cubemap, glm::vec4 refPos, int NumLights, int MaxLights);

glm::mat4 axis_from_rotation(float yaw, float pitch, glm::vec3 eye = glm::vec3(0.0));

void fill_lightSSBO(const framebuffer& buffer, const glm::mat4& view, Shader& ssbo_program ,GLuint tbo, GLuint ssbo);

void draw_skybox(Shader& skybox_program, const framebuffer& cube_buf, glm::mat4 view, glm::mat4 proj, Cube& cube);

/*
void draw_stenciled_spheres(framebuffer& gbuffer, framebuffer& rsm_buffer, scene& sphere_scene, Model& sphere, Shader& sphereShader, GLuint sphereVAO, glm::vec3 mid, glm::vec2 coord);

void do_stencil_pass(framebuffer& gbuffer, const framebuffer& rsm_buffer, const scene& sponza, const Shader& stencil_pass, const GLuint sphereVAO, const glm::vec3& sphere_mid, const glm::vec2& dims);
*/