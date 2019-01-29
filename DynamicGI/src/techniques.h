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
	glm::vec4 p; //position	
	glm::vec4 c;//color
	float rad;
};

/*
void draw_stenciled_spheres(framebuffer& gbuffer, framebuffer& rsm_buffer, scene& sphere_scene, Model& sphere, Shader& sphereShader, GLuint sphereVAO, glm::vec3 mid, glm::vec2 coord);

void do_stencil_pass(framebuffer& gbuffer, const framebuffer& rsm_buffer, const scene& sponza, const Shader& stencil_pass, const GLuint sphereVAO, const glm::vec3& sphere_mid, const glm::vec2& dims);
*/