#include "PrototypeIncludes.h"

//GLOBAL CONSTANTS
const float PI = 3.1415926f;
const float PI_TWO = 6.2831853;
const int VPL_SAMPLES = 8 * 8; //number of lights
const int MaxVPL = VPL_SAMPLES * 2;
//scene and model paths
const char* sponza_path = "models/sponza/sponza.obj";
const char* sphere_path = "models/sphere/sphere.obj";

scene sponza;
shadow_data shadowmap;
quad screen_quad;
Cube test_cube;
scene sphere_scene;

/*---camera controls---*/
double lastX = 0, lastY = 0;
bool active_cam = true;
bool view_vpls = false;
bool debug_view = false;

glm::vec3 ref_up;
glm::vec3 ref_front;
glm::vec3 ref_pos;
glm::vec3 lightPos;
glm::vec3 lightDir;

void render_debug_view(window& wnd, Shader& debug_program, quad& debug_quad, shadow_data& shadow_data, framebuffer& shadow_buffer);

//keyboard function
void kbfunc(GLFWwindow* window, int key, int scan, int action, int mods); 
//mouse function
void mfunc(GLFWwindow *window, double xpos, double ypos);


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

int main(int argc, char **argv) {	

	//DONE:
	// - SAMPLE RSM
	// - DRAW INSTANCED SPHERES
	// - RENDER INDIRECT LIGHTING (DACHSBACHER2006) or Tiled Shading
	// - RENDER CUBEMAP
	// - REFLECT VPLS
	//TODO:
	//1 - CLUSTER VPLS
	//2 - CUBEMAP VS CLUSTER VISIBILITY
	//3 - FIX SHADING
 			
	/*-----parameters----*/
	float Wid = 1280.0f; 
	float Hei = 720.0f;	
	const float fov = 60.0f;
	const float n_v = 1.0f;	
	const float f_v = 400.f;

	/*----SET WINDOW AND CALLBACKS ----*/	
	window w;
	
	int res = w.create_window(Wid, Hei);	
	if (res != 0)
	{
		return res;
	}	
		
	w.set_kb(kbfunc);
	w.set_mouse(mfunc);
	//vsync off
	glfwSwapInterval(0);
	
	/*--------IMGUI INITIALIZATION---------*/	
	init_gui(w.wnd);

	/*----OPENGL Enable/Disable functions----*/
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	glClearColor(0.0, 0.0, 0.0, 1.0);

	//Resize sponza
	sponza.mesh = new mesh_loader(sponza_path);
	sponza.bb_mid = sponza.mesh->bb_mid * 0.05f;
	sponza.model = glm::scale(glm::mat4(1.0), glm::vec3(0.05f));
	sponza.proj = glm::perspective(glm::radians(fov), Wid/Hei, n_v, f_v);
	glm::mat4 invProj = glm::inverse(sponza.proj);
	sponza.n_val = n_v;
	sponza.f_val = f_v;

	glm::vec3 eyePos = sponza.bb_mid;	
	sponza.shader = new Shader("shaders/screen_quad_vert.glsl", "shaders/sponza_frag.glsl", nullptr);
	sponza.camera = new Camera(eyePos);
	sponza.view = sponza.camera->GetViewMatrix();
			
	/*--- SHADOW MAP AND RSM LIGHT TRANSFORMS---*/		
	shadowmap.s_w = 512.0f;
	shadowmap.s_h = 512.0f;
	shadowmap.s_near = 0.1f;
	shadowmap.s_far = 400.0f;		
	shadowmap.proj = glm::ortho(-46.0f, 46.0f, -10.0f, 10.0f, shadowmap.s_near, shadowmap.s_far);
	
	glm::vec3 l_pos = sponza.bb_mid + glm::vec3(0, 100, 0); // light position
	shadowmap.lightPos = l_pos;
	glm::vec3 l_center = sponza.bb_mid; //light looking at
	glm::vec3 worldup(0.0, 1.0, 0.2); //movendo ligeiramente o vetor up, caso contrario teriamos direcao e up colineares, a matriz seria degenerada
	shadowmap.view = glm::lookAt(l_pos, l_center, worldup);
	lightDir = glm::normalize(l_pos - l_center);	
	shadowmap.lightDir = lightDir;
	shadowmap.lightColor = glm::vec3(0.5f);
	shadowmap.model = sponza.model;
	shadowmap.shader = new Shader("shaders/shadow_map_vert.glsl", "shaders/shadow_map_frag.glsl", nullptr);
	glm::mat4 lightspacemat;

	/*----SHADOW MAP DEBUG VIEW -----*/
	Shader depthView("shaders/screen_quad_vert.glsl", "shaders/depth_view_frag.glsl", nullptr);
	framebuffer depthBuffer(fbo_type::SHADOW_MAP, shadowmap.s_w, shadowmap.s_h, 0);	
	
	/* ---------------- G-BUFFER ----------------*/
	framebuffer gbuffer(fbo_type::G_BUFFER, w.Wid, w.Hei, 0);
	Shader geometry_pass ("shaders/deferred_render_vert.glsl", "shaders/deferred_render_frag.glsl");
	
	/* -------------- RSM FRAMEBUFFER AND SHADER----------------*/			
	framebuffer rsm_buffer(fbo_type::RSM, shadowmap.s_w, shadowmap.s_h, 0);
	Shader RSM_pass("shaders/rsm_vert.glsl", "shaders/rsm_frag.glsl");
		
	/*----FILL SSBO WITH LIGHTS ----*/
	
	unsigned int size = MaxVPL * sizeof(point_light);
	shader_storage_buffer lightSSBO(size);
	bool is_filled = false;
	//GLuint lightSSBO;
	//GLCall(glGenBuffers(1, &lightSSBO));
	//GLCall(glBindBuffer(GL_SHADER_STORAGE_BUFFER, lightSSBO));
	//GLCall(glBufferData(GL_SHADER_STORAGE_BUFFER, MaxVPL * sizeof(point_light) , NULL , GL_DYNAMIC_COPY));
		
	/* ---- COMPUTE SHADERS ---- */
	Shader gen_light_buffer(nullptr, nullptr, nullptr, "shaders/gen_light_ssbo.glsl");	
	Shader tiled_shading(nullptr, nullptr, nullptr, "shaders/tiled_shading.glsl");	
	Shader ssvp(nullptr, nullptr, nullptr,  "shaders/SSVP.glsl");
	
	GLuint samplesBuffer;
	GLuint samplesTBO;

	//RSM sampling pattern
	std::vector<glm::vec2> samples = gen_uniform_samples(VPL_SAMPLES, 0.11f, 0.9f);	
	
	GLCall(glGenBuffers(1, &samplesBuffer));
	GLCall(glBindBuffer(GL_TEXTURE_BUFFER, samplesBuffer));
	GLCall(glBufferData(GL_TEXTURE_BUFFER, sizeof(glm::vec2)*samples.size(), samples.data(), GL_DYNAMIC_READ));

	GLCall(glGenTextures(1, &samplesTBO));
	GLCall(glBindTexture(GL_TEXTURE_BUFFER, samplesTBO));
	GLCall(glTexBuffer(GL_TEXTURE_BUFFER, GL_RG32F, samplesBuffer));
	/*------------------------------------------------------*/
			
	//drwa texture , image unit to be shown
	GLuint draw_tex;
	GLCall(glGenTextures(1, &draw_tex));
	GLCall(glBindTexture(GL_TEXTURE_2D, draw_tex));
	GLCall(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, Wid, Hei, 0, GL_RGBA, GL_FLOAT, NULL));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));	
	GLCall(glBindTexture(GL_TEXTURE_2D, 0));
	Shader dLightProgram("shaders/screen_quad_vert.glsl", "shaders/gbuffer_shade_frag.glsl");
	Shader blit("shaders/screen_quad_vert.glsl", "shaders/blit_texture_frag.glsl");	
	Shader skybox("shaders/skybox_vert.glsl", "shaders/skybox_frag.glsl");
		
	//Cube Map data	
	glm::vec2 cube_res(256.f, 256.f);
	glm::mat4 cubeModel = sponza.model;
	glm::mat4 cubeProj = glm::perspective(glm::radians(90.0f), cube_res.x/cube_res.y, 1.f, 400.0f);	
	glm::mat4 cubeView[6];	
	glm::vec3 pos = sponza.mesh->bb_mid * 0.05f;

	cubeView[0] = glm::lookAt(pos, pos + glm::vec3( 1.0, 0.0, 0.0),  glm::vec3(0.0,  -1.0,  0.0));
	cubeView[1] = glm::lookAt(pos, pos + glm::vec3(-1.0, 0.0, 0.0),  glm::vec3(0.0,  -1.0,  0.0));
	cubeView[2] = glm::lookAt(pos, pos + glm::vec3( 0.0, 1.0, 0.0),  glm::vec3(0.0,  0.0,  1.0));
	cubeView[3] = glm::lookAt(pos, pos + glm::vec3(0.0, -1.0, 0.0),  glm::vec3(0.0,  0.0, -1.0));
	cubeView[4] = glm::lookAt(pos, pos + glm::vec3(0.0,  0.0,  1.0), glm::vec3(0.0,  -1.0,  0.0));
	cubeView[5] = glm::lookAt(pos, pos + glm::vec3(0.0,  0.0, -1.0), glm::vec3(0.0,  -1.0,  0.0));	
	
	Shader cube_rendering("shaders/vertex_passthru_vert.glsl", "shaders/cube_layered_frag.glsl", "shaders/cube_layered_geom.glsl");
	framebuffer cube(fbo_type::CUBE_MAP, cube_res.x, cube_res.y, 0);
	uniform_buffer cube_buf(6 * sizeof(glm::mat4));
	cube_buf.upload_data(&cubeView[0]);

	cube.bind();
	glViewport(0, 0, cube_res.x, cube_res.y);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	cube_rendering.use();
	unsigned int mat_index = glGetUniformBlockIndex(cube_rendering.ID, "Matrices");
	glUniformBlockBinding(cube_rendering.ID, mat_index, 0);
	cube_buf.set_binding_point(0);
	cube_rendering.setMat4("M", cubeModel);
	cube_rendering.setMat4("P", cubeProj);
	sponza.mesh->Draw(cube_rendering);
	cube.unbind();
	
	/*-----uniform sampling inside circular patch oriented by axis-----*/
	GLuint vector_offset_buffer;
	GLuint vector_offset_tbo;
	std::vector<glm::vec2> vector_offset_samples = gen_uniform_samples((MaxVPL - VPL_SAMPLES), 0.0f, 1.0f);
	//for (auto i : vector_offset_samples)
	//{
	//	std::cout << glm::to_string(i) << std::endl;
	//}
	GLCall(glGenBuffers(1, &vector_offset_buffer));
	GLCall(glBindBuffer(GL_TEXTURE_BUFFER, vector_offset_buffer));
	GLCall(glBufferData(GL_TEXTURE_BUFFER, sizeof(glm::vec2)*vector_offset_samples.size(), vector_offset_samples.data(), GL_DYNAMIC_READ));

	GLCall(glGenTextures(1, &vector_offset_tbo));
	GLCall(glBindTexture(GL_TEXTURE_BUFFER, vector_offset_tbo));
	GLCall(glTexBuffer(GL_TEXTURE_BUFFER, GL_RG32F, vector_offset_buffer));
		

	Shader ssbo_debug("shaders/ssbo_debug_draw_vert.glsl","shaders/ssbo_debug_draw_frag.glsl");
	GLuint lightvao;
	GLCall(glGenVertexArrays(1, &lightvao));
	GLCall(glBindVertexArray(lightvao));
	GLCall(glBindBuffer(GL_ARRAY_BUFFER, lightSSBO.ssbo));
	GLCall(glEnableVertexAttribArray(0));
	GLCall(glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(point_light), (void*)0));
	GLCall(glBindVertexArray(0));
	glEnable(GL_PROGRAM_POINT_SIZE);

	double t0, tf, delta_t;
	t0 = tf = delta_t = 0;
	t0 = glfwGetTime();
	//main loop
	while (!w.should_close()) {			
								
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		glDepthMask(GL_TRUE);
		glEnable(GL_DEPTH_TEST);		
				
		/*---- SHADOW MAP AND RSM PASS ----*/
		//shadow_pass(shadowmap, depthBuffer, *sponza.mesh);
		/*------- LIGHT SPACE MATRIX -------*/
		shadowmap.light_space_mat = shadowmap.proj * shadowmap.view;
		
		/*---------RSM_PASS--------*/
		rsm_pass(RSM_pass, rsm_buffer, shadowmap, sponza);	

		/* -------G BUFFER PASS------*/			
		gbuffer_pass(gbuffer, geometry_pass, sponza);

		glDepthMask(GL_FALSE);
		glDisable(GL_DEPTH_TEST);

		/*-----ACTUAL RENDERING-----*/
		if (!debug_view)
		{						
			//FILL LIGHT SSBO 
			if (!is_filled) 
			{  
				fill_lightSSBO(rsm_buffer, sponza.camera->GetViewMatrix(), gen_light_buffer, samplesTBO, lightSSBO);
				is_filled = true;
			}						
			
			glm::vec4 refPos = glm::vec4((sponza.camera->Position), glm::radians(75.0f));
			do_SSVP(ssvp, lightSSBO, vector_offset_tbo, cube, refPos, VPL_SAMPLES, MaxVPL);
						
			do_tiled_shading(tiled_shading, gbuffer, rsm_buffer, draw_tex,
				              invProj, sponza, shadowmap, MaxVPL, lightSSBO, Wid, Hei);
			

			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			
			blit.use();
			GLCall(glActiveTexture(GL_TEXTURE0));
			GLCall(glBindTexture(GL_TEXTURE_2D, draw_tex));
			blit.setInt("texImage", 0);
			GLCall(glActiveTexture(GL_TEXTURE1));
			GLCall(glBindTexture(GL_TEXTURE_2D, gbuffer.depth_map));
			blit.setInt("depthImage", 1);			
			blit.setFloat("near", n_v);
			blit.setFloat("far", f_v);						
			screen_quad.renderQuad();	
			
			if (view_vpls)
			{
				GLCall(glBindVertexArray(lightvao));
				ssbo_debug.use();
				glm::mat4 MVP = sponza.proj * sponza.camera->GetViewMatrix();
				ssbo_debug.setMat4("MVP", MVP);
				glDrawArrays(GL_POINTS, 0, MaxVPL);
				GLCall(glBindVertexArray(0));
			}
			

		}
		else		
		{				
			/* ---- DEBUG VIEW ---- */
			//render_debug_view(w, depthView, screen_quad, shadowmap, rsm_buffer);	
			//debug cube draw
			draw_skybox(skybox, cube, sponza.camera->GetViewMatrix(), sponza.proj, test_cube);
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
		//std::cout << to_string(sponza.camera->Position) << std::endl;
	}

	if (key == GLFW_KEY_A && (action == GLFW_PRESS || action == GLFW_REPEAT))
	{
		sponza.camera->ProcessKeyboard(LEFT, 1);
		//std::cout << to_string(sponza.camera->Position) << std::endl;
	}

	if (key == GLFW_KEY_S && (action == GLFW_PRESS || action == GLFW_REPEAT))
	{
		sponza.camera->ProcessKeyboard(BACKWARD, 1);
		//std::cout << to_string(sponza.camera->Position) << std::endl;
	}

	if (key == GLFW_KEY_D && (action == GLFW_PRESS || action == GLFW_REPEAT))
	{
		sponza.camera->ProcessKeyboard(RIGHT, 1);
		//std::cout << to_string(sponza.camera->Position) << std::endl;
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
		view_vpls = !view_vpls;
	}

	if (key == GLFW_KEY_O && (action == GLFW_PRESS)) {
		debug_view = !debug_view;
	}

	if (key == GLFW_KEY_U && (action == GLFW_PRESS)) {
		
	}
	if (key == GLFW_KEY_J && (action == GLFW_PRESS)) {
		
	}

	if (key == GLFW_KEY_0 && (action == GLFW_PRESS)) {
		
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
		//std::cout << "camera yaw"   << sponza.camera->Yaw  << std::endl;
		//std::cout << "camera pitch" << sponza.camera->Pitch << std::endl;
	
	}
	else if (state == GLFW_RELEASE) {
		active_cam = true;
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




