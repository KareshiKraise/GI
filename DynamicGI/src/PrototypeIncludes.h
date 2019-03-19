#pragma once
#include <iostream>
#include <sstream>
#include <string>
#include <random>
#include <GL/glew.h>

#define GLM_ENABLE_EXPERIMENTAL

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/rotate_vector.hpp>
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
#include "shader_storage_buffer.h"