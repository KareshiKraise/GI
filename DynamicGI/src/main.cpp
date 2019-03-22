#include "PrototypeIncludes.h"

//GLOBAL CONSTANTS
const float PI = 3.1415926f;
const float PI_TWO = 6.2831853;
int VPL_SAMPLES = 8 * 8; //number of lights

//scene and model paths
const char* sponza_path = "models/sponza/sponza.obj";
const char* sphere_path = "models/sphere/sphere.obj";
const char* cornell_path = "models/cornell/CornellBox-Original.obj";
std::string path;

scene current_scene;
shadow_data light_data;

quad screen_quad;
Cube test_cube;
scene sphere_scene;

enum view_mode {
	SCENE = 0,
	SHADOW_MAP = 1,
	PARABOLIC = 2,
	NUM_MODES
};

/*---camera controls---*/
double lastX = 0, lastY = 0;
bool active_cam = true;
int current_view = 0;
bool view_vpls = false;
bool debug_view = false;
bool view_parabolic = false;

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
	
	//Shader hash
	std::unordered_map<std::string, Shader> shader_table;

	/*-----parameters----*/
	float Wid = 1280.0f;
	float Hei = 720.0f;
	
	const float fov = 60.0f;
	const float cornel_n_v = 0.1f;
	const float cornel_f_v = 5.f;

	const float sponza_n_v = 1.f;
	const float sponza_f_v = 3000.f;

	const float sponza_spd = 25.f;
	const float cbox_spd = 0.10f;

	const glm::vec3 cbox_pos(0.0150753, 1.01611, 3.31748);
	
	const float rsm_res = 512.f;

	const float ism_w = 512.f;
	const float ism_h = 512.f;

	float ism_near;
	float ism_far;

	const int num_val_clusters = 4;
	float vpl_radius;

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
	
	if (argc > 1) 
	{
		std::string model = argv[1];
		if (model == "sponza") {
			path = sponza_path;
			current_scene.mesh = new mesh_loader(path.c_str());
			current_scene.bb_mid = current_scene.mesh->bb_mid;
			current_scene.n_val = sponza_n_v;
			current_scene.f_val = sponza_f_v;
			current_scene.camera = new Camera(current_scene.bb_mid);
			current_scene.camera->MovementSpeed = sponza_spd;
			current_scene.model = glm::mat4(1.0f);
			current_scene.view = current_scene.camera->GetViewMatrix();
			current_scene.proj = glm::perspective(glm::radians(fov), Wid / Hei, sponza_n_v, sponza_f_v);

			light_data.lightColor = glm::vec3(0.65);
			light_data.lightPos = current_scene.bb_mid + glm::vec3(0, 2000.f, 0);
			light_data.lightDir = glm::normalize(light_data.lightPos - current_scene.bb_mid);
			light_data.s_near = sponza_n_v;
			light_data.s_far = sponza_f_v;
			light_data.s_w = rsm_res;
			light_data.s_h = rsm_res;
			light_data.view = glm::lookAt(light_data.lightPos, current_scene.bb_mid, glm::vec3(0.0, 1.0, 0.2));
			light_data.proj = glm::ortho(-1000.f, 1000.f, -200.f, 200.f, light_data.s_near, light_data.s_far);
			vpl_radius = 700.f;
			ism_near = 1.0;
			ism_far = vpl_radius;
		}
		else if (model == "cbox") {
			path = cornell_path;			
			current_scene.mesh = new mesh_loader(path.c_str(), model_type::NO_TEXTURE);
			current_scene.bb_mid = current_scene.mesh->bb_mid;
			current_scene.n_val = cornel_n_v;
			current_scene.f_val = cornel_f_v;
			current_scene.camera = new Camera(cbox_pos);
			current_scene.camera->MovementSpeed = cbox_spd;
			current_scene.model = glm::mat4(1.0f);
			current_scene.view = current_scene.camera->GetViewMatrix();
			current_scene.proj = glm::perspective(glm::radians(fov), Wid / Hei, current_scene.n_val, current_scene.f_val);

			light_data.lightColor = glm::vec3(0.9);
			light_data.lightPos = glm::vec3(0.0, 1.95f, 0.0f);
			light_data.lightDir = glm::normalize(light_data.lightPos - current_scene.bb_mid);
			light_data.s_near = cornel_n_v;
			light_data.s_far = cornel_f_v;
			light_data.s_w = rsm_res;
			light_data.s_h = rsm_res;
			light_data.view = glm::lookAt(light_data.lightPos, light_data.lightPos + glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0, 0.0, -1.0));
			light_data.proj = glm::ortho(current_scene.mesh->min.x, current_scene.mesh->max.x, current_scene.mesh->min.z, current_scene.mesh->max.z, light_data.s_near, light_data.s_far);
			
			vpl_radius = 1.50f;
			ism_near = 0.1;
			ism_far = vpl_radius;
		}
		else {
			std::cout <<  "unknown argument or command, program will exit." << std::endl;
			return 1;
		}		
	}
	else {
		path = sponza_path;
		current_scene.mesh = new mesh_loader(path.c_str());
		current_scene.bb_mid = current_scene.mesh->bb_mid;
		current_scene.n_val = sponza_n_v;
		current_scene.f_val = sponza_f_v;
		current_scene.camera = new Camera(current_scene.bb_mid);
		current_scene.camera->MovementSpeed = sponza_spd;
		current_scene.model = glm::mat4(1.0f);
		current_scene.view = current_scene.camera->GetViewMatrix();
		current_scene.proj = glm::perspective(glm::radians(fov), Wid / Hei, sponza_n_v, sponza_f_v);

		light_data.lightColor = glm::vec3(0.65);
		light_data.lightPos = current_scene.bb_mid + glm::vec3(0, current_scene.mesh->max.y, 0);
		light_data.lightDir = glm::normalize(light_data.lightPos - current_scene.bb_mid);
		light_data.s_near = sponza_n_v;
		light_data.s_far = sponza_f_v;
		light_data.s_w = rsm_res;
		light_data.s_h = rsm_res;
		light_data.view = glm::lookAt(light_data.lightPos, current_scene.bb_mid, glm::vec3(0.0, 1.0, 0.2));
		light_data.proj = glm::ortho(-1000.f, 1000.f, -200.f, 200.f, light_data.s_near, light_data.s_far);
		
		vpl_radius = 700.f;
		ism_near = 1.0;
		ism_far = vpl_radius;
	}
	
	//All shader declarations

	//Cornell Box specific
	Shader cbox_g_buffer("shaders/cbox_gbuffer_vert.glsl", "shaders/cbox_gbuffer_frag.glsl");
	Shader cbox_rsm_pass("shaders/cbox_rsm_vert.glsl", "shaders/cbox_rsm_frag.glsl");
	Shader cbox_parabolic_rsm_pass("shaders/cbox_parabolic_rsm_vert.glsl", "shaders/cbox_parabolic_rsm_frag.glsl");	

	//Sponza specific
	Shader sponza_g_buffer("shaders/deferred_render_vert.glsl", "shaders/deferred_render_frag.glsl");
	Shader sponza_rsm_pass("shaders/rsm_vert.glsl", "shaders/rsm_frag.glsl");
	Shader sponza_parabolic_rsm_pass("shaders/parabolic_rsm_vert.glsl", "shaders/parabolic_rsm_frag.glsl");
	Shader sponza_shadow_map("shaders/shadow_map_vert.glsl", "shaders/shadow_map_frag.glsl");
	Shader sponza_parabolic_map("shaders/parabolic_map_vert.glsl", "shaders/parabolic_map_frag.glsl");

	/* ---- COMPUTE SHADERS ---- */
	Shader gen_frustum(nullptr, nullptr, nullptr, "shaders/gen_frustum.glsl");
	Shader gen_light_buffer(nullptr, nullptr, nullptr, "shaders/gen_light_ssbo.glsl");
	Shader tiled_shading(nullptr, nullptr, nullptr, "shaders/tiled_shading.glsl");
	Shader ssvp(nullptr, nullptr, nullptr, "shaders/compute_SSVP2.glsl");
	Shader backface_cluster(nullptr, nullptr, nullptr, "shaders/cluster_backface.glsl");
	Shader gen_vals(nullptr, nullptr, nullptr, "shaders/generate_vals.glsl");
	Shader clusterize_vals(nullptr, nullptr, nullptr, "shaders/clusterize_vals.glsl");
	Shader update_vals(nullptr, nullptr, nullptr, "shaders/update_vals.glsl");
	Shader generate_final_buffer(nullptr, nullptr, nullptr, "shaders/generate_final_light_buffer.glsl");
	Shader second_bounce_program("shaders/ssbo_debug_draw_vert.glsl", "shaders/count_second_bounce_vpls_frag.glsl");
	Shader recompute_bounce(nullptr, nullptr, nullptr, "shaders/recompute_second_bounce.glsl");
	
	/*----SHADOW MAP DEBUG VIEW -----*/
	Shader depthView("shaders/screen_quad_vert.glsl", "shaders/depth_view_frag.glsl", nullptr);

	//shader for debug drawing
	Shader ssbo_debug("shaders/ssbo_debug_draw_vert.glsl", "shaders/ssbo_debug_draw_frag.glsl");
	
	//Shader to blit a texture to full screen quad
	Shader blit("shaders/screen_quad_vert.glsl", "shaders/blit_texture_frag.glsl");

	//Shader for debug drawing
	Shader test(nullptr, nullptr, nullptr, "shaders/test_output.glsl");

	shader_table["blit"] = blit;
	shader_table["test output"] = test;
	shader_table["ssbo debug"] = ssbo_debug;
	shader_table["view depth"] = depthView;
	
	if (path == sponza_path)
	{
		shader_table["geometry pass"] = sponza_g_buffer;
		shader_table["rsm pass"] = sponza_rsm_pass;
		shader_table["parabolic rsm pass"] = sponza_parabolic_rsm_pass;
	}
	else if (path == cornell_path)
	{
		shader_table["geometry pass"] = cbox_g_buffer;
		shader_table["rsm pass"] = cbox_rsm_pass;
		shader_table["parabolic rsm pass"] = cbox_parabolic_rsm_pass;
	}

	shader_table["generate frustum"] = gen_frustum;
	shader_table["sample rsm"] = gen_light_buffer;
	shader_table["tiled shading"] = tiled_shading;
	shader_table["ssvp"] = ssvp;
	shader_table["generate vals"] = gen_vals;
	shader_table["cluster vals"] = clusterize_vals;
	shader_table["update vals"] = update_vals;
	shader_table["gen final buffer"] = generate_final_buffer;


	//END OF SHADER DECLARATIONS

	//Framebuffer declarations
	//main shadow_map depth buffer
	framebuffer depthBuffer(fbo_type::SHADOW_MAP, light_data.s_w, light_data.s_h, 0);
	//Main GBUFFER
	framebuffer gbuffer(fbo_type::G_BUFFER, w.Wid, w.Hei, 0); 
	//main RSM buffer
	framebuffer rsm_buffer(fbo_type::RSM, light_data.s_w, light_data.s_h, 0);
	std::vector<framebuffer> parabolic_fbos(num_val_clusters);
	for (int i = 0; i < parabolic_fbos.size(); i++)
	{
		//parabolic_fbos[i] = framebuffer(fbo_type::SHADOW_MAP, ism_w, ism_h, 0);
		parabolic_fbos[i] = framebuffer(fbo_type::RSM, ism_w, ism_h, 0);
	}

	//ALL SSBO DECLARATIONS
	
	/*----FILL SSBO WITH LIGHTS ----*/

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
	
	//clustered vals
	shader_storage_buffer direct_vals(sizeof(point_light) * num_val_clusters);

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

	//IMAGE BUFFERS

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
	
	std::vector<glm::vec2> val_samp(4);
	//tile dim
	float half_xres = light_data.s_w / 2.0f;
	float half_yres = light_data.s_h / 2.0f;

	int it_size = (num_val_clusters / 2);

	for (int i = 0; i < it_size; i++)
	{
		for (int j = 0; j < it_size; j++)
		{
			glm::vec2 s = glm::vec2(j*half_xres +  0.5 * half_xres,
				i * half_yres + 0.5*half_yres);
			s /= glm::vec2(light_data.s_w, light_data.s_h);
			
			val_samp[j * it_size + i] = s;
		} 	
	}	
	
	GLCall(glGenBuffers(1, &val_sample_buffer));
	GLCall(glBindBuffer(GL_TEXTURE_BUFFER, val_sample_buffer));
	GLCall(glBufferData(GL_TEXTURE_BUFFER, sizeof(glm::vec2)*val_samp.size(), val_samp.data(), GL_DYNAMIC_READ));

	GLCall(glGenTextures(1, &val_sample_tbo));
	GLCall(glBindTexture(GL_TEXTURE_BUFFER, val_sample_tbo));
	GLCall(glTexBuffer(GL_TEXTURE_BUFFER, GL_RG32F, val_sample_buffer));

	/*-----uniform sampling inside circular patch oriented by axis-----*/
	GLuint vector_offset_buffer;
	GLuint vector_offset_tbo;
	std::vector<glm::vec2> vector_offset_samples = gen_uniform_samples((VPL_SAMPLES), 0.0f, 1.0f);

	GLCall(glGenBuffers(1, &vector_offset_buffer));
	GLCall(glBindBuffer(GL_TEXTURE_BUFFER, vector_offset_buffer));
	GLCall(glBufferData(GL_TEXTURE_BUFFER, sizeof(glm::vec2)*vector_offset_samples.size(), vector_offset_samples.data(), GL_DYNAMIC_READ));

	GLCall(glGenTextures(1, &vector_offset_tbo));
	GLCall(glBindTexture(GL_TEXTURE_BUFFER, vector_offset_tbo));
	GLCall(glTexBuffer(GL_TEXTURE_BUFFER, GL_RG32F, vector_offset_buffer));

	//Uniform Buffers Declarations

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
	
	//Debug VAO
	glEnable(GL_PROGRAM_POINT_SIZE);
	GLuint lightvao;

	GLCall(glGenVertexArrays(1, &lightvao));
	GLCall(glBindVertexArray(lightvao));
	GLCall(glBindBuffer(GL_ARRAY_BUFFER, lightSSBO.ssbo));
	GLCall(glEnableVertexAttribArray(0));
	GLCall(glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(point_light), (void*)0));
	GLCall(glEnableVertexAttribArray(1));
	GLCall(glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(point_light), (void*)offsetof(point_light, n)));
	GLCall(glEnableVertexAttribArray(2));
	GLCall(glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(point_light), (void*)offsetof(point_light, c)));
	GLCall(glBindVertexArray(0));
	

	//see clusters
	GLuint cluster_vao;

	GLCall(glGenVertexArrays(1, &cluster_vao));
	GLCall(glBindVertexArray(cluster_vao));
	GLCall(glBindBuffer(GL_ARRAY_BUFFER, direct_vals.ssbo));
	GLCall(glEnableVertexAttribArray(0));
	GLCall(glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(point_light), (void*)0));
	GLCall(glEnableVertexAttribArray(1));
	GLCall(glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(point_light), (void*)offsetof(point_light, n)));
	GLCall(glEnableVertexAttribArray(2));
	GLCall(glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(point_light), (void*)offsetof(point_light, c)));
	GLCall(glBindVertexArray(0));

	//view second bounces
	GLuint second_bounce_vao;

	GLCall(glGenVertexArrays(1, &second_bounce_vao));
	GLCall(glBindVertexArray(second_bounce_vao));
	GLCall(glBindBuffer(GL_ARRAY_BUFFER, backface_vpls.ssbo));
	GLCall(glEnableVertexAttribArray(0));
	GLCall(glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(point_light), (void*)0));
	GLCall(glEnableVertexAttribArray(1));
	GLCall(glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(point_light), (void*)offsetof(point_light, n)));
	GLCall(glEnableVertexAttribArray(2));
	GLCall(glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(point_light), (void*)offsetof(point_light, c)));
	GLCall(glBindVertexArray(0));


	double t0, tf, delta_t;
	t0 = tf = delta_t = 0;
	t0 = glfwGetTime();
	
	glm::mat4 invProj = glm::inverse(current_scene.proj);

	//Tile Frustum generation decoupled from tiled frustum culling pass
	generate_tile_frustum(gen_frustum, frustum_planes, Wid, Hei, invProj);		
	
	int num_cluster_pass = 2;
	
	std::vector<glm::mat4> pView(num_val_clusters);
	//main loop
	while (!w.should_close()) {			
								
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		glDepthMask(GL_TRUE);
		glEnable(GL_DEPTH_TEST);		
				
		current_scene.view = current_scene.camera->GetViewMatrix();
		/*---- SHADOW MAP AND RSM PASS ----*/
		
		/*------- LIGHT SPACE MATRIX -------*/
		light_data.light_space_mat = light_data.proj * light_data.view;
		
		/*---------RSM_PASS--------*/
		
		rsm_pass(shader_table["rsm pass"], rsm_buffer, light_data, current_scene);	

		/* -------G BUFFER PASS------*/			
		
		glm::mat4 v_mat = current_scene.camera->GetViewMatrix();
				
		gbuffer_pass(gbuffer, shader_table["geometry pass"], current_scene, v_mat);
			

		/*-----ACTUAL RENDERING-----*/
		if (current_view == SCENE)
		{						
			//FILL LIGHT SSBO 
			if (!is_filled) 
			{  				
				fill_lightSSBO(rsm_buffer, current_scene.camera->GetViewMatrix(), shader_table["sample rsm"], samplesTBO, lightSSBO, VPL_SAMPLES, vpl_radius);
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
				do_parabolic_rsm(pView[i], parabolic_fbos[i], shader_table["parabolic rsm pass"],ism_w, ism_h, current_scene, ism_near, ism_far);
				
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
					compute_vpl_propagation(shader_table["ssvp"], pView, parabolic_fbos[i], parabolic_fbos, samplesTBO, 1.0f, 40.0f, VPL_SAMPLES, num_val_clusters, vpl_radius);
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
			
			//lightSSBO.bind();
			//point_light *a = (point_light*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
			//glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
			//lightSSBO.unbind();
			//
			//for (int i = 0; i < VPL_SAMPLES; i++)
			//{
			//	std::cout << a[i].n.w << std::endl;
			//}
			

			/* ---- shading -----*/

			frustum_planes.bindBase(0);
			lightSSBO.bindBase(1);
			direct_vals.bindBase(2);	
			backface_vpls.bindBase(3);
			backface_vpl_count.bindBase(4);			
			
			do_tiled_shading(shader_table["tiled shading"], gbuffer, rsm_buffer, draw_tex, invProj, current_scene, light_data, VPL_SAMPLES, num_val_clusters, lightSSBO, Wid, Hei, ism_near, ism_far, pView, parabolic_fbos);
			
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
			blit.setFloat("near", current_scene.n_val);
			blit.setFloat("far", current_scene.f_val);
			screen_quad.renderQuad();	
		
			if (view_vpls)
			{

				glEnable(GL_DEPTH_TEST);
												
				GLCall(glBindVertexArray(lightvao));
				ssbo_debug.use();
				glm::mat4 MVP = current_scene.proj * current_scene.camera->GetViewMatrix();
				ssbo_debug.setMat4("MVP", MVP);
				ssbo_debug.setFloat("size", 10.0f);
				ssbo_debug.setVec4("vpl_color", glm::vec4(1.0, 0.0, 0.0, 1.0));
				glDrawArrays(GL_POINTS, 0, VPL_SAMPLES);
				GLCall(glBindVertexArray(0));						

				GLCall(glBindVertexArray(cluster_vao));
				ssbo_debug.use();
				MVP = current_scene.proj * current_scene.camera->GetViewMatrix();
				ssbo_debug.setMat4("MVP", MVP);
				ssbo_debug.setFloat("size", 10.0f);
				ssbo_debug.setVec4("vpl_color", glm::vec4(0.0, 1.0, 0.0, 1.0));
				glDrawArrays(GL_POINTS, 0, num_val_clusters);
				GLCall(glBindVertexArray(0));								

				backface_vpl_count.bind();
				unsigned int *a = (unsigned int*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
				glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
				backface_vpl_count.unbind();
				//std::cout << "tamanho do light buffer " << *a << std::endl;
				GLCall(glBindVertexArray(second_bounce_vao));
				ssbo_debug.use();
				MVP = current_scene.proj * current_scene.camera->GetViewMatrix();
				ssbo_debug.setMat4("MVP", MVP);
				ssbo_debug.setFloat("size", 10.0f);
				ssbo_debug.setVec4("vpl_color", glm::vec4(0.0, 0.0, 1.0, 1.0));
				glDrawArrays(GL_POINTS, 0, *a);
				GLCall(glBindVertexArray(0));
			}

		}
		else if(current_view == SHADOW_MAP)
		{				
			/* ---- DEBUG VIEW ---- */		
			render_debug_view(w, shader_table["view depth"], screen_quad, light_data, rsm_buffer);			
			
		}
		else if(current_view == PARABOLIC) {
			render_debug_view(w, shader_table["view depth"], screen_quad, light_data, parabolic_fbos[0]);
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
	if (current_scene.mesh)
		delete current_scene.mesh;
	if (current_scene.camera)
		delete current_scene.camera;
	
			
	return 0;
}

void kbfunc(GLFWwindow* window, int key, int scan, int action, int mods) {
	if (key == GLFW_KEY_ESCAPE && (action == GLFW_PRESS || action == GLFW_REPEAT))
	{
		glfwSetWindowShouldClose(window, GLFW_TRUE);
	}

	if (key == GLFW_KEY_W && (action == GLFW_PRESS || action == GLFW_REPEAT))
	{		
		current_scene.camera->ProcessKeyboard(FORWARD, 1);		
		//std::cout << to_string(current_scene.camera->Position) << std::endl;
	}

	if (key == GLFW_KEY_A && (action == GLFW_PRESS || action == GLFW_REPEAT))
	{
		current_scene.camera->ProcessKeyboard(LEFT, 1);		
		//std::cout << to_string(current_scene.camera->Position) << std::endl;
	}

	if (key == GLFW_KEY_S && (action == GLFW_PRESS || action == GLFW_REPEAT))
	{
		current_scene.camera->ProcessKeyboard(BACKWARD, 1);		
		//std::cout << to_string(current_scene.camera->Position) << std::endl;
	}

	if (key == GLFW_KEY_D && (action == GLFW_PRESS || action == GLFW_REPEAT))
	{
		current_scene.camera->ProcessKeyboard(RIGHT, 1);		
		//std::cout << to_string(current_scene.camera->Position) << std::endl;
	}


	if (key == GLFW_KEY_B && (action == GLFW_PRESS)) {
		current_scene.camera->print_camera_coords();
		//On pressing B, light will be updated to the current position, direction  and up of the camera
		light_data.lightDir = -current_scene.camera->Front;
		ref_pos = current_scene.camera->Position;
		ref_up = current_scene.camera->Up;
		ref_front = current_scene.camera->Front;
		light_data.view = glm::lookAt(ref_pos, ref_front, ref_up);
		
	}

	if (key == GLFW_KEY_P && (action == GLFW_PRESS)) {
		view_vpls = !view_vpls;			
	}

	if (key == GLFW_KEY_O && (action == GLFW_PRESS)) {
		debug_view = !debug_view;
		current_view++;
		current_view = current_view % NUM_MODES;		
	}

	if (key == GLFW_KEY_U && (action == GLFW_PRESS)) {
		view_parabolic = !view_parabolic;
		std::cout << "view parabolic depth map" << std::endl;
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

		current_scene.camera->ProcessMouseMovement(offsetx, offsety);
		
		lastX = xpos;
		lastY = ypos;
		//std::cout << "camera yaw"   << current_scene.camera->Yaw  << std::endl;
		//std::cout << "camera pitch" << current_scene.camera->Pitch << std::endl;
	
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




