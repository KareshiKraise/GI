#include "PrototypeIncludes.h"

//GLOBAL CONSTANTS
const float PI = 3.1415926f;
const float PI_TWO = 6.2831853;
//int VPL_SAMPLES = 16 * 16; /
int VPL_SAMPLES = 8 * 8; //number of lights


//scene and model paths
const char* sponza_path = "models/sponza/sponza.obj";
const char* sphere_path = "models/sphere/sphere.obj";
const char* cornell_path = "models/cornell/CornellBox-Original.obj";

scene sponza; 
scene cornell_box;
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

float gdelta_t = 0.f;

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

	//TODO 
	// 1 - IMPLEMENTAR TROCA DE MODELOS EM RUNTIME, MODO ATUAL É HORRIVEL
 			
	/*-----parameters----*/
	float Wid = 1280.0f; 
	float Hei = 720.0f;	

	const float fov = 60.0f;
	const float n_v = 0.1f;	
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

	//cornel
	
	
	mesh_loader cbox(cornell_path, model_type::NO_TEXTURE);
	glm::mat4 cbox_model = glm::scale(glm::mat4(1.0), glm::vec3(10.0f));
	glm::vec3 cbox_mid = cbox.bb_mid;
	glm::vec3 cbox_eyePos = glm::vec3(-2.67726, 75, 126.931);//cbox_mid;
	glm::mat4 cbox_proj = glm::perspective(glm::radians(fov), Wid / Hei, n_v, f_v);
	glm::vec3 cornell_light_pos = cbox_mid + glm::vec3(0.0, 100.0, 0.0);

	cornell_box.mesh = &cbox;
	cornell_box.camera = new Camera(cbox_eyePos);
	cornell_box.model = cbox_model;
	cornell_box.proj = cbox_proj;
	cornell_box.view = cornell_box.camera->GetViewMatrix();
	cornell_box.n_val = n_v;
	cornell_box.f_val = f_v;
	cornell_box.bb_mid = cbox_mid;
	
	Shader cbox_g_buffer          ("shaders/cbox_gbuffer_vert.glsl", "shaders/cbox_gbuffer_frag.glsl");
	Shader cbox_rsm_pass          ("shaders/cbox_rsm_vert.glsl", "shaders/cbox_rsm_frag.glsl");
	Shader cbox_parabolic_rsm_pass("shaders/cbox_parabolic_rsm_vert.glsl", "shaders/cbox_parabolic_rsm_frag.glsl");

	//Resize sponza
	sponza.mesh = new mesh_loader(sponza_path);
	sponza.bb_mid = sponza.mesh->bb_mid * glm::vec3(0.05);	
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
	shadowmap.s_w = 512.f;
	shadowmap.s_h = 512.f;
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
	framebuffer gbuffer(fbo_type::G_BUFFER, w.Wid, w.Hei, 0); //front buffer
	framebuffer gbackbuffer(fbo_type::G_BUFFER, w.Wid, w.Hei, 0); //backbuffer
	Shader geometry_pass ("shaders/deferred_render_vert.glsl", "shaders/deferred_render_frag.glsl");
	
	/* -------------- RSM FRAMEBUFFER AND SHADER----------------*/			
	framebuffer rsm_buffer(fbo_type::RSM, shadowmap.s_w, shadowmap.s_h, 0);
	Shader RSM_pass("shaders/rsm_vert.glsl", "shaders/rsm_frag.glsl");
		
	/*----FILL SSBO WITH LIGHTS ----*/
		
	//unsigned int size = sizeof(point_light) * VPL_MAX;
	unsigned int size = sizeof(point_light) * (VPL_SAMPLES * 2);
	shader_storage_buffer lightSSBO(size);
	
	bool is_filled = false;

	unsigned int num_tiles = ceil(Wid / 16) * ceil(Hei / 16);
	shader_storage_buffer frustum_planes(sizeof(frustum) * num_tiles);
	

	int data = VPL_SAMPLES;
	shader_storage_buffer light_buffer_size(sizeof(GLint));
	light_buffer_size.bind();
	GLCall(glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(GLint), &data));
	light_buffer_size.unbind();
				
	/* ---- COMPUTE SHADERS ---- */
	Shader gen_frustum(nullptr, nullptr, nullptr, "shaders/gen_frustum.glsl" );
	Shader gen_light_buffer(nullptr, nullptr, nullptr, "shaders/gen_light_ssbo.glsl");	
	Shader tiled_shading(nullptr, nullptr, nullptr, "shaders/tiled_shading.glsl");	
	Shader ssvp(nullptr, nullptr, nullptr,  "shaders/compute_SSVP2.glsl");
	Shader backface_cluster(nullptr, nullptr, nullptr, "shaders/cluster_backface.glsl");
	Shader gen_vals(nullptr, nullptr, nullptr, "shaders/generate_vals.glsl");
	Shader clusterize_vals(nullptr, nullptr, nullptr, "shaders/clusterize_vals.glsl");
	Shader update_vals(nullptr, nullptr, nullptr, "shaders/update_vals.glsl");
	Shader generate_final_buffer(nullptr, nullptr, nullptr, "shaders/generate_final_light_buffer.glsl");
	Shader second_bounce_program("shaders/ssbo_debug_draw_vert.glsl", "shaders/count_second_bounce_vpls_frag.glsl");
	Shader recompute_bounce(nullptr, nullptr, nullptr, "shaders/recompute_second_bounce.glsl" );
	Shader parabolic_rsm("shaders/parabolic_rsm_vert.glsl","shaders/parabolic_rsm_frag.glsl" );

	GLuint samplesBuffer;
	GLuint samplesTBO;

	//RSM sampling pattern
	std::vector<glm::vec2> samples = gen_uniform_samples(VPL_SAMPLES, 0.05f, 0.95f);	

	GLCall(glGenBuffers(1, &samplesBuffer));
	GLCall(glBindBuffer(GL_TEXTURE_BUFFER, samplesBuffer));
	GLCall(glBufferData(GL_TEXTURE_BUFFER, sizeof(glm::vec2)*samples.size(), samples.data(), GL_DYNAMIC_READ));

	GLCall(glGenTextures(1, &samplesTBO));
	GLCall(glBindTexture(GL_TEXTURE_BUFFER, samplesTBO));
	GLCall(glTexBuffer(GL_TEXTURE_BUFFER, GL_RG32F, samplesBuffer));

	//Normal sampling pattern
	int num_normal_samples = 64;
	std::vector<glm::vec2> normal_samples(num_normal_samples);
	GLuint normal_samples_buffer;
	GLuint normal_samples_tbo;

	std::normal_distribution<float> randomFloats(0.1, 0.9);
	//std::normal_distribution<float> randomFloats(min, max);
	std::default_random_engine gen;	

	for (int i = 0; i < num_normal_samples; i++) {
		glm::vec2 val{ 0,0 };
		val.x = randomFloats(gen);
		val.y = randomFloats(gen);
		normal_samples[i] = val;
	}


	GLCall(glGenBuffers(1, &normal_samples_buffer));
	GLCall(glBindBuffer(GL_TEXTURE_BUFFER, normal_samples_buffer));
	GLCall(glBufferData(GL_TEXTURE_BUFFER, sizeof(glm::vec2)*normal_samples.size(), normal_samples.data(), GL_DYNAMIC_READ));

	GLCall(glGenTextures(1, &normal_samples_tbo));
	GLCall(glBindTexture(GL_TEXTURE_BUFFER, normal_samples_tbo));
	GLCall(glTexBuffer(GL_TEXTURE_BUFFER, GL_RG32F, normal_samples_buffer));

	//VAL sampling pattern
	GLuint val_sample_buffer;
	GLuint val_sample_tbo;
	int num_val_clusters = 4;
	std::vector<glm::vec2> val_samp(4);
	//tile dim
	float half_xres = shadowmap.s_w / 2.0f;
	float half_yres = shadowmap.s_h / 2.0f;

	int it_size = (num_val_clusters / 2);

	for (int i = 0; i < it_size; i++)
	{
		for (int j = 0; j < it_size; j++)
		{
			glm::vec2 s = glm::vec2(j*half_xres +  0.5 * half_xres,
				i * half_yres + 0.5*half_yres);
			s /= glm::vec2(shadowmap.s_w, shadowmap.s_h);
			
			val_samp[j * it_size + i] = s;

		} 	
	}	

	shader_storage_buffer direct_vals(sizeof(point_light) * num_val_clusters);
	GLCall(glGenBuffers(1, &val_sample_buffer));
	GLCall(glBindBuffer(GL_TEXTURE_BUFFER, val_sample_buffer));
	GLCall(glBufferData(GL_TEXTURE_BUFFER, sizeof(glm::vec2)*val_samp.size(), val_samp.data(), GL_DYNAMIC_READ));

	GLCall(glGenTextures(1, &val_sample_tbo));
	GLCall(glBindTexture(GL_TEXTURE_BUFFER, val_sample_tbo));
	GLCall(glTexBuffer(GL_TEXTURE_BUFFER, GL_RG32F, val_sample_buffer));

	shader_storage_buffer vpls_per_val(sizeof(unsigned int) * (num_val_clusters * VPL_SAMPLES));
	shader_storage_buffer count_vpl_per_val(sizeof(unsigned int)  * num_val_clusters);
	shader_storage_buffer pass_vpl_count(sizeof(unsigned int));

	int num_back_samples = 256;
	shader_storage_buffer backface_vpls(sizeof(point_light)*num_back_samples);
	shader_storage_buffer backface_vpl_count(sizeof(unsigned int));
	unsigned int count = 0;
	backface_vpl_count.bind();
	GLCall(glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(GLuint), &count));
	backface_vpl_count.unbind();

	pass_vpl_count.bind();
	GLCall(glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(GLuint), &count));
	pass_vpl_count.unbind();

	uniform_buffer val_mats(num_val_clusters * sizeof(glm::mat4));
	
	bool first_frame = true;
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
	//Shader skybox("shaders/skybox_vert.glsl", "shaders/skybox_frag.glsl");
		
	//Cube Map data	
	//glm::vec2 cube_res(128.f, 128.f);
	//glm::mat4 cubeModel = sponza.model;
	//glm::mat4 cubeProj = glm::perspective(glm::radians(90.0f), cube_res.x/cube_res.y, 1.f, 400.0f);	
	//glm::mat4 cubeView[6];	
	//
	//Shader cube_rendering("shaders/vertex_passthru_vert.glsl", "shaders/cube_layered_frag.glsl", "shaders/cube_layered_geom.glsl");
	//framebuffer cube(fbo_type::CUBE_MAP, cube_res.x, cube_res.y, 0);
	//
	////glm::vec3 pos = sponza.mesh->bb_mid * 0.05f;
	//glm::vec3 pos = glm::vec3(0.0, 1.0, 0.0);
	//
	//cubeView[0] = glm::lookAt(eyePos, eyePos +  glm::vec3(1.0, 0.0, 0.0), glm::vec3(0.0, -1.0, 0.0));
	//cubeView[1] = glm::lookAt(eyePos, eyePos + glm::vec3(-1.0, 0.0, 0.0), glm::vec3(0.0, -1.0, 0.0));
	//cubeView[2] = glm::lookAt(eyePos, eyePos +  glm::vec3(0.0, 1.0, 0.0), glm::vec3(0.0, 0.0, 1.0));
	//cubeView[3] = glm::lookAt(eyePos, eyePos + glm::vec3(0.0, -1.0, 0.0), glm::vec3(0.0, 0.0, -1.0));
	//cubeView[4] = glm::lookAt(eyePos, eyePos +  glm::vec3(0.0, 0.0, 1.0), glm::vec3(0.0, -1.0, 0.0));
	//cubeView[5] = glm::lookAt(eyePos, eyePos + glm::vec3(0.0, 0.0, -1.0), glm::vec3(0.0, -1.0, 0.0));
	//
	//uniform_buffer cube_buf(6 * sizeof(glm::mat4));
	//cube_buf.upload_data(&cubeView[0]);

	//paste cube map here
	

	/*---------VPL VISIBILITY SAMPLES AND PROGRAM--------------*/
	float ism_w = 512.f;
	float ism_h = 512.f;
	Shader parabolic_map ("shaders/parabolic_map_vert.glsl", "shaders/parabolic_map_frag.glsl");
	
	std::vector<framebuffer> parabolic_fbos(num_val_clusters);
	for (int i = 0; i < parabolic_fbos.size() ; i++)
	{
		//parabolic_fbos[i] = framebuffer(fbo_type::SHADOW_MAP, ism_w, ism_h, 0);
		parabolic_fbos[i] = framebuffer(fbo_type::RSM, ism_w, ism_h, 0);
	}
		
	/*-----------------------------------------------------------------------------------------------------*/


	/*-----uniform sampling inside circular patch oriented by axis-----*/
	GLuint vector_offset_buffer;
	GLuint vector_offset_tbo;
	std::vector<glm::vec2> vector_offset_samples = gen_uniform_samples((VPL_SAMPLES), 0.0f, 1.0f);
	//std::vector<glm::vec2> vector_offset_samples = gen_uniform_samples((MaxVPL - VPL_SAMPLES), 0.0f, 1.0f);
	
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
	GLCall(glBindBuffer(GL_ARRAY_BUFFER, backface_vpls.ssbo));
	GLCall(glEnableVertexAttribArray(0));
	GLCall(glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(point_light), (void*)0));
	GLCall(glEnableVertexAttribArray(1));
	GLCall(glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(point_light), (void*)offsetof(point_light, n)));
	GLCall(glEnableVertexAttribArray(2));
	GLCall(glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(point_light), (void*)offsetof(point_light, c)));
	GLCall(glBindVertexArray(0));
	glEnable(GL_PROGRAM_POINT_SIZE);

	double t0, tf, delta_t;
	t0 = tf = delta_t = 0;
	t0 = glfwGetTime();
	
	//Tile Frustum generation decoupled from tiled frustum culling pass
	generate_tile_frustum(gen_frustum, frustum_planes, Wid, Hei, invProj);		
	
	int num_cluster_pass = 2;

	Shader test(nullptr, nullptr, nullptr, "shaders/test_output.glsl");
	std::vector<glm::mat4> pView(num_val_clusters);
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
		//sponza
		//rsm_pass(RSM_pass, rsm_buffer, shadowmap, sponza);	

		//cornel rsm pass
		cbox_rsm_pass.use();
		cbox_rsm_pass.setMat4("P", shadowmap.proj);
		cbox_rsm_pass.setMat4("V", shadowmap.view);
		cbox_rsm_pass.setMat4("M", cornell_box.model);
		cbox_rsm_pass.setFloat("lightColor", 0.65f);
		cbox_rsm_pass.setVec3("lightDir", shadowmap.lightDir);
		glViewport(0, 0, shadowmap.s_w, shadowmap.s_h);
		rsm_buffer.bind();
		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
		cornell_box.mesh->Draw(cbox_rsm_pass);
		rsm_buffer.unbind();

		/* -------G BUFFER PASS------*/			
		glm::mat4 v_mat = cornell_box.camera->GetViewMatrix();

		//sponza
		//gbuffer_pass(gbuffer, geometry_pass, sponza, v_mat);
		//cornell
		gbuffer_pass(gbuffer, cbox_g_buffer, cornell_box, v_mat);		
		

		/*-----ACTUAL RENDERING-----*/
		if (!debug_view)
		{						
			//FILL LIGHT SSBO 
			if (!is_filled) 
			{  
				fill_lightSSBO(rsm_buffer, cornell_box.camera->GetViewMatrix(), gen_light_buffer, samplesTBO, lightSSBO, VPL_SAMPLES);	
				//sponza
				//fill_lightSSBO(rsm_buffer, sponza.camera->GetViewMatrix(), gen_light_buffer, samplesTBO, lightSSBO, VPL_SAMPLES);				
				is_filled = true;
			}		
				
			// GENERATE VALS
			gen_vals.use();

			GLCall(glActiveTexture(GL_TEXTURE0));
			GLCall(glBindTexture(GL_TEXTURE_2D, rsm_buffer.pos));
			gen_vals.setInt("rsm_position", 0);
			GLCall(glActiveTexture(GL_TEXTURE1));
			GLCall(glBindTexture(GL_TEXTURE_2D, rsm_buffer.normal));
			gen_vals.setInt("rsm_normal", 1);
			GLCall(glActiveTexture(GL_TEXTURE2));
			GLCall(glBindTexture(GL_TEXTURE_2D, rsm_buffer.albedo));
			gen_vals.setInt("rsm_flux", 2);
			GLCall(glBindImageTexture(0, val_sample_tbo, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RG32F));
			direct_vals.bindBase(0);
			GLCall(glDispatchCompute(1, 1, 1));
			GLCall(glMemoryBarrier(GL_ALL_BARRIER_BITS));
			direct_vals.unbind();
						
			/* --- BEGIN CLUSTER PASS ---*/
			
			clusterize_vals.use();

			direct_vals.bindBase(0);
			lightSSBO.bindBase(1);
			vpls_per_val.bindBase(2);
			count_vpl_per_val.bindBase(3);

			clusterize_vals.setInt("num_vals", num_val_clusters);
			clusterize_vals.setInt("vpl_samples", VPL_SAMPLES);

			GLCall(glDispatchCompute(1, 1, 1));
			GLCall(glMemoryBarrier(GL_ALL_BARRIER_BITS));
			direct_vals.unbind();
			lightSSBO.unbind();
			vpls_per_val.unbind();
			count_vpl_per_val.unbind();					

			//update vals
			update_vals.use();
			direct_vals.bindBase(0);
			lightSSBO.bindBase(1);
			vpls_per_val.bindBase(2);
			count_vpl_per_val.bindBase(3);			
			update_vals.setInt("vpl_samples", VPL_SAMPLES);
			GLCall(glDispatchCompute(1, 1, 1));
			GLCall(glMemoryBarrier(GL_ALL_BARRIER_BITS));
			direct_vals.unbind();
			lightSSBO.unbind();
			vpls_per_val.unbind();
			count_vpl_per_val.unbind();
						
			/* --- END CLUSTER PASS ---*/			
			
			//* ---- BEGIN VAL SM PASS ---- */
			direct_vals.bind();
			point_light *first_vals = (point_light* )glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
			glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);		
			direct_vals.unbind();
			glDepthMask(GL_TRUE);
			glEnable(GL_DEPTH_TEST);
			
			
			//render a parabolic map for each val
			for (int i = 0; i < num_val_clusters; i++)
			{
				glm::vec3 valpos(first_vals[i].p);
				glm::vec3 norm(first_vals[i].n);
			
				//ONB construction
				float ks = (norm.z >= 0.0) ? 1.0 : -1.0;
				float ka = 1.0 / (1.0 + abs(norm.z));
				float kb = -ks * norm.x * norm.y * ka;
				glm::vec3 uu = glm::normalize(glm::vec3(1.0 - norm.x * norm.x * ka, ks*kb, -ks * norm.x));
				glm::vec3 vv = glm::normalize(glm::vec3(kb, ks - norm.y * norm.y * ka * ks, -norm.y));
				glm::mat4 val_lookat = glm::lookAt(valpos, valpos + (-norm), uu);
				pView[i] = val_lookat;
				
				glCullFace(GL_FRONT);
				//do_parabolic_map(parabolicModelView, parabolic_fbos[i], parabolic_map, ism_w, ism_h, sponza);
				//sponza
				//do_parabolic_rsm(pView[i], parabolic_fbos[i],parabolic_rsm ,ism_w, ism_h, sponza);
				do_parabolic_rsm(pView[i], parabolic_fbos[i], cbox_parabolic_rsm_pass,ism_w, ism_h, cornell_box);
			}						
			glViewport(0, 0, Wid, Hei);
			glCullFace(GL_BACK);

			/* ------- END VAL SM PASS -------- */


			/* ------- NEW SSVP PROPAGATION ------- */			
		
			if (first_frame)
			{
				//val_mats.upload_data(&pView);
				backface_vpls.bindBase(0);
				backface_vpl_count.bindBase(1);
				//direct_vals.bindBase(2);
				//pass_vpl_count.bindBase(3);
				//val_mats.set_binding_point(0);
				for (int i = 0; i < num_val_clusters; i++)
				{
					compute_vpl_propagation(ssvp, pView, parabolic_fbos[i], parabolic_fbos, samplesTBO, 1.0f, 40.0f, VPL_SAMPLES, num_val_clusters);
				}
				backface_vpls.unbind();
				backface_vpl_count.unbind();
				//direct_vals.unbind();
				//pass_vpl_count.unbind();
				//val_mats.unbind();
				first_frame = false;
			}
									
			
			/*-----------Old SSVP-------------------*/

			/*---- GENERATING FINAL ITERATION BUFFER --- */
			direct_vals.bindBase(0);
			lightSSBO.bindBase(1);
			vpls_per_val.bindBase(2);
			count_vpl_per_val.bindBase(3);			
			generate_final_buffer.use();
			generate_final_buffer.setInt("num_vpls", VPL_SAMPLES);
			GLCall(glDispatchCompute(1, 1, 1));
			GLCall(glMemoryBarrier(GL_ALL_BARRIER_BITS));
			direct_vals.unbind();
			lightSSBO.unbind();
			vpls_per_val.unbind();
			count_vpl_per_val.unbind();
			
			

			/* ---- shading -----*/

			frustum_planes.bindBase(0);
			lightSSBO.bindBase(1);
			direct_vals.bindBase(2);	
			backface_vpls.bindBase(3);
			backface_vpl_count.bindBase(4);

			//sponza
			//do_tiled_shading(tiled_shading, gbuffer, rsm_buffer, draw_tex, invProj, sponza, shadowmap, VPL_SAMPLES, num_val_clusters, lightSSBO, Wid, Hei, 1.0f, 40.f, pView, parabolic_fbos);
			do_tiled_shading(tiled_shading, gbuffer, rsm_buffer, draw_tex, invProj, cornell_box, shadowmap, VPL_SAMPLES, num_val_clusters, lightSSBO, Wid, Hei, 1.0f, 80.f, pView, parabolic_fbos);
			
			frustum_planes.unbind();			
			lightSSBO.unbind();		
			direct_vals.unbind();
			backface_vpl_count.unbind();
			backface_vpls.unbind();
						

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
				glDepthMask(GL_FALSE);
				glDisable(GL_DEPTH_TEST);

				backface_vpl_count.bind();
				unsigned int *a = (unsigned int*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
				glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
				backface_vpl_count.unbind();
				//std::cout << "tamanho do light buffer " << *a << std::endl;
				GLCall(glBindVertexArray(lightvao));
				ssbo_debug.use();
				glm::mat4 MVP = cornell_box.proj * cornell_box.camera->GetViewMatrix();
				//glm::mat4 MVP = sponza.proj * sponza.camera->GetViewMatrix();
				ssbo_debug.setMat4("MVP", MVP);
				ssbo_debug.setFloat("size", 10.0f);
				glDrawArrays(GL_POINTS, 0, num_back_samples);
				GLCall(glBindVertexArray(0));

				glDepthMask(GL_TRUE);
				glEnable(GL_DEPTH_TEST);
			}			

		}
		else		
		{				
			/* ---- DEBUG VIEW ---- */		
			render_debug_view(w, depthView, screen_quad, shadowmap, parabolic_fbos[0]);			
			//debug cube draw
			//draw_skybox(skybox, cube, sponza.camera->GetViewMatrix(), sponza.proj, test_cube);
		}
		
		//delta time and gui frametime renderer;
		{
			tf = glfwGetTime();
			delta_t = (tf - t0) * 1000.0;
			gdelta_t = delta_t;
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
	if (cornell_box.camera)
		delete cornell_box.camera;
			
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
		cornell_box.camera->ProcessKeyboard(FORWARD, 1);
		
		//std::cout << to_string(sponza.camera->Position) << std::endl;
	}

	if (key == GLFW_KEY_A && (action == GLFW_PRESS || action == GLFW_REPEAT))
	{
		sponza.camera->ProcessKeyboard(LEFT, 1);
		cornell_box.camera->ProcessKeyboard(LEFT, 1);
		//std::cout << to_string(sponza.camera->Position) << std::endl;
	}

	if (key == GLFW_KEY_S && (action == GLFW_PRESS || action == GLFW_REPEAT))
	{
		sponza.camera->ProcessKeyboard(BACKWARD, 1);
		cornell_box.camera->ProcessKeyboard(BACKWARD, 1);
		//std::cout << to_string(sponza.camera->Position) << std::endl;
	}

	if (key == GLFW_KEY_D && (action == GLFW_PRESS || action == GLFW_REPEAT))
	{
		sponza.camera->ProcessKeyboard(RIGHT, 1);
		cornell_box.camera->ProcessKeyboard(RIGHT, 1);
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

		cornell_box.camera->ProcessMouseMovement(offsetx, offsety);
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
	GLCall(glBindTexture(GL_TEXTURE_2D, shadow_buffer.albedo));
	//GLCall(glBindTexture(GL_TEXTURE_2D, rsm.albedo));
	debug_quad.renderQuad();
}




