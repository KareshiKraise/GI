#include <iostream>
#include "window.h"
#include <GLFW/glfw3.h>
#include "mesh_loader.h"
#include "shader_table.h"
#include "framebuffer.h"
#include "camera.h"
#include "primitives.h"
#include "gputimer.h"
#include "render_passes.h"
#include "SSBO.h"
#include "sample_generator.h"
#include "texture.h"
#include "UBO.h"
#include <glm/gtx/rotate_vector.hpp>

#include <glm/gtx/string_cast.hpp >

#include <algorithm>

std::fstream timer_file;
std::string perf_journal = "";

#define STATS_SIZE 120
struct stats{	
	std::vector<double> gbuffer    ;
	std::vector<double> misc       ;
	std::vector<double> paraboloid ;
	std::vector<double> shade      ;
	std::vector<double> denoise    ;
} stats_track;

int stats_iterator = 0;

double average(std::vector<double>& vec) {
	double cnt = 0;
	for (auto a : vec) 
	{
		cnt += a;
	}
	return cnt / (double)STATS_SIZE;
}

extern window w;
struct mpos {
	float x;
	float y;
};

enum view_mode {
	SCENE = 0,
	//SHADOW_MAP,
	PARABOLIC,
	NUM_MODES
};
int current_view = 0;

//global constants
//number of view samples for importance map computation
const unsigned int num_view_samples = 256;
const unsigned int num_vpls_first_bounce = 256;
const unsigned int num_clusters = 4;
const unsigned int num_vpls_second_bounce = 64; //#change ssvp.cshader if you change here
const unsigned int num_total_2nd = num_clusters * num_vpls_second_bounce;
const float wid = 512.f;
const float hei = 512.f;
//const float wid = 1920.f;
//const float hei = 1200.f;
//const float wid = 3840.f;
//const float hei = 2160.f;
bool first_cluster = true;
const int num_cluster_pass = 5;
int cur_Layer = 0;
int interleave_rows = 2;
int interleave_cols = 2;
bool see_bounce = true; //see 2nd boucne
bool see_vpls = false;
int num_blur_pass = 3;
bool light_move_left = false;
bool light_move_right = false;
bool is_spotlight = false;
bool direct_only = false;
int perf_counter = 0;

bool first = false;
bool cluster = false;
bool second = false;

glm::mat4 process_camera(camera& c, float delta, mpos& lpos, bool& cstate)
{
	static bool last[KEYS] = { 0 };
	//kb
	if (key_state[GLFW_KEY_W])
	{		
		c.ProcessKeyboard(Camera_Movement::FORWARD, delta);
	}
	if (key_state[GLFW_KEY_A])
	{		
		c.ProcessKeyboard(Camera_Movement::LEFT, delta);
	}
	if (key_state[GLFW_KEY_S])
	{
		c.ProcessKeyboard(Camera_Movement::BACKWARD, delta);
	}
	if (key_state[GLFW_KEY_D])
	{
		c.ProcessKeyboard(Camera_Movement::RIGHT, delta);
	}

	//layer
	if (key_state[GLFW_KEY_L])
	{
		last[GLFW_KEY_L] = true;
	}
	if (!key_state[GLFW_KEY_L]) {
		if (last[GLFW_KEY_L] == true) {
			cur_Layer++;
			if (cur_Layer >= num_clusters) {
				cur_Layer = 0;
			}			
		}
		last[GLFW_KEY_L] = false;
	}

	//see second bounce
	if (key_state[GLFW_KEY_O])
	{
		last[GLFW_KEY_O] = true;
	}
	if (!key_state[GLFW_KEY_O]) {
		if (last[GLFW_KEY_O] == true)
			see_bounce = !see_bounce;		
		last[GLFW_KEY_O] = false;
	}

	//see vpl positions
	if (key_state[GLFW_KEY_V])
	{
		last[GLFW_KEY_V] = true;
		
	}
	if (!key_state[GLFW_KEY_V]) {
		if (last[GLFW_KEY_V] == true)
		{
			see_vpls = !see_vpls;
		}
		last[GLFW_KEY_V] = false;
	}

	/*--------------------------------------*/
	if (key_state[GLFW_KEY_G])
	{		
		last[GLFW_KEY_G] = true;				
	}
	if (!key_state[GLFW_KEY_G]) 
	{
		if (last[GLFW_KEY_G] == true)
		{
			direct_only = !direct_only;
		}
		last[GLFW_KEY_G] = false;		
	}
	/*--------------------------------------*/

	   	 	
	//mouse
	if (mouse_state[GLFW_MOUSE_BUTTON_LEFT]){
		if (cstate) {
			lpos.x = mouse_pos[0];
			lpos.y = mouse_pos[1];
			cstate = false;
		}

		double offsetx = mouse_pos[0] - lpos.x;
		double offsety = lpos.y - mouse_pos[1];		
		c.ProcessMouseMovement(offsetx, offsety);
		lpos.x = mouse_pos[0];
		lpos.y = mouse_pos[1];

	}
	else {
		cstate = true;
	}

	if (key_state[GLFW_KEY_T])
	{
		light_move_left = true;
	}
	else if (!key_state[GLFW_KEY_T]) {
		light_move_left = false;
	}

	if (key_state[GLFW_KEY_Y])
	{
		light_move_right = true;
	}
	else if (!key_state[GLFW_KEY_Y]) {
		light_move_right = false;
	}

	if (key_state[GLFW_KEY_K])
	{
		last[GLFW_KEY_K] = true;
	}
	if (!key_state[GLFW_KEY_K]) {
		if (last[GLFW_KEY_K] == true)
		{
			current_view++;
			current_view = current_view % NUM_MODES;
		}
		last[GLFW_KEY_K] = false;
	}

	if (key_state[GLFW_KEY_P]) {
		last[GLFW_KEY_P] = true;
	}	
	if (!key_state[GLFW_KEY_P])
	{
		if (last[GLFW_KEY_P] == true)
		{
			std::cout << "camera pos: " << glm::to_string(c.Position) << std::endl;
			std::cout << "camera front: " << glm::to_string(c.Front) << std::endl;
			std::cout << "camera up: " << glm::to_string(c.Up) << std::endl;
			std::cout << "camera right: " << glm::to_string(c.Right) << std::endl;
			std::cout << "yaw: " << c.Yaw << std::endl;
			std::cout << "pitch: " << c.Pitch << std::endl;
		}
		last[GLFW_KEY_P] = false;
	}

	if (key_state[GLFW_KEY_Q]) {
		last[GLFW_KEY_Q] = true;
	}
	if (!key_state[GLFW_KEY_Q])
	{
		if (last[GLFW_KEY_Q] == true)
		{
			second = !second;
			//std::cout << std::to_string(second) << std::endl;
		}
		last[GLFW_KEY_Q] = false;
	}


	return c.GetViewMatrix();
}

int main(int argc, char** argv){   		

	if (w.create_window(wid, hei, 0) != 0)
		return -1;
	glfwSetInputMode(w.wnd, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
	init_gui(w.wnd);
	glClearColor(0.0, 0.0, 0.0, 1.0);

	frametime_data time_dat;
	frametime_data ms_dat;
	time_dat.frame_count = 0;
	ms_dat.frame_count = 0;	
		
	std::string path;
	std::string scene;

	float sponza_n_v = 0.1f;
	float sponza_f_v = 1000.f;	
	float fov = 65.0f;

	float sponza_spd = 0.0;

	//light properties	
	light_props ldata;
	float paraboloid_near = 50.f;
	float paraboloid_far = 2500.f;
	mesh_loader model;

	if (argc > 1) {
		 scene = argv[1];
	}
	else {
		scene = "sponza";
	}

	if (scene == "sponza") {		
		glClearColor(213.0, 415.0, 811.0, 0.0);

		path = "models/sponza.obj";
		sponza_n_v = 1.0f;
		sponza_f_v = 5000.f;
		model.lazy_init(path.c_str());
		sponza_spd = glm::length(model.bb_size) * 0.00005;		
		ldata.light_pos = model.bb_mid + glm::vec3(0, 1000.f, 0);
		ldata.light_col = glm::vec3(1.0f);
		ldata.light_dir = glm::normalize(ldata.light_pos - model.bb_mid);

		paraboloid_near = 10.f;
		paraboloid_far = 1500.f;
	}
	else if (scene == "cbox") {
		path = "models/cornell_closed/cornell_closed.obj";
		sponza_n_v = 0.1f;
		sponza_f_v = 100.f;
		model.lazy_init(path.c_str());

		sponza_spd = glm::length(model.bb_size) * 0.001;
		
		ldata.light_pos = glm::vec3(0.17507, 1.27212, 1.0336);
		ldata.light_col = glm::vec3(1.0);
		//ldata.light_dir = glm::vec3(0.536056, 0.428935, -0.727089);
		ldata.light_dir = glm::vec3(0.536056, 0.428935, -0.927089);

		paraboloid_near = 0.1f;
		paraboloid_far = 10.f;
		is_spotlight = true;
	}
	else if (scene == "vokselia") {

		glClearColor(0.52, 0.807, 0.921, 1.0);

		path = "models/vokselia/vokselia_spawn.obj";
		sponza_n_v = 0.01f;
		sponza_f_v = 5000.f;
		model.lazy_init(path.c_str());

		sponza_spd = glm::length(model.bb_size) * 0.0001;

		ldata.light_pos = model.bb_mid + glm::vec3(0, 10.f, 0);
		ldata.light_col = glm::vec3(1.0f);
		ldata.light_dir = glm::normalize(ldata.light_pos - model.bb_mid);
	}
	else if (scene == "sibenik") 
	{
		glClearColor(0.6, 0.6, 0.6, 1.0);
		path = "models/SibenikAtlas/sibenik.obj";
		sponza_n_v = 0.1f;
		sponza_f_v = 100.f;
		model.lazy_init(path.c_str());
		sponza_spd = glm::length(model.bb_size) * 0.0001;
		//ldata.light_pos = model.bb_mid + glm::vec3(0, 0.f, 0);
		//ldata.light_col = glm::vec3(1.0f);
		//ldata.light_dir = glm::normalize(ldata.light_pos - model.bb_mid);
		ldata.light_pos = model.bb_mid;//glm::vec3(3.9809, 0.0039617, 9.5979);
		ldata.light_col = glm::vec3(1.0);
		ldata.light_dir = glm::vec3(1.0, 0.0, 0.0);
		paraboloid_near = 1.0f;
		paraboloid_far = 100.0f;	
		is_spotlight = true;
	}
	else if (scene == "conf") 
	{
		glClearColor(1.0, 1.0, 1.0, 1.0);
		path = "models/conference/textured_conference.obj";
		sponza_n_v = 0.1f;
		sponza_f_v = 1000.f;
		model.lazy_init(path.c_str());
		sponza_spd = glm::length(model.bb_size) * 0.0001;
		//TODO: CHANGE		
		ldata.light_pos = model.bb_mid;
		ldata.light_col = glm::vec3(1.0f);
		ldata.light_dir = glm::vec3(0.0, 1.0, 0.0);//glm::normalize(ldata.light_pos - model.bb_mid);
		paraboloid_near = 1.0f;
		paraboloid_far = 380.0f;
		is_spotlight = true;	
	}
			

	stats_track.gbuffer.resize(STATS_SIZE);
	stats_track.misc.resize(STATS_SIZE);
	stats_track.denoise.resize(STATS_SIZE);
	stats_track.shade.resize(STATS_SIZE);
	stats_track.paraboloid.resize(STATS_SIZE);

	std::fill(stats_track.gbuffer.begin(), stats_track.gbuffer.end(), 0.0);
	std::fill(stats_track.misc.begin(), stats_track.misc.end(), 0.0);
	std::fill(stats_track.denoise.begin(), stats_track.denoise.end(), 0.0);
	std::fill(stats_track.shade.begin(), stats_track.shade.end(), 0.0);
	std::fill(stats_track.paraboloid.begin(), stats_track.paraboloid.end(), 0.0);		

	//camera properties
	camera cam(model.bb_mid);
	//camera cam(glm::vec3(604.035767, 188.382080, -516.054749), glm::vec3(-0.176975, 0.983255, 0.043469), glm::vec3(-0.954873, -0.182236, 0.234538), -193.8, -10.5);	
	cam.MovementSpeed = sponza_spd;

	//camera press state (for mouse movement)
	bool cstate = true;
	mpos lpos;
		
	//ldata.light_pos = model.bb_mid + glm::vec3(0, 1000.f, 0);
	//ldata.light_col = glm::vec3(0.65);
	//ldata.light_dir = glm::normalize(ldata.light_pos - model.bb_mid);	

	const unsigned int rsm_w = 2048;
	const unsigned int rsm_h = 2048;
	const unsigned int sm_w = 2048;
	const unsigned int sm_h = 2048;
	const unsigned int ism_w = 1024;
	const unsigned int ism_h = 1024;	

	//gbuffer transformation properties	
	Transforms scene_transforms;
	scene_transforms["M"] = glm::mat4(1.0f);
	scene_transforms["V"] = cam.GetViewMatrix();
	scene_transforms["P"] = glm::perspective(glm::radians(fov), wid / hei, sponza_n_v, sponza_f_v);

	//rsm transformation properties
	Transforms light_transforms;
	float cutoff_angle = 0.0f; //in degrees

	if (scene == "sponza") {
		light_transforms["M"] = glm::mat4(1.0f);
		light_transforms["V_shadowmap"] = glm::lookAt(ldata.light_pos, model.bb_mid, glm::vec3(0.0, 1.0, 0.2));
		auto rsm_pos = model.bb_mid + glm::vec3(0, 100.f, 0);
		light_transforms["V_rsm"] = glm::lookAt(rsm_pos, model.bb_mid, glm::vec3(0.0, 1.0, 0.2));
		light_transforms["P"] = glm::ortho(-900.f, 900.f, -190.f, 190.f, sponza_n_v, sponza_f_v);
		
		//camera pos: vec3(1048.577393, 267.680908, -25.795895)
		//camera front : vec3(-0.992874, -0.116671, 0.024268)
		//camera up : vec3(-0.116636, 0.993171, 0.002851)
		//camera right : vec3(-0.024435, 0.000000, -0.999701)
		//yaw : -181.4
	    //pitch : -6.70001

		cam.Position = glm::vec3(1048.577393, 267.680908, -25.795895);
		cam.Front =    glm::vec3(-0.992874, -0.116671, 0.024268);
		cam.Up =       glm::vec3(-0.116636, 0.993171, 0.002851);
		cam.Right =    glm::vec3(-0.024435, 0.000000, -0.999701);
		cam.Yaw = -181.4;
		cam.Pitch = -6.70001;
		cam.updateCameraVectors();
	}
	else if (scene == "cbox"){
		cutoff_angle = 20.f;
		light_transforms["M"] = glm::mat4(1.0f);
		light_transforms["P"] = glm::perspective(glm::radians(40.f), 1.0f, sponza_n_v, sponza_f_v);
		glm::vec3 target = glm::vec3(ldata.light_pos) + glm::vec3(ldata.light_dir);
		glm::vec3 norm_dir = glm::normalize(target);
		glm::vec3 npos = ldata.light_pos;// - norm_dir*0.1f;
		light_transforms["V_rsm"] = glm::lookAt(npos, target, glm::vec3(-0.204316, 0.93606, 0.28644));
		//light_transforms["V_rsm"] = glm::lookAt(glm::vec3(ldata.light_pos), glm::vec3(ldata.light_pos) + glm::vec3(ldata.light_dir), glm::vec3(-0.204316, 0.93606, 0.28644));
		ldata.light_dir = -ldata.light_dir;

		cam.Position    = glm::vec3(0.102807, 0.949880, 2.765358);
		cam.Front       = glm::vec3(0.0, 0.00, -1.0);
		cam.Up          = glm::vec3(0.0, 1.0, 0.0);
		cam.Right       = glm::vec3(1.0, 0.000000, 0.0);
		cam.Yaw         = -90.0;
		cam.Pitch       = 0;
		cam.updateCameraVectors();
	}
	else if (scene == "vokselia") {
		light_transforms["M"] = glm::mat4(1.0f);
		light_transforms["V_shadowmap"] = glm::lookAt(ldata.light_pos, model.bb_mid, glm::vec3(0.0, 1.0, 0.2));
		auto rsm_pos = model.bb_mid + glm::vec3(0, 100.f, 0);
		light_transforms["V_rsm"] = glm::lookAt(rsm_pos, model.bb_mid, glm::vec3(0.0, 1.0, 0.2));
		light_transforms["P"] = glm::ortho(model.min.x, model.max.x, model.min.y, model.max.y, sponza_n_v, sponza_f_v);
	}
	else if (scene == "sibenik") {	
		cutoff_angle = 15.0f;
		light_transforms["M"] = glm::mat4(1.0f);
		//light_transforms["V_shadowmap"] = glm::lookAt(ldata.light_pos, model.bb_mid, glm::vec3(0.0, 1.0, 0.2));
		light_transforms["V_rsm"] = glm::lookAt(glm::vec3(ldata.light_pos), glm::vec3(ldata.light_pos) + glm::vec3(ldata.light_dir), glm::vec3(0.0, 1.0, 0.2));
		light_transforms["P"] = glm::perspective(glm::radians(cutoff_angle), 1.0f, sponza_n_v, sponza_f_v);;
		ldata.light_dir = -ldata.light_dir;

		cam.Position = glm::vec3(-4.904220, 14.371094, -0.259506);
		cam.Front    = glm::vec3(0.949373, -0.313993, 0.009943);
		cam.Up       = glm::vec3(0.313976, 0.949425, 0.003288);
		cam.Right    = glm::vec3(-0.010472, 0.000000, 0.999945);
		cam.Yaw = -359.4;
		cam.Pitch = -18.3;
		cam.updateCameraVectors();
	}
	else if (scene == "conf") {
		//change
		cutoff_angle = 15.0f;
		light_transforms["M"] = glm::mat4(1.0f);
		//light_transforms["V_shadowmap"] = glm::lookAt(ldata.light_pos, model.bb_mid, glm::vec3(0.0, 1.0, 0.));
		light_transforms["P"] = glm::perspective(glm::radians(cutoff_angle), 1.0f, sponza_n_v, sponza_f_v);;
		light_transforms["V_rsm"] = glm::lookAt(glm::vec3(ldata.light_pos), glm::vec3(ldata.light_pos) + glm::vec3(ldata.light_dir), glm::vec3(0.0, 1.0, 0.2));
		ldata.light_dir = -ldata.light_dir;

		cam.Position = glm::vec3(243.096207, 39.140842, -7.070547);
		cam.Front = glm::vec3(-0.940934, -0.253758, 0.224164);
		cam.Up = glm::vec3(-0.246850, 0.967268, 0.058808);
		cam.Right = glm::vec3(-0.231750, 0.000000, -0.972775);
		cam.Yaw = 166.6;
		cam.Pitch = -14.7;
		cam.updateCameraVectors();
	}	

	timer_file.open("timer.txt", std::fstream::in | std::fstream::out | std::fstream::app);
	timer_file << scene + " performance:\n";	
	timer_file << std::to_string(num_clusters) + " clusters\n";
	//for moving lights
	float rotation = glm::radians(0.05f);
	
	//full screen quad
	quad fsquad;

	shader_table shader_list("shaders/shader_list.txt");
	std::unordered_map<std::string, framebuffer*> fblist;
	fblist["gbuffer"] = new framebuffer(fbo_type::G_BUFFER, w.Wid, w.Hei, 0);	   			
	fblist["interleaved_gbuffer"] = new framebuffer(fbo_type::G_BUFFER, w.Wid, w.Hei, 0);
	fblist["rsm"] = new framebuffer(fbo_type::RSM, rsm_w, rsm_h, 0);
	fblist["shadowmap"] = new framebuffer(fbo_type::SHADOW_MAP, sm_w, sm_h, 0);
	fblist["layered_fbo"] = new framebuffer(fbo_type::DEEP_G_BUFFER, ism_w, ism_h, num_clusters);
		
	//ssbo		
	SSBO brsm( rsm_w *rsm_h ,sizeof(GLfloat));	
	
	auto sampleData = sample_generator::gen_hammersley2d(num_view_samples);
	SSBO view_samples(num_view_samples , sizeof(GLfloat)*2, sampleData.data());
	
	SSBO ssbo_cdf_cols(rsm_w * rsm_h ,sizeof(GLfloat));
	
	SSBO ssbo_cdf_rows(rsm_h ,sizeof(GLfloat));

	SSBO ssbo_1st_vpls(num_vpls_first_bounce, sizeof(VPL));

	SSBO ssbo_vals(num_clusters, sizeof(VPL));

	SSBO vpl_to_vals((num_clusters * num_vpls_first_bounce), sizeof(unsigned int));

	SSBO vpl_count(num_clusters, sizeof(unsigned int));

	SSBO backface_vpls(num_vpls_second_bounce*num_clusters, sizeof(VPL));
	unsigned int zero_cnt = 0;
	SSBO backface_vpl_count(1, sizeof(unsigned int), &zero_cnt);

	UBO val_mats(num_clusters * sizeof(glm::mat4));
	val_mats.set_binding_point(0);
	
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glEnable(GL_PROGRAM_POINT_SIZE);

	double t0, tf, delta_t;
	t0 = tf = delta_t = 0;
	t0 = glfwGetTime();
	float gdelta_t = 0.f;
		
	std::vector<glm::vec2> cdf_r_vec = sample_generator::gen_uniform_samples2d(num_vpls_first_bounce, 0.0f, 1.0f);
	GLuint sample_cdf_tbo = texture2D::gen_tbo(sizeof(glm::vec2)*cdf_r_vec.size(), cdf_r_vec.data());

	std::vector<glm::vec2> sample_ssvp_data = sample_generator::gen_uniform_circle(num_vpls_second_bounce, 0.0f, 0.99f);
	//std::fill(sample_ssvp_data.begin(), sample_ssvp_data.end(), glm::vec2(0.09, 0.09));
	GLuint sample_ssvp = texture2D::gen_tbo(sizeof(glm::vec2) * sample_ssvp_data.size(), sample_ssvp_data.data());

	std::vector<glm::vec2> sample_rsm_data = sample_generator::gen_hammersley2d(num_vpls_first_bounce);
	GLuint sample_rsm = texture2D::gen_tbo(sizeof(glm::vec2) * sample_rsm_data.size(), sample_rsm_data.data());

	GLuint ishading_tex = texture2D::gen_tex(w.Wid, w.Hei, GL_FLOAT, GL_RGBA, GL_RGBA16F, GL_LINEAR, GL_CLAMP_TO_BORDER);
	GLuint deinterleave_tex = texture2D::gen_tex(w.Wid, w.Hei, GL_FLOAT, GL_RGBA, GL_RGBA16F, GL_LINEAR, GL_CLAMP_TO_BORDER);
	GLuint edge_tex = texture2D::gen_tex(w.Wid, w.Hei, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA8, GL_LINEAR, GL_CLAMP_TO_BORDER);
	GLuint out_tex = texture2D::gen_tex(w.Wid, w.Hei, GL_FLOAT, GL_RGBA, GL_RGBA16F, GL_LINEAR, GL_CLAMP_TO_BORDER);

	std::vector<glm::vec2> list = sample_generator::gen_uniform_circle(512, 0.0f, 1.0f);;
	
	std::for_each(list.begin(), list.end(), [](glm::vec2& x)
		{                                          
			x.x = x.x  * 2.0 - 1.0;
			x.y = x.y  * 2.0 - 1.0;
		});


	vao debug_sample_vao(list.data(), list.size()*sizeof(glm::vec2));
	debug_sample_vao.bind();
	debug_sample_vao.set_vertexarrays(2, sizeof(glm::vec2), 0, GL_FLOAT);
	
	vao first_bounce_vao;
	first_bounce_vao.from_ssbo(ssbo_1st_vpls.ssbo);
	first_bounce_vao.set_vertexarrays(4,sizeof(VPL), 0, GL_FLOAT);
	first_bounce_vao.set_vertexarrays(4, sizeof(VPL), offsetof(VPL, normal), GL_FLOAT);
	first_bounce_vao.set_vertexarrays(4, sizeof(VPL), offsetof(VPL, color), GL_FLOAT);

	vao cluster_vao; 
	cluster_vao.from_ssbo(ssbo_vals.ssbo);
	cluster_vao.set_vertexarrays(4,sizeof(VPL), 0, GL_FLOAT);
	cluster_vao.set_vertexarrays(4, sizeof(VPL), offsetof(VPL, normal), GL_FLOAT);
	cluster_vao.set_vertexarrays(4, sizeof(VPL), offsetof(VPL, color), GL_FLOAT);

	vao sec_bounce_vao; 
	sec_bounce_vao.from_ssbo(backface_vpls.ssbo);
	sec_bounce_vao.set_vertexarrays(4, sizeof(VPL), 0, GL_FLOAT);
	sec_bounce_vao.set_vertexarrays(4, sizeof(VPL), offsetof(VPL, normal), GL_FLOAT);
	sec_bounce_vao.set_vertexarrays(4, sizeof(VPL), offsetof(VPL, color), GL_FLOAT);	

	while (!w.should_close())// && (stats_iterator < STATS_SIZE))
	{		
		//process input
		scene_transforms["V"] = process_camera(cam, gdelta_t, lpos, cstate);
		
		if ((scene == "sponza") || (scene == "vokselia") || (scene == "sibenik")) 
		{			
			if (light_move_left) {
				ldata.light_pos = glm::rotate(ldata.light_pos, rotation, glm::vec3(1.0, 0.0, 0.0));
				ldata.light_dir = glm::normalize(ldata.light_pos - model.bb_mid);
				//rsm_pos = glm::rotate(rsm_pos, rotation, glm::vec3(1.0, 0.0, 0.0));
				light_transforms["V_rsm"] = glm::lookAt(ldata.light_pos, model.bb_mid, glm::vec3(0.0, 1.0, 0.2));
				light_transforms["V_shadowmap"] = glm::lookAt(ldata.light_pos, model.bb_mid, glm::vec3(0.0, 1.0, 0.2));
			}
			if (light_move_right) {
				ldata.light_pos = glm::rotate(ldata.light_pos, -rotation, glm::vec3(1.0, 0.0, 0.0));
				ldata.light_dir = glm::normalize(ldata.light_pos - model.bb_mid);
				//rsm_pos = glm::rotate(rsm_pos, -rotation, glm::vec3(1.0, 0.0, 0.0));
				light_transforms["V_rsm"] = glm::lookAt(ldata.light_pos, model.bb_mid, glm::vec3(0.0, 1.0, 0.2));
				light_transforms["V_shadowmap"] = glm::lookAt(ldata.light_pos, model.bb_mid, glm::vec3(0.0, 1.0, 0.2));
			}
		}

		//update screen
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		//rsm
		{		
			//gputimer a("rsm");
			glViewport(0, 0, rsm_w, rsm_h);
			generate_rsm(fblist["rsm"], shader_list.table["rsm"], light_transforms, model, ldata);
			//stats_track.gbuffer[stats_iterator] += a.stop();			
		}
		
		if (scene == "sponza")
		{//shadow map
			{				
				//gputimer a("shadowmap");
				glViewport(0, 0, sm_w, sm_h);
				glCullFace(GL_FRONT);
				generate_shadowmap(fblist["shadowmap"], shader_list.table["shadowmap"], light_transforms, model, ldata);
				glCullFace(GL_BACK);
				//stats_track.misc[stats_iterator] += a.stop();
			}
		}
		//gbuffer
		{			
			//gputimer a("gbuffer");
			glViewport(0, 0, fblist["gbuffer"]->w_res, fblist["gbuffer"]->h_res);
			generate_gbuffer(fblist["gbuffer"], shader_list.table["gbuffer"], scene_transforms, model);
			//stats_track.gbuffer[stats_iterator] += a.stop();
		}

		if (current_view == SCENE)
		{
			//BRSM
			{
				//gputimer a("Importance Map")
				//generate_importance_map(shader_list.table["BRSM"], fblist["gbuffer"], fblist["rsm"], brsm, view_samples);
			}
			//generate cdf
			{
				//gputimer a("CDF computation");
			    //generate_cdf_cols(shader_list.table["CDF_COLS"], brsm, ssbo_cdf_cols, rsm_w, rsm_h);
				//generate_cdf_rows(shader_list.table["CDF_ROWS"], ssbo_cdf_cols, ssbo_cdf_rows, rsm_w, rsm_h);		
			}
			//generate VPLs
			{
				//gputimer a("sample vpls");
				//generate_vpls(shader_list.table["SampleVpls"], fblist["rsm"], ssbo_cdf_cols, ssbo_cdf_rows, ssbo_1st_vpls, num_vpls_first_bounce, sample_cdf_tbo);					
				sample_vpls_from_rsm(shader_list.table["SampleRSM"], fblist["rsm"], ssbo_1st_vpls, num_vpls_first_bounce, sample_rsm);
				//stats_track.misc[stats_iterator] += a.stop();

			}
			//clusterize
			{
				
			    //gputimer a("cluster vpls");

				if (first_cluster)
				{
					generate_vals(shader_list.table["FirstCluster"], ssbo_1st_vpls, ssbo_vals, num_vpls_first_bounce, num_clusters);
					first_cluster = false;
				}
				{					
					bool first_cluster_pass = true;
					for (int i = 0; i < num_cluster_pass; i++)
					{
						//distance to vals
						calc_distance_to_val(shader_list.table["KMeans"], num_clusters, num_vpls_first_bounce, first_cluster_pass, ssbo_1st_vpls, ssbo_vals, vpl_to_vals, vpl_count);
						//update vals
						update_cluster_centers(shader_list.table["UpdateClusters"], num_vpls_first_bounce, num_clusters, ssbo_1st_vpls, ssbo_vals, vpl_to_vals, vpl_count);
						first_cluster_pass = false;
					}
				}

				//stats_track.misc[stats_iterator] += a.stop();				
			}

			ssbo_vals.bind();
			VPL* val_ptr = (VPL*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
			glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
			ssbo_vals.unbind();
			std::vector<glm::mat4> pView(num_clusters);
			{				
				//gputimer a("render paraboloids");
				for (int i = 0; i < num_clusters; i++)
				{
					glm::vec3 valpos(val_ptr[i].pos);
					glm::vec3 norm(val_ptr[i].normal);

					valpos = valpos + glm::normalize(norm) * 1.01f;

					//ONB construction
					float ks = (norm.z >= 0.0) ? 1.0 : -1.0;
					float ka = 1.0 / (1.0 + abs(norm.z));
					float kb = -ks * norm.x * norm.y * ka;
					glm::vec3 uu = glm::normalize(glm::vec3(1.0f - norm.x * norm.x * ka, ks * kb, -ks * norm.x));
					glm::vec3 vv = glm::normalize(glm::vec3(kb, ks - norm.y * norm.y * ka * ks, -norm.y));
					glm::mat4 val_lookat = glm::lookAt(valpos, valpos + (-norm), uu);
					pView[i] = val_lookat;
				}
				val_mats.upload_data(pView.data());
				//render visibility
				{
				    glViewport(0, 0, ism_w, ism_h);
					glCullFace(GL_FRONT);
					render_cluster_shadow_map(shader_list.table["ParabolicShadowMap"], fblist["layered_fbo"], paraboloid_near, paraboloid_far, model, num_clusters, light_transforms);
					glCullFace(GL_BACK);
					glViewport(0, 0, fblist["gbuffer"]->w_res, fblist["gbuffer"]->h_res);
				}
				//stats_track.paraboloid[stats_iterator] += a.stop();
			}

			//second bounce SSVP
			{
			   // gputimer a("SSVP");

				backface_vpl_count.bind();
				unsigned int zero = 0;
				glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(GLuint), &zero);
				backface_vpl_count.unbind();
				propagate_vpls(shader_list.table["SSVP"], sample_ssvp, fblist["layered_fbo"], backface_vpls, backface_vpl_count, 400, num_clusters);
				update_light_buffers(shader_list.table["UpdateLights"], ssbo_1st_vpls, ssbo_vals, vpl_to_vals, vpl_count, num_vpls_first_bounce, num_clusters);

				//stats_track.misc[stats_iterator] += a.stop();				
			}

			std::vector<unsigned int> l(1);
			backface_vpl_count.bind();
			auto ptr = (void*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
			memcpy(&l[0], ptr, sizeof(unsigned int));
			glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
			backface_vpl_count.unbind();
			//std::cout << l[0] << std::endl;


			std::vector<VPL> a;
			a.resize(l[0]);
			backface_vpls.bind();
			auto pt = (void*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
			memcpy(&a.data()[0], pt, sizeof(VPL)* l[0]);
			glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
			backface_vpls.unbind();


			//shade
			{				
				{
					split_gbuffer(shader_list.table["SplitGBuffer"], fblist["gbuffer"], fblist["interleaved_gbuffer"], interleave_rows, interleave_cols, fblist["gbuffer"]->w_res, fblist["gbuffer"]->h_res);
				}

				ssbo_1st_vpls.bindBase(1);
				ssbo_vals.bindBase(2);
				backface_vpls.bindBase(3);
				backface_vpl_count.bindBase(4);

				{
					float cutangle = glm::cos(glm::radians(cutoff_angle));

					//gputimer a("shade");

					shade_interleaved_buffer(shader_list.table["InterleavedShading"], ishading_tex, fblist["interleaved_gbuffer"], fblist["layered_fbo"], fblist["shadowmap"],
						fblist["rsm"], light_transforms, interleave_cols, interleave_rows, num_vpls_first_bounce,
						num_clusters, num_vpls_second_bounce*num_clusters, paraboloid_near, paraboloid_far, cam, ldata, see_bounce, wid, hei, direct_only, cutangle, is_spotlight);
				
					//std::cout << cutangle << std::endl;
					//deferred_lighting(shader_list.table["dlighting"], fsquad, scene_transforms, light_transforms, 
					//	              ldata, fblist["gbuffer"], fblist["shadowmap"], fblist["layered_fbo"],
					//	              num_vpls_first_bounce, paraboloid_near, paraboloid_far, cam);
										
					//stats_track.shade[stats_iterator] += a.stop();					
				}

				//std::cout << std::to_string(is_spotlight) << std::endl;

				backface_vpl_count.unbind();
				backface_vpls.unbind();
				ssbo_vals.unbind();
				ssbo_1st_vpls.unbind();
				
				{
					join_buffers(shader_list.table["Deinterleave"], ishading_tex, deinterleave_tex, interleave_rows, interleave_cols, w.Wid, w.Hei);
				}
				{
					//gputimer a("blur");
					{
						edge_detection(shader_list.table["EdgeDetection"], fblist["gbuffer"], edge_tex);
					}
					{				
						blur_pass(shader_list.table["blur_x"], shader_list.table["blur_y"], num_blur_pass, edge_tex, deinterleave_tex, out_tex, fsquad, w.Wid, w.Hei);
					}
					//stats_track.denoise[stats_iterator] += a.stop();					
				}
			}
			
			//final stage blit to screen	
			{
				//gputimer a("blit_to_screen");				
				//blit_to_screen(shader_list.table["blit_and_tonemap"], fblist["interleaved_gbuffer"], out_tex, sponza_n_v, sponza_f_v, fsquad);
				blit_to_screen(shader_list.table["blit_and_tonemap"], fblist["gbuffer"], out_tex, sponza_n_v, sponza_f_v, fsquad);
			}

			//debug_blit(shader_list.table["blit"], fsquad, fblist["layered_fbo"], cur_Layer);

			if (see_vpls)
			{
				glEnable(GL_DEPTH_TEST);
				first_bounce_vao.bind();
				shader_list.table["DEBUGDRAW"].use();
				shader_list.table["DEBUGDRAW"].setMat4("MVP", scene_transforms["P"] * scene_transforms["V"] * scene_transforms["M"]);
				shader_list.table["DEBUGDRAW"].setFloat("size", 15.0f);
				shader_list.table["DEBUGDRAW"].setVec4("vpl_color", glm::vec4(1.0, 0.0, 0.0, 1.0));
				glDrawArrays(GL_POINTS, 0, num_vpls_first_bounce);
				first_bounce_vao.unbind();

				cluster_vao.bind();
				shader_list.table["DEBUGDRAW"].use();
				shader_list.table["DEBUGDRAW"].setMat4("MVP", scene_transforms["P"] * scene_transforms["V"] * scene_transforms["M"]);
				shader_list.table["DEBUGDRAW"].setFloat("size", 40.0f);
				shader_list.table["DEBUGDRAW"].setVec4("vpl_color", glm::vec4(0.0, 1.0, 0.0, 1.0));				
				glDrawArrays(GL_POINTS, 0, num_clusters);		
					
				if (second)
				{
					sec_bounce_vao.bind();
					shader_list.table["DEBUGDRAW"].use();
					shader_list.table["DEBUGDRAW"].setMat4("MVP", scene_transforms["P"] * scene_transforms["V"] * scene_transforms["M"]);
					shader_list.table["DEBUGDRAW"].setFloat("size", 15.0f);
					shader_list.table["DEBUGDRAW"].setVec4("vpl_color", glm::vec4(0.0, 0.0, 1.0, 1.0));
					backface_vpl_count.bind();
					unsigned int* a = (unsigned int*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
					glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
					backface_vpl_count.unbind();
					glDrawArrays(GL_POINTS, 0, *a);
					sec_bounce_vao.unbind();
				}
				//draw light position					

			}
		}

		if (PARABOLIC == current_view) {
			debug_blit(shader_list.table["blit"], fsquad, fblist["layered_fbo"], cur_Layer);
			//glDisable(GL_DEPTH_TEST);
			//debug_sample_vao.bind();
			//shader_list.table["DEBUGDRAW"].use();
			//shader_list.table["DEBUGDRAW"].setMat4("MVP", glm::mat4(1.0));
			//shader_list.table["DEBUGDRAW"].setFloat("size", 15.0f);
			//shader_list.table["DEBUGDRAW"].setVec4("vpl_color", glm::vec4(0.0, 0.0, 1.0, 1.0));		;
			//glDrawArrays(GL_POINTS, 0, list.size());
			//debug_sample_vao.unbind();	
			//glEnable(GL_DEPTH_TEST);
		}
	
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
			//render_gui(ms, fps);
		}

		
		stats_iterator++;

		perf_counter++;
		w.swap();
		w.poll_ev();
	}

	std::cout << "leaving" << std::endl;
	perf_journal.append("\n");
	double gbuf = average(stats_track.gbuffer);
	double misc = average(stats_track.misc);
	double para = average(stats_track.paraboloid);
	double shad = average(stats_track.shade);
	double deno = average(stats_track.denoise);
	perf_journal.append("Gbuffer:    " +  std::to_string(gbuf) + "\n");
	perf_journal.append("Misc:       " +  std::to_string(misc) + "\n");
	perf_journal.append("Paraboloid: " +  std::to_string(para) + "\n");
	perf_journal.append("Shade:      " +  std::to_string(shad) + "\n");
	perf_journal.append("Denoise:    " +  std::to_string(deno) + "\n");
	perf_journal.append("\n");
	timer_file << perf_journal.c_str();
	timer_file.close();

	//destroy_gui();
	w.close();

	return 0;
}
