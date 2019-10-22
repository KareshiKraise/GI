#include "PrototypeIncludes.h"

//GLOBAL CONSTANTS
const float PI = 3.1415926f;
const float PI_TWO = 6.2831853;
int VPL_SAMPLES = 8 * 8; //number of lights
const int NUM_2ND_BOUNCE = 64;
int num_val_clusters = 4;
int viewSamples = 256;
int vpl_budget = 256;

//scene and model paths
const char* sponza_path = "models/sponza/sponza.obj";
const char* sphere_path = "models/sphere/sphere.obj";
//const char* cornell_path = "models/cornell/CornellBox-Original.obj";
const char* cornell_path = "models/cornell/dense.obj";

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

enum move {
	F,
	B,
	L,
	R
};

bool forward = false, back = false, left = false, right = false;
bool light_rotate_left = false, light_rotate_right = false;

/*---camera controls---*/
double lastX = 0, lastY = 0;
bool active_cam = true;
int current_view = 0;
int current_layer = 0;
bool view_vpls = false;
bool debug_view = false;
bool view_parabolic = false;
bool see_bounce = false;

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

#define FRAME_AVG 128
struct frametime_data
{
	int frame_count;	
	double data[FRAME_AVG];
};

double avg_fps(frametime_data &dat)
{
	double sum = 0;
	for (int i = 0; i < FRAME_AVG; i++)
	{
		sum += dat.data[i];
	}
	sum /= FRAME_AVG;
	sum = 1000.0 / sum;
	return sum;
}

double avg_ms(frametime_data &dat)
{
	double sum = 0;
	for (int i = 0; i < FRAME_AVG; i++)
	{
		sum += dat.data[i];
	}
	sum /= FRAME_AVG;	
	return sum;
}

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
void render_gui(double delta, double fps) {
	// Start the Dear ImGui frame
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	//IMGUI scope
	{
		ImGui::Begin("CPU Timer");
		ImGui::Text("%f miliseconds per frame", delta);	
		ImGui::Text("%f frames per second", fps);
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

	//frametime data 
	frametime_data time_dat;
	frametime_data ms_dat;
	ms_dat.frame_count = 0;
	time_dat.frame_count = 0;

	//Shader hash
	std::unordered_map<std::string, Shader> shader_table;

	/*-----parameters----*/
	//float Wid = 3840.0f;
	//float Hei = 2160.0f;
	
	float Wid = 1280.0f;
	float Hei = 720.0f;

	//float Wid = 1920.0f;
	//float Hei = 1080.0f;

	const float fov = 65.0f;
	const float cornel_n_v = 0.1f;
	const float cornel_f_v = 10.f;

	const float sponza_n_v = 1.f;
	const float sponza_f_v = 5000.f;

	const float sponza_spd = 25.f;
	const float cbox_spd = 0.10f;

	const glm::vec3 cbox_pos(0.0150753, 1.01611, 3.31748);

	//Change values for res
	const float rsm_res = 256.f;

	const float ism_w = 512.f;
	const float ism_h = 512.f;

	float ism_near;
	float ism_far;
	
	float vpl_radius;

	int num_cluster_pass = 3;
	int num_blur_pass = 2;

	int num_rows = 2;
	int num_cols = 2;

	//cornell box lighting
	spot_light spotlight;
	float cutoff_angle = 15.0f; //in degrees
	float cutoff = cos(glm::radians(cutoff_angle));
	spotlight.c = glm::vec4(1.0, 1.0, 1.0, glm::radians(cutoff_angle));
    spotlight.d = glm::vec4(0.536056, 0.428935, -0.727089, 1.0);	
	//spotlight.d = glm::vec4(0.336056, 0.628935, -0.727089, 1.0);
	//spotlight.d = glm::vec4(0.136056, 0.828935, -0.727089, 1.0);
      spotlight.d = glm::vec4(0.356056, 0.4228935, -0.727089, 1.0);
	//spotlight.d = glm::vec4(0.0, 1.0, -0.5, 1.0);
	spotlight.p = glm::vec4(0.17507, 1.27212, 1.0336, 1.0);

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

	//glClearColor(1.0, 1.0, 1.0, 1.0);
	
	if (argc > 1)
	{
		std::string model = argv[1];
		if (model == "sponza") {
			glClearColor(1.0, 1.0, 1.0, 1.0);
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
			//light_data.lightPos = glm::vec3(100, 2000, 100);
			light_data.lightDir = glm::normalize(light_data.lightPos - current_scene.bb_mid);
			light_data.s_near = sponza_n_v;
			light_data.s_far = sponza_f_v;
			light_data.s_w = rsm_res;
			light_data.s_h = rsm_res;
			light_data.view = glm::lookAt(light_data.lightPos, current_scene.bb_mid, glm::vec3(0.0, 1.0, 0.2));
			light_data.proj = glm::ortho(-1000.f, 1000.f, -200.f, 200.f, light_data.s_near, light_data.s_far);
			//light_data.proj = glm::ortho(-900.f, 900.f, -500.f, 500.f, light_data.s_near, light_data.s_far);
			vpl_radius = 2000.f;
			ism_near = 100.f;
			ism_far = vpl_radius-500.f;
			//num_val_clusters = 4;
		}
		else if (model == "cbox") {
			glClearColor(0.0, 0.0, 0.0, 1.0);
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

			light_data.lightColor = glm::vec3(1.0);
			//light_data.lightPos = glm::vec3(0.0, 1.95f, 0.0f);
			//light_data.lightDir = glm::normalize(light_data.lightPos - current_scene.bb_mid);
			light_data.lightPos = glm::vec3(spotlight.p);
			light_data.lightDir = -glm::normalize(glm::vec3(spotlight.d));
			light_data.s_near = cornel_n_v;
			light_data.s_far = cornel_f_v;
			light_data.s_w = rsm_res;
			light_data.s_h = rsm_res;
			light_data.view = glm::lookAt(glm::vec3(spotlight.p), glm::vec3(spotlight.p)+glm::vec3(spotlight.d), glm::vec3(-0.204316, 0.93606, 0.28644));
			light_data.proj = glm::perspective(glm::radians(cutoff_angle), 1.0f, light_data.s_near, light_data.s_far);
			//light_data.view = glm::lookAt(light_data.lightPos, light_data.lightPos + glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0, 0.0, -1.0));
			//light_data.proj = glm::ortho(current_scene.mesh->min.x, current_scene.mesh->max.x, current_scene.mesh->min.z, current_scene.mesh->max.z, light_data.s_near, light_data.s_far);

			vpl_radius = 5.0f;
			ism_near = 0.1;
			ism_far = vpl_radius;
			//num_val_clusters = 4;
		}
		else {
			std::cout << "unknown argument or command, program will exit." << std::endl;
			return 1;
		}
	}
	else {
		glClearColor(1.0, 1.0, 1.0, 1.0);
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

		vpl_radius = 2000.f;
		ism_near = 1.f;
		ism_far = vpl_radius-500.f;
		//num_val_clusters = 4;
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

	//generic 
	Shader view_layered("shaders/screen_quad_vert.glsl", "shaders/view_layered_frag.glsl");
	Shader layered_val_shadowmap("shaders/layered_parabolic_view_vert.glsl", "shaders/layered_parabolic_view_frag.glsl", "shaders/layered_parabolic_view_geom.glsl");
	Shader layered_cbox_val_shadowmap("shaders/layered_cbox_parabolic_view_vert.glsl", "shaders/layered_cbox_parabolic_view_frag.glsl", "shaders/layered_cbox_parabolic_view_geom.glsl");


	//COMPUTE SHADERS
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
	Shader split_buff(nullptr, nullptr, nullptr, "shaders/split_gbuffer.glsl" );
	Shader interleaved_shade(nullptr, nullptr, nullptr, "shaders/interleaved_shade.glsl");
	Shader interleaved_shade2(nullptr, nullptr, nullptr, "shaders/interleaved_shade2.glsl");
	Shader cbox_interleaved_shade(nullptr, nullptr, nullptr, "shaders/cbox_interleaved_shade.glsl");
	Shader compute_BRSM(nullptr, nullptr, nullptr, "shaders/compute_brsm.glsl");
	Shader sample_VPLs(nullptr, nullptr, nullptr, "shaders/VPLS_From_CDF.glsl");
	Shader build_cdf_col(nullptr,nullptr,nullptr, "shaders/build_cdf_cols.glsl");
	Shader build_cdf_row(nullptr,nullptr,nullptr, "shaders/build_cdf_row.glsl");

	Shader join_gbuffer(nullptr, nullptr, nullptr, "shaders/join_gbuffer.glsl");
	Shader edge_program(nullptr, nullptr, nullptr, "shaders/edge_detection.glsl");
	Shader xblur(nullptr, nullptr, nullptr, "shaders/gaussian_blur_x.glsl");
	Shader yblur(nullptr, nullptr, nullptr, "shaders/gaussian_blur_y.glsl");

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
		shader_table["parabolic rsm pass"] = layered_val_shadowmap;
		shader_table["interleaved shading"] = interleaved_shade2;
	}
	else if (path == cornell_path)
	{
		shader_table["geometry pass"] = cbox_g_buffer;
		shader_table["rsm pass"] = cbox_rsm_pass;
		shader_table["parabolic rsm pass"] = layered_cbox_val_shadowmap;
		shader_table["interleaved shading"] = cbox_interleaved_shade;
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
	//Interleaved Gbuffer
	framebuffer interleaved_buffer(fbo_type::G_BUFFER, w.Wid, w.Hei, 0);
	//main RSM buffer
	framebuffer rsm_buffer(fbo_type::RSM, light_data.s_w, light_data.s_h, 0);

	std::vector<framebuffer> parabolic_fbos(num_val_clusters);

	for (int i = 0; i < parabolic_fbos.size(); i++)
	{
		//parabolic_fbos[i] = framebuffer(fbo_type::SHADOW_MAP, ism_w, ism_h, 0);
		parabolic_fbos[i] = framebuffer(fbo_type::RSM, ism_w, ism_h, 0);
	}

	//val visibility fbo and matrices
	framebuffer val_array_fbo(fbo_type::DEEP_G_BUFFER, ism_w, ism_h, num_val_clusters);
	std::vector<glm::mat4> pView(num_val_clusters);

	//ALL SSBO DECLARATIONS

	/*----FILL SSBO WITH LIGHTS ----*/

	//unsigned int size = sizeof(point_light) * (VPL_SAMPLES * 2);
	unsigned int size = sizeof(point_light) * vpl_budget;
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

	shader_storage_buffer vpls_per_val(sizeof(unsigned int) * (num_val_clusters * vpl_budget));
	shader_storage_buffer count_vpl_per_val(sizeof(unsigned int)  * num_val_clusters);
	shader_storage_buffer pass_vpl_count(sizeof(unsigned int));

	int num_back_samples = NUM_2ND_BOUNCE * num_val_clusters;
	shader_storage_buffer backface_vpls(sizeof(point_light)*num_back_samples);
	shader_storage_buffer backface_vpl_count(sizeof(unsigned int));
	unsigned int count = 0;
	backface_vpl_count.bind();
	GLCall(glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(GLuint), &count));
	backface_vpl_count.unbind();

	pass_vpl_count.bind();
	GLCall(glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(GLuint), &count));
	pass_vpl_count.unbind();

	//BRSM stores average influence of each pvpl over selected view samples	
	GLuint BRSM_Buf;
	GLuint BRSM_TBO;
	shader_storage_buffer BRSM_ssbo(sizeof(GLfloat)*(rsm_res*rsm_res));
	//GLCall(glGenBuffers(1, &BRSM_Buf));
	//GLCall(glBindBuffer(GL_TEXTURE_BUFFER, BRSM_Buf));
	//GLCall(glBufferData(GL_TEXTURE_BUFFER, sizeof(GLfloat)*rsm_res*rsm_res, NULL, GL_STATIC_READ));
	//GLCall(glGenTextures(1, &BRSM_TBO));
	//GLCall(glBindTexture(GL_TEXTURE_BUFFER, BRSM_TBO));
	//GLCall(glTexBuffer(GL_TEXTURE_BUFFER, GL_R32F, BRSM_Buf));

	GLCall(glGenTextures(1, &BRSM_TBO));
	GLCall(glBindTexture(GL_TEXTURE_2D, BRSM_TBO));
	GLCall(glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, rsm_res, rsm_res, 0, GL_RED, GL_FLOAT, NULL));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
	

	//BRSM_CDF stores the 2D CDF OF A BRSM
	GLuint BRSM_CDF_Buf;
	//COLS CDF
	GLuint BRSM_CDF_TBO; 
	//ROW CDF
	GLuint BRSM_CDF_ROW_TBO;
	shader_storage_buffer BRSM_cdf_ssbo(sizeof(GLfloat)*rsm_res*rsm_res);
	shader_storage_buffer BRSM_cdf_row_ssbo(sizeof(GLfloat)*rsm_res);

	//GLCall(glGenBuffers(1, &BRSM_CDF_Buf));
	//GLCall(glBindBuffer(GL_TEXTURE_BUFFER, BRSM_CDF_Buf));
	//GLCall(glBufferData(GL_TEXTURE_BUFFER, sizeof(GLfloat)*rsm_res*rsm_res, NULL, GL_STATIC_READ));
	//GLCall(glGenTextures(1, &BRSM_CDF_TBO));
	//GLCall(glBindTexture(GL_TEXTURE_BUFFER, BRSM_CDF_TBO));
	//GLCall(glTexBuffer(GL_TEXTURE_BUFFER, GL_R32F, BRSM_CDF_Buf));

	GLCall(glGenTextures(1, &BRSM_CDF_TBO));
	GLCall(glBindTexture(GL_TEXTURE_2D, BRSM_CDF_TBO));
	GLCall(glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, rsm_res, rsm_res, 0, GL_RED, GL_FLOAT, NULL));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));

	GLCall(glGenTextures(1, &BRSM_CDF_ROW_TBO));
	GLCall(glBindTexture(GL_TEXTURE_1D, BRSM_CDF_ROW_TBO));
	GLCall(glTexImage1D(GL_TEXTURE_1D, 0, GL_R32F, rsm_res, 0, GL_RED, GL_FLOAT, NULL));
	GLCall(glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
	GLCall(glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));

	//CDF SAMPLING VALUES
	GLuint cdf_r_buff;
	GLuint cdf_r_tbo;
	std::vector<glm::vec2> cdf_r_vec = gen_uniform_samples(vpl_budget, 0.0f, 1.0f);
	
	GLCall(glGenBuffers(1, &cdf_r_buff));
	GLCall(glBindBuffer(GL_TEXTURE_BUFFER, cdf_r_buff));
	GLCall(glBufferData(GL_TEXTURE_BUFFER, sizeof(glm::vec2)*cdf_r_vec.size(), cdf_r_vec.data(), GL_DYNAMIC_READ));
	GLCall(glGenTextures(1, &cdf_r_tbo));
	GLCall(glBindTexture(GL_TEXTURE_BUFFER, cdf_r_tbo));
	GLCall(glTexBuffer(GL_TEXTURE_BUFFER, GL_RG32F, cdf_r_buff));

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

	//viewSamples for bidirectional rsm
	//std::vector<glm::vec2> view_samples = gen_uniform_samples(viewSamples, 0.05f, 0.95f);	
	std::vector<glm::vec2> view_samples(viewSamples); 
	for (int i =0; i < viewSamples; i++)
	{
		view_samples[i] = hammersley2d(i, viewSamples);
	}

	//for (auto a : view_samples)
	//	std::cout << glm::to_string(a) << std::endl;

	shader_storage_buffer viewportSamples(viewSamples * sizeof(GLfloat) * 2, view_samples.data());

	//unit circle Sampling Pattern	
	std::vector<glm::vec2> normal_samples = gen_uniform_samples(NUM_2ND_BOUNCE, 0.05f, 0.95f);
	
	GLuint normal_samples_buffer;
	GLuint normal_samples_tbo;
	
	for (int i = 0; i < NUM_2ND_BOUNCE; i++) {
		
		glm::vec2 theta = PI_TWO * normal_samples[i];
		glm::vec2 r = sqrt(normal_samples[i]);
		float x = r.y * cos(theta.x);
		float y = r.y * sin(theta.x);
		normal_samples[i] = (glm::vec2(x,y)*0.5f) + glm::vec2(0.5f,0.5f);
		//normal_samples[i] = glm::vec2(x, y);
	}	
		
	GLCall(glGenBuffers(1, &normal_samples_buffer));
	GLCall(glBindBuffer(GL_TEXTURE_BUFFER, normal_samples_buffer));
	GLCall(glBufferData(GL_TEXTURE_BUFFER, sizeof(glm::vec2)*normal_samples.size(), normal_samples.data(), GL_DYNAMIC_READ));

	GLCall(glGenTextures(1, &normal_samples_tbo));
	GLCall(glBindTexture(GL_TEXTURE_BUFFER, normal_samples_tbo));
	GLCall(glTexBuffer(GL_TEXTURE_BUFFER, GL_RG32F, normal_samples_buffer));		
	
	//paper illustrative
	GLuint paper_vao;
	GLuint buff;
    std::vector<glm::vec2> nsamples = gen_uniform_samples(120, 0.05, 0.95f);
	//for (int i = 0; i < 120; i++) {
	//
	//	glm::vec2 theta = PI_TWO * nsamples[i];
	//	glm::vec2 r = sqrt(nsamples[i]);
	//	float x = r.y * cos(theta.x);
	//	float y = r.y * sin(theta.x);
	//	nsamples[i] = (glm::vec2(x, y)*0.5f) + glm::vec2(0.5f, 0.5f);
	//	//normal_samples[i] = glm::vec2(x, y);
	//}
	glGenBuffers(1, &buff);
	glBindBuffer(GL_ARRAY_BUFFER, buff);
	glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec2)*nsamples.size(), nsamples.data(), GL_DYNAMIC_READ);
	Shader paper_prog("shaders/paper_vertex.glsl","shaders/paper_frag.glsl");
	////GLuint paper_vbo;
	glGenVertexArrays(1, &paper_vao);
	glBindVertexArray(paper_vao);
	////glGenBuffers(1, &paper_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, buff);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
	glBindVertexArray(0);

	//VAL sampling pattern TODO: Delete
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
	val_mats.set_binding_point(0);
	
	bool first_frame = true;
	/*------------------------------------------------------*/
			
	//drwa texture , image unit to be shown
	GLuint draw_tex;
	GLCall(glGenTextures(1, &draw_tex));
	GLCall(glBindTexture(GL_TEXTURE_2D, draw_tex));
	GLCall(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, Wid, Hei, 0, GL_RGBA, GL_FLOAT, NULL));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER));	
	GLCall(glBindTexture(GL_TEXTURE_2D, 0));

	GLuint final_tex;
	GLCall(glGenTextures(1, &final_tex));
	GLCall(glBindTexture(GL_TEXTURE_2D, final_tex));
	GLCall(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, Wid, Hei, 0, GL_RGBA, GL_FLOAT, NULL));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER ));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER ));
	//float border_col[] = { 0.0f, 0.0f, 0.0f, 0.0f };
	//GLCall(glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border_col));
	GLCall(glBindTexture(GL_TEXTURE_2D, 0));

	GLuint out_tex;
	GLCall(glGenTextures(1, &out_tex));
	GLCall(glBindTexture(GL_TEXTURE_2D, out_tex));
	GLCall(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, Wid, Hei, 0, GL_RGBA, GL_FLOAT, NULL));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER ));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER ));	
	//GLCall(glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border_col));
	GLCall(glBindTexture(GL_TEXTURE_2D, 0));

	GLuint blur_tex;
	GLCall(glGenTextures(1, &blur_tex));
	GLCall(glBindTexture(GL_TEXTURE_2D, blur_tex));
	GLCall(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, Wid, Hei, 0, GL_RGBA, GL_FLOAT, NULL));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER ));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER ));	
	//GLCall(glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border_col));
	GLCall(glBindTexture(GL_TEXTURE_2D, 0));
	
	GLuint edge_tex;
	GLCall(glGenTextures(1, &edge_tex));
	GLCall(glBindTexture(GL_TEXTURE_2D, edge_tex));
	GLCall(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, Wid, Hei, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER ));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER ));
	//GLCall(glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border_col));
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
	//generate_tile_frustum(gen_frustum, frustum_planes, Wid, Hei, invProj);		
	
	double ct0, ctf, cdelta_t;

	//cdf vector
	std::vector<float> cdf(rsm_res*rsm_res);

	bool dr = true;
	
	bool first_clusterization = true;
	//main loop
	while (!w.should_close()) {

		//Input checking
		if (forward)
		{
			current_scene.camera->ProcessKeyboard(FORWARD, 1);
			//std::cout << to_string(current_scene.camera->Position) << std::endl;
		}
		if (left) 
		{
			current_scene.camera->ProcessKeyboard(LEFT, 1);
			//std::cout << to_string(current_scene.camera->Position) << std::endl;
		}
		if (back) 
		{
			current_scene.camera->ProcessKeyboard(BACKWARD, 1);
			//std::cout << to_string(current_scene.camera->Position) << std::endl;
		}
		if (right)
		{
			current_scene.camera->ProcessKeyboard(RIGHT, 1);
			//std::cout << to_string(current_scene.camera->Position) << std::endl;
		}
				
		if (light_rotate_left)
		{
			light_data.lightPos = glm::rotate(light_data.lightPos, glm::radians(0.05f), glm::vec3(1.0, 0.0, 0.0));					
			light_data.lightDir = glm::normalize(light_data.lightPos - current_scene.bb_mid);
			light_data.view = glm::lookAt(light_data.lightPos, current_scene.bb_mid, glm::vec3(0.0, 1.0, 0.2));
			//std::cout << "rotating light to left" << std::endl;
		}
		if (light_rotate_right)
		{
			light_data.lightPos = glm::rotate(light_data.lightPos, glm::radians(-0.05f), glm::vec3(1.0, 0.0, 0.0));
			light_data.lightDir = glm::normalize(light_data.lightPos - current_scene.bb_mid);
			light_data.view = glm::lookAt(light_data.lightPos, current_scene.bb_mid, glm::vec3(0.0, 1.0, 0.2));
			//std::cout << "rotating light to right" << std::endl;
		}
		
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
			//Compute BRSM
			compute_BRSM.use();
			GLCall(glActiveTexture(GL_TEXTURE0));
			GLCall(glBindTexture(GL_TEXTURE_2D, gbuffer.pos));
			compute_BRSM.setInt("gbuffer_pos", 0);
			GLCall(glActiveTexture(GL_TEXTURE1));
			GLCall(glBindTexture(GL_TEXTURE_2D, gbuffer.normal));
			compute_BRSM.setInt("gbuffer_normal", 0);
			GLCall(glActiveTexture(GL_TEXTURE2));
			GLCall(glBindTexture(GL_TEXTURE_2D, gbuffer.albedo));
			compute_BRSM.setInt("gbuffer_albedo", 2);
			GLCall(glActiveTexture(GL_TEXTURE3));
			GLCall(glBindTexture(GL_TEXTURE_2D, rsm_buffer.pos));
			compute_BRSM.setInt("rsm_pos", 3);
			GLCall(glActiveTexture(GL_TEXTURE4));
			GLCall(glBindTexture(GL_TEXTURE_2D, rsm_buffer.normal));
			compute_BRSM.setInt("rsm_normal", 4);
			GLCall(glActiveTexture(GL_TEXTURE5));
			GLCall(glBindTexture(GL_TEXTURE_2D, rsm_buffer.albedo));
			compute_BRSM.setInt("rsm_flux", 5);
			compute_BRSM.setInt("num_samples", viewSamples);
			compute_BRSM.setInt("dim", rsm_res);
			//GLCall(glBindImageTexture(0, BRSM_TBO, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R32F));			
			viewportSamples.bindBase(0);
			BRSM_ssbo.bindBase(1);
			GLCall(glDispatchCompute(rsm_res/16, rsm_res / 16, 1));
			GLCall(glMemoryBarrier(GL_ALL_BARRIER_BITS));			
			BRSM_ssbo.unbind();
			viewportSamples.unbind();						
			//------------------------------------------------------//
			//compute CDF			
			build_cdf_col.use();
			//GLCall(glActiveTexture(GL_TEXTURE0));
			//GLCall(glBindTexture(GL_TEXTURE_2D, BRSM_TBO));
			//build_cdf_col.setInt("BRSM", 0);	
			build_cdf_col.setInt("size", int(rsm_res));
			BRSM_ssbo.bindBase(0);
			BRSM_cdf_ssbo.bindBase(1);
			//GLCall(glBindImageTexture(0, BRSM_CDF_TBO, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32F));
			//TODO change workgroups
			GLCall(glDispatchCompute((rsm_res / 16), 1, 1));
			GLCall(glMemoryBarrier(GL_ALL_BARRIER_BITS));
			BRSM_ssbo.unbind();
			BRSM_cdf_ssbo.unbind();					
			//compute row cdf
			build_cdf_row.use();			
			build_cdf_row.setInt("size", int(rsm_res));
			//GLCall(glBindImageTexture(0, BRSM_CDF_TBO, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32F));
			//GLCall(glBindImageTexture(1, BRSM_CDF_ROW_TBO, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32F));
			BRSM_cdf_ssbo.bindBase(0);
			BRSM_cdf_row_ssbo.bindBase(1);
			//TODO change workgroups
			GLCall(glDispatchCompute(1, 1, 1));
			GLCall(glMemoryBarrier(GL_ALL_BARRIER_BITS));
			BRSM_cdf_ssbo.unbind();
			BRSM_cdf_row_ssbo.unbind();						
			//sample vpls
			lightSSBO.bindBase(0);
			BRSM_cdf_ssbo.bindBase(1);
			BRSM_cdf_row_ssbo.bindBase(2);
			sample_VPLs.use();
			GLCall(glActiveTexture(GL_TEXTURE0));
			GLCall(glBindTexture(GL_TEXTURE_2D, rsm_buffer.pos));
			sample_VPLs.setInt("rsm_pos", 0);
			GLCall(glActiveTexture(GL_TEXTURE1));
			GLCall(glBindTexture(GL_TEXTURE_2D, rsm_buffer.normal));
			sample_VPLs.setInt("rsm_normal", 1);
			GLCall(glActiveTexture(GL_TEXTURE2));
			GLCall(glBindTexture(GL_TEXTURE_2D, rsm_buffer.albedo));
			sample_VPLs.setInt("rsm_flux", 2);
			sample_VPLs.setInt("num_vpls", vpl_budget);
			sample_VPLs.setFloat("vpl_radius", vpl_radius );
			sample_VPLs.setInt("dim", int(rsm_res));
			//GLCall(glBindImageTexture(0, BRSM_CDF_TBO, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R32F));
			//GLCall(glBindImageTexture(1, BRSM_CDF_ROW_TBO, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R32F));
			GLCall(glBindImageTexture(2, cdf_r_tbo, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RG32F));
			GLCall(glDispatchCompute((vpl_budget / 16), 1, 1));
			GLCall(glMemoryBarrier(GL_ALL_BARRIER_BITS));
			lightSSBO.unbind();
			BRSM_cdf_ssbo.unbind();
			BRSM_cdf_row_ssbo.unbind();

			//debug print
			//if (dr)
			//{
			//	lightSSBO.bind();
			//	point_light *vpl = (point_light*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
			//	glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);				
			//	lightSSBO.unbind();
			//	for (int i = 0; i < vpl_budget; i++)
			//	{
			//		std::cout << "vpl no: " << i << " position: " <<glm::to_string(vpl[i].p) << std::endl;					
			//	}
			//	dr = false;
			//}			
			
			//------------------------------------------------------//
						
			//FILL LIGHT SSBO -- OLD ROUTINE PRE-BRSM
			//fill_lightSSBO(rsm_buffer, current_scene.camera->GetViewMatrix(), shader_table["sample rsm"], samplesTBO, lightSSBO, VPL_SAMPLES, vpl_radius);
						
			// GENERATE VALS
			direct_vals.bindBase(0);
			lightSSBO.bindBase(1);
			backface_vpl_count.bindBase(2);
			if (first_clusterization)
			{
				generate_vals(gen_vals, rsm_buffer, val_sample_tbo, vpl_budget, num_val_clusters);
				first_clusterization = false;
			}
			direct_vals.unbind();
			lightSSBO.unbind();	
			backface_vpl_count.unbind();

			/* --- BEGIN CLUSTER PASS ---*/			
			lightSSBO.bindBase(1);
			direct_vals.bindBase(0);
			//mapping of vpls to a large list
			vpls_per_val.bindBase(2);			
			//how many vpls per val
			count_vpl_per_val.bindBase(3);
			

			bool first_cluster_pass = true;
			for (int i = 0; i < num_cluster_pass; i++)
			{
				//distance to vals
				calc_distance_to_val(clusterize_vals, num_val_clusters, vpl_budget, first_cluster_pass);
				//update vals
				update_cluster_centers(update_vals, vpl_budget);
				first_cluster_pass = false;
			}
			
			direct_vals.unbind();
			lightSSBO.unbind();
			vpls_per_val.unbind();
			count_vpl_per_val.unbind();
			

			//////debug print			
			//direct_vals.bind();
			//point_light *vpl = (point_light*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
			//for (int i = 0; i < num_val_clusters; i++)
			//{
			//	std::cout << "val no: " << i << " position: " << glm::to_string(vpl[i].p) << std::endl;
			//	std::cout << "val no: " << i << " normal: " << glm::to_string(vpl[i].n) << std::endl;
			//	
			//}
			//glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
			//direct_vals.unbind();			

			//debug print
			//count_vpl_per_val.bind();
			//unsigned int *test = (unsigned int*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
			//for(int i =0; i < num_val_clusters; i++)
			//	std::cout << "num vpls in " << i << " cluster: " << test[i] << std::endl;					
			//glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
			//count_vpl_per_val.unbind();

			/* --- END CLUSTER PASS ---*/	
			
			//* ---- BEGIN VAL SM PASS ---- */
			direct_vals.bind();
			point_light *first_vals = (point_light* )glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
			//if (!first_vals)
			//{
			//	std::cout << "failed to map vals buffer" << std::endl;
			//}
			//for(int i=0; i < num_val_clusters; i++)
			//{ 
			//	//first_vals[i].p = first_vals[i].p + first_vals[i].n * 10.0f;
			//	std::cout << glm::to_string(first_vals[i].p) << std::endl;
			//}

			glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
			direct_vals.unbind();
			glDepthMask(GL_TRUE);
			glEnable(GL_DEPTH_TEST);					
			GLCall(glMemoryBarrier(GL_ALL_BARRIER_BITS));

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
				
				//dont uncoment
				//glCullFace(GL_FRONT);			
				//do_parabolic_rsm(pView[i], parabolic_fbos[i], shader_table["parabolic rsm pass"], ism_w, ism_h, current_scene, ism_near, ism_far);				
			}		
			
			val_mats.upload_data(pView.data());							
			//render clusters dynamically	
			glViewport(0, 0, ism_w, ism_h);			
			glCullFace(GL_FRONT);
			render_cluster_shadow_map(shader_table["parabolic rsm pass"], val_array_fbo, ism_near, ism_far, current_scene, num_val_clusters);				
			glCullFace(GL_BACK);
			glViewport(0, 0, Wid, Hei);
			/* ------- END VAL SM PASS -------- */
			
			/* ------- NEW SSVP PROPAGATION ------- */	
			//TODO - FIX PROPAGATION WITH TEXTURE ARRAYS CHANGE HERE TO FIX SECOND BOUNCES NOT UPDATING
			//uncoment
			backface_vpls.bindBase(0);
			backface_vpl_count.bindBase(1);
			//uncoment
			compute_vpl_propagation(shader_table["ssvp"], pView, val_array_fbo, parabolic_fbos, normal_samples_tbo, ism_near, ism_far, vpl_budget, num_val_clusters, vpl_radius);				
			backface_vpl_count.unbind();			
			backface_vpls.unbind();
						
			//debug print
			//backface_vpl_count.bind();
			//unsigned int *a = (unsigned int*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
			//glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
			//backface_vpl_count.unbind();
			//if (a == nullptr)
			//{
			//	std::cout << "cant read" << std::endl;
			//}
			//else {
			//	std::cout << *a << std::endl;
			//}			
			//backface_vpls.bind();
			//point_light *b = (point_light*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
			//glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
			//backface_vpls.unbind();
			//for (int i =0; i < *a; i++)
			//{
			//	std::cout << glm::to_string(b[i].p) << std::endl;
			//}



			/*---- GENERATING FINAL ITERATION BUFFER --- */
			direct_vals.bindBase(0);
			lightSSBO.bindBase(1);
			vpls_per_val.bindBase(2);
			count_vpl_per_val.bindBase(3);		

			generate_final_buffer.use();
			generate_final_buffer.setInt("num_vpls", vpl_budget);
			GLCall(glDispatchCompute(1, 1, 1));
			GLCall(glMemoryBarrier(GL_ALL_BARRIER_BITS));

			direct_vals.unbind();
			lightSSBO.unbind();
			vpls_per_val.unbind();
			count_vpl_per_val.unbind();	
			
			//debug print
			//backface_vpls.bind();
			//point_light *a = (point_light*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
			//for (int i = 0; i < 256; i++)
			//{
			//	std::cout << glm::to_string(a[52].p) << std::endl;
			//}	
			//glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
			//backface_vpls.unbind();					

			/* ---- shading -----*/
					
			//frustum_planes.bindBase(0);
			
			lightSSBO.bindBase(1);
			direct_vals.bindBase(2);	
			backface_vpls.bindBase(3);
			backface_vpl_count.bindBase(4);	
			
			//dont uncoment
			//do_tiled_shading(shader_table["tiled shading"], gbuffer, rsm_buffer, draw_tex, invProj, current_scene, light_data, VPL_SAMPLES, num_val_clusters, lightSSBO, Wid, Hei, ism_near, ism_far, pView, val_array_fbo, see_bounce);
			//frustum_planes.unbind();			
			//lightSSBO.unbind();		
			//direct_vals.unbind();
			//backface_vpl_count.unbind();
			//backface_vpls.unbind();	

			split_gbuffer(split_buff, gbuffer, interleaved_buffer, num_rows, num_cols,  Wid, Hei);
						
			interleaved_shading(current_scene, draw_tex, shader_table["interleaved shading"], interleaved_buffer, rsm_buffer, val_array_fbo,
				                light_data, vpl_budget, (num_val_clusters * NUM_2ND_BOUNCE) , 
								num_val_clusters, ism_near, ism_far, see_bounce, Wid, Hei, num_rows,  num_cols);
						
			//join/gather split buffer	
			join_buffers(join_gbuffer, draw_tex, final_tex, Wid, Hei, num_rows, num_cols);			

			//discontinuity buffer 
			edge_detection(edge_program, gbuffer, edge_tex, Wid, Hei);
			
			//gaussian blur			
			bilateral_blur(xblur, yblur, final_tex, blur_tex, out_tex, edge_tex, (float)Wid, (float)Hei);			
			
			lightSSBO.unbind();		
			direct_vals.unbind();
			backface_vpl_count.unbind();
			backface_vpls.unbind();	
									
			/*----------------------------blit and tone mapping------------------------------------------------*/

			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);			
			blit.use();
			GLCall(glActiveTexture(GL_TEXTURE0));	
			glBindTexture(GL_TEXTURE_2D, final_tex);
			//GLCall(glBindTexture(GL_TEXTURE_2D, final_tex));
			blit.setInt("texImage1", 0);

			GLCall(glActiveTexture(GL_TEXTURE1));
			GLCall(glBindTexture(GL_TEXTURE_2D, gbuffer.albedo));
			blit.setInt("texAlbedo", 1);

		    GLCall(glActiveTexture(GL_TEXTURE2));
			GLCall(glBindTexture(GL_TEXTURE_2D, gbuffer.depth_map));
			blit.setInt("depthImage", 2);

			blit.setInt("cfactor", (num_rows*num_cols));
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
				ssbo_debug.setFloat("size", 15.0f);
				ssbo_debug.setVec4("vpl_color", glm::vec4(1.0, 0.0, 0.0, 1.0));
				glDrawArrays(GL_POINTS, 0, vpl_budget);
				GLCall(glBindVertexArray(0));
				GLCall(glBindVertexArray(cluster_vao));
				ssbo_debug.use();
				MVP = current_scene.proj * current_scene.camera->GetViewMatrix();
				ssbo_debug.setMat4("MVP", MVP);
				ssbo_debug.setFloat("size", 25.0f);
				ssbo_debug.setVec4("vpl_color", glm::vec4(0.0, 1.0, 0.0, 1.0));
				glDrawArrays(GL_POINTS, 0, num_val_clusters);
				GLCall(glBindVertexArray(0));	
				backface_vpl_count.bind();
				unsigned int *a = (unsigned int*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
				glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
				backface_vpl_count.unbind();
				////std::cout << "tamanho do light buffer " << *a << std::endl;
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

			paper_prog.use();
			glBindVertexArray(paper_vao);
			glDrawArrays(GL_POINTS, 0, 120);
			glBindVertexArray(0);
			
		}
		else if(current_view == PARABOLIC) {
			
			glViewport(0, 0, w.Wid, w.Hei);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			view_layered.use();
			view_layered.setFloat("near", ism_near);
			view_layered.setFloat("far", ism_far);
			view_layered.setInt("layer", current_layer);
			
			GLCall(glActiveTexture(GL_TEXTURE0));
			view_layered.setInt("val_pos", 0);
			GLCall(glBindTexture(GL_TEXTURE_2D_ARRAY, val_array_fbo.pos));
			
			GLCall(glActiveTexture(GL_TEXTURE1));
			view_layered.setInt("val_normal", 1);
			GLCall(glBindTexture(GL_TEXTURE_2D_ARRAY, val_array_fbo.normal));
			
			GLCall(glActiveTexture(GL_TEXTURE2));
			view_layered.setInt("val_color", 2);
			GLCall(glBindTexture(GL_TEXTURE_2D_ARRAY, val_array_fbo.albedo));
			
			GLCall(glActiveTexture(GL_TEXTURE3));
			view_layered.setInt("val_depth", 3);
			GLCall(glBindTexture(GL_TEXTURE_2D_ARRAY, val_array_fbo.depth_map));
			
			screen_quad.renderQuad();

			//paper_prog.use();
			//glBindVertexArray(paper_vao);
			//glDrawArrays(GL_POINTS, 0, 120);
			//glBindVertexArray(0);

		}
		
		//delta time and gui frametime renderer;
		{
			tf = glfwGetTime();
			delta_t = (tf - t0) * 1000.0;
			gdelta_t = delta_t;
		
			if (time_dat.frame_count < FRAME_AVG)			
				time_dat.data[time_dat.frame_count++] = delta_t;
			else
				time_dat.frame_count = 0;
		
			if (ms_dat.frame_count < FRAME_AVG)
				ms_dat.data[ms_dat.frame_count++] = delta_t;
			else
				ms_dat.frame_count = 0;
			
			double fps = avg_fps(time_dat);
			double ms = avg_ms(ms_dat);
			t0 = tf;
			render_gui(ms, fps);
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
		forward = true;
		
		//current_scene.camera->ProcessKeyboard(FORWARD, 1);		
		//std::cout << to_string(current_scene.camera->Position) << std::endl;
	}
	else if (key == GLFW_KEY_W && action == GLFW_RELEASE)
	{
		forward = false;
	}

	if (key == GLFW_KEY_A && (action == GLFW_PRESS || action == GLFW_REPEAT))
	{
		left = true;
		//current_scene.camera->ProcessKeyboard(LEFT, 1);		
		//std::cout << to_string(current_scene.camera->Position) << std::endl;
	}
	else if (key == GLFW_KEY_A && action == GLFW_RELEASE)
	{
		left = false;
	}

	if (key == GLFW_KEY_S && (action == GLFW_PRESS || action == GLFW_REPEAT))
	{
		back = true;
		//current_scene.camera->ProcessKeyboard(BACKWARD, 1);		
		//std::cout << to_string(current_scene.camera->Position) << std::endl;
	}
	else if (key == GLFW_KEY_S && action == GLFW_RELEASE)
	{
		back = false;
	}

	if (key == GLFW_KEY_D && (action == GLFW_PRESS || action == GLFW_REPEAT))
	{
		right = true;
		//current_scene.camera->ProcessKeyboard(RIGHT, 1);		
		//std::cout << to_string(current_scene.camera->Position) << std::endl;
	}
	else if (key == GLFW_KEY_D && action == GLFW_RELEASE)
	{
		right = false;
	}
	
	if (key == GLFW_KEY_T && (action == GLFW_PRESS || action == GLFW_REPEAT))
	{
		light_rotate_left = true;		
	}
	else if (key == GLFW_KEY_T && action == GLFW_RELEASE)
	{
		light_rotate_left = false;
	}

	if (key == GLFW_KEY_Y && (action == GLFW_PRESS || action == GLFW_REPEAT))
	{
		light_rotate_right = true;
	}
	else if (key == GLFW_KEY_Y && action == GLFW_RELEASE)
	{
		light_rotate_right = false;
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
		see_bounce = !see_bounce;
	}

	if (key == GLFW_KEY_L && (action == GLFW_PRESS)) {
		current_layer++;
		current_layer = current_layer % num_val_clusters;
		std::cout << "current_layer " << current_layer << std::endl;
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

	GLCall(glActiveTexture(GL_TEXTURE1));
	debug_program.setInt("rsm_albedo", 1);
	GLCall(glBindTexture(GL_TEXTURE_2D, shadow_buffer.albedo));

	GLCall(glActiveTexture(GL_TEXTURE2));
	debug_program.setInt("rsm_normal", 2);
	GLCall(glBindTexture(GL_TEXTURE_2D, shadow_buffer.normal));

	GLCall(glActiveTexture(GL_TEXTURE3));
	debug_program.setInt("rsm_pos", 3);
	GLCall(glBindTexture(GL_TEXTURE_2D, shadow_buffer.pos));
	
	debug_quad.renderQuad();
}




