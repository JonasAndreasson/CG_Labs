#include "assignment5.hpp"

#include "config.hpp"
#include "core/Bonobo.h"
#include "core/FPSCamera.h"
#include "core/helpers.hpp"
#include "core/node.hpp"
#include "core/ShaderProgramManager.hpp"
#include "parametric_shapes.hpp"
#include <stdlib.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/common.hpp>
#include <imgui.h>
#include <tinyfiledialogs.h>
#include <clocale>
#include <stdexcept>

enum game_states
{
	NOT_STARTED = 0,
	RUNNING = 1,
	PAUSED = 2,
	GAME_OVER = 3
};

enum collision
{
	OUTSIDE = 0,
	INSIDE = 1,
	HIT = 2
};


edaf80::Assignment5::Assignment5(WindowManager& windowManager) :
	mCamera(0.5f * glm::half_pi<float>(),
	        static_cast<float>(config::resolution_x) / static_cast<float>(config::resolution_y),
	        0.01f, 1000.0f),
	inputHandler(), mWindowManager(windowManager), window(nullptr)
{
	WindowManager::WindowDatum window_datum{ inputHandler, mCamera, config::resolution_x, config::resolution_y, 0, 0, 0, 0};

	window = mWindowManager.CreateGLFWWindow("EDAF80: Assignment 5", window_datum, config::msaa_rate);
	if (window == nullptr) {
		throw std::runtime_error("Failed to get a window: aborting!");
	}

	bonobo::init();
}

edaf80::Assignment5::~Assignment5()
{
	bonobo::deinit();
}

bool testSphereSphere(glm::vec3 p1, float r1, glm::vec3 p2, float r2) {


	return std::abs(glm::distance(p1, p2)) < (r1 + r2);
}

collision testBoatTorus(glm::vec3 boat_pos, glm::vec3 torus_pos, glm::vec3 torus_normal,float torus_outer_radius, float torus_inner_radius) {
	float disc_radius = torus_outer_radius - torus_inner_radius;
	if (!testSphereSphere(boat_pos, 0.01f, torus_pos, torus_outer_radius)) return OUTSIDE;
	glm::vec3 pq = (glm::vec3(0, 1, 0) * disc_radius);
	glm::vec3 pr = (torus_normal * disc_radius);
	glm::vec3 plane_coef = glm::cross(pq, pr);
	glm::vec3 inside = torus_inner_radius*glm::normalize(plane_coef);
	glm::vec3 outside = torus_outer_radius * glm::normalize(plane_coef);
	float g = glm::dot(boat_pos, plane_coef);
	float inside_lbound = glm::dot(torus_pos - inside, plane_coef);
	float inside_ubound = glm::dot(torus_pos + inside, plane_coef);
	if ((g >= inside_lbound && g <= inside_ubound) && testSphereSphere(boat_pos, 0.01f, torus_pos, disc_radius))
	{
		return INSIDE;
	}
	else if ((g >= inside_lbound && g <= inside_ubound) && testSphereSphere(boat_pos, 0.01f, torus_pos, torus_outer_radius))
	{
		return HIT;
	}
	else
	{
		return OUTSIDE;
	}
	
}

float random_angle(float max)
{
	return max * rand() / (RAND_MAX + 1.0f);
}

glm::vec3 random_pos(glm::vec3 boat_pos, float d)
{
	if (d > 300) d = 300;
	float angle = random_angle(glm::two_pi<float>());
	glm::vec3 new_pos = glm::vec3(0.0);
	new_pos.x = d * cos(angle);
	new_pos.z = d * sin(angle);
	return new_pos;
}


void edaf80::Assignment5::run()
{
	// Set up the camera
	glm::vec3 camera_displacement = glm::vec3(-1.0f, 1.0f, -1.0f);
	mCamera.mWorld.SetTranslate(camera_displacement);
	mCamera.mMouseSensitivity = glm::vec2(0.003f);
	mCamera.mMovementSpeed = glm::vec3(3.0f); // 3 m/s => 10.8 km/h
	mCamera.mWorld.LookAt(glm::vec3(2.0f,1.0f,0.0f));
	game_states game_state = NOT_STARTED;
	auto camera_position = mCamera.mWorld.GetTranslation();
	float elapsed_time_s = 0.0f;
	float remaining_time = 30.0f;
	int score_counter = 0;
	auto light_position = glm::vec3(0.0f, 10.0f, 0.0f);
	// Create the shader programs
	ShaderProgramManager program_manager;
	GLuint fallback_shader = 0u;
	program_manager.CreateAndRegisterProgram("Fallback",
	                                         { { ShaderType::vertex, "common/fallback.vert" },
	                                           { ShaderType::fragment, "common/fallback.frag" } },
	                                         fallback_shader);
	if (fallback_shader == 0u) {
		LogError("Failed to load fallback shader");
		return;
	}
	//Sampler
	auto s = [](GLuint a) {
		glSamplerParameteri(a, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		};
	
	GLuint sampler = bonobo::createSampler(s);



	// Skybox 
	GLuint skybox_shader = 0u;
	program_manager.CreateAndRegisterProgram("Skybox",
		{ { ShaderType::vertex, "EDAF80/skybox.vert" },
		  { ShaderType::fragment, "EDAF80/skybox.frag" } },
		skybox_shader);

	if ((skybox_shader) == 0u)
		LogError("Failed to load skybox shader");

	GLuint water_shader = 0u;
	program_manager.CreateAndRegisterProgram("Water",
		{ { ShaderType::vertex, "EDAF80/water.vert"},
		{ShaderType::fragment, "EDAF80/water.frag"} }, water_shader);
	if (water_shader == 0u) {
		LogError("Failed to load water shader");
		return;
	}
	GLuint boat_shader = 0u;
	program_manager.CreateAndRegisterProgram("Boat",
		{ { ShaderType::vertex, "EDAF80/boat.vert" },
		{ ShaderType::fragment, "EDAF80/boat.frag" } },
		boat_shader);
	if (boat_shader == 0u) {
		LogError("Failed to load boat shader");
		return;
	}
	GLuint flag_shader = 0u;
	program_manager.CreateAndRegisterProgram("Boat",
		{ { ShaderType::vertex, "EDAF80/flag.vert" },
		{ ShaderType::fragment, "EDAF80/boat.frag" } },
		flag_shader);
	if (flag_shader == 0u) {
		LogError("Failed to load boat shader");
		return;
	}

	GLuint goal_shader = 0u;
	program_manager.CreateAndRegisterProgram("Goal",
		{ { ShaderType::vertex, "EDAF80/torus.vert" },
		{ ShaderType::fragment, "EDAF80/torus.frag" } },
		goal_shader);
	if (goal_shader == 0u) {
		LogError("Failed to load goal shader");
		return;
	}

	GLuint score_shader = 0u;
	program_manager.CreateAndRegisterProgram("Score",
		{ { ShaderType::vertex, "EDAF80/score.vert" },
		{ ShaderType::fragment, "EDAF80/score.frag" } },
		score_shader);
	if (score_shader == 0u) {
		LogError("Failed to load score shader");
		return;
	}




	
	GLuint cubemap = bonobo::loadTextureCubeMap(
		config::resources_path("cubemaps/SemiSky/sky-right.png"),
		config::resources_path("cubemaps/SemiSky/sky-left.png"),
		config::resources_path("cubemaps/SemiSky/sky-top.png"),
		config::resources_path("cubemaps/SemiSky/sky-bottom.png"),
		config::resources_path("cubemaps/SemiSky/sky-front.png"),
		config::resources_path("cubemaps/SemiSky/sky-back.png")
	);
	GLuint wave_texture = bonobo::loadTexture2D(config::resources_path("textures/waves.png"));
	GLuint rainbow_texture = bonobo::loadTexture2D(config::resources_path("textures/Seamless_Rainbow.png"));
	GLuint not_running_texture = bonobo::loadTexture2D(config::resources_path("textures/not-running.png"));
	GLuint pause_texture = bonobo::loadTexture2D(config::resources_path("textures/pause.png"));
	GLuint game_over_texture = bonobo::loadTexture2D(config::resources_path("textures/game-over.png"));


	auto skybox_shape = parametric_shapes::createSphere(100.0f, 100u, 100u);
	if (skybox_shape.vao == 0u) {
		LogError("Failed to retrieve the mesh for the skybox");
		return;
	}
	Node skybox,surface;
	skybox.set_geometry(skybox_shape);
	skybox.add_texture("cubemap", cubemap, GL_TEXTURE_CUBE_MAP);
	skybox.set_program(&skybox_shader);

	auto ocean_size = 300u;
	auto half_ocean_size = ocean_size / 2;
	auto surface_shape = parametric_shapes::createQuad(ocean_size, ocean_size, 1000, 1000);
	if (surface_shape.vao == 0u)
	{
		LogError("Failed to retrive mesh for the surface");
		return;
	}
	auto water_uniforms = [&elapsed_time_s, &camera_position](GLuint program) {
		glUniform1f(glGetUniformLocation(program, "elapsed_time_s"), elapsed_time_s);
		glUniform3fv(glGetUniformLocation(program, "camera_position"), 1, glm::value_ptr(camera_position));
		};
	surface.set_geometry(surface_shape);
	surface.set_program(&water_shader, water_uniforms);
	surface.add_texture("wave_texture", wave_texture, GL_TEXTURE_2D);
	surface.add_texture("cubemap", cubemap, GL_TEXTURE_CUBE_MAP);


	auto boat_shapes = bonobo::loadObjects(config::resources_path("models/ship.obj"));
	std::vector<Node>boat;
	for (auto boat_shape : boat_shapes) {
		Node boat_sub;
		boat_sub.set_geometry(boat_shape);
		auto boat_uniforms = [&elapsed_time_s, &camera_position, &light_position](GLuint program) {
			glUniform1f(glGetUniformLocation(program, "elapsed_time_s"), elapsed_time_s);
			glUniform3fv(glGetUniformLocation(program, "camera_position"), 1, glm::value_ptr(camera_position));
			};
		if (boat_shape.name == "//flag_Cube.005") {
			boat_sub.set_program(&flag_shader, boat_uniforms);
		}
		else {
			boat_sub.set_program(&boat_shader, boat_uniforms);
		}
		boat.push_back(boat_sub);
	}

	std::vector<Node> goal_posts;
	for (unsigned int i = 0; i < 5; i++) {
		Node goal;
		goal.set_geometry(parametric_shapes::createTorus(20.0f, 3.0f, 100u, 100u));
		goal.get_transform().SetRotateZ(glm::half_pi<float>()); //Stands it up on it's edge
		goal.get_transform().SetTranslate(random_pos(glm::vec3(0,0,0), 100.0f));
		goal.get_transform().Rotate(random_angle(glm::two_pi<float>()), glm::vec3(1, 0, 0));
		goal.add_texture("rainbow", rainbow_texture, GL_TEXTURE_2D);
		goal.set_program(&goal_shader);
		goal_posts.push_back(goal);
		
	}
	int* width = new int(0);
	int* height = new int(0);
	glfwGetWindowSize(window, width, height);
	glm::vec2 res = glm::vec2(*width, *height);
	delete width;
	width = nullptr;
	delete height;
	height = nullptr;

	
	auto score_uniform = [&elapsed_time_s, &score_counter, &res](GLuint program) {
		glUniform1f(glGetUniformLocation(program, "elapsed_time_s"), elapsed_time_s);
		glUniform1i(glGetUniformLocation(program, "score"), score_counter);
		glUniform2fv(glGetUniformLocation(program, "iResolution"), 1, glm::value_ptr(res));
		};
	std::cout << res << '\n';
	auto score_shape = parametric_shapes::createQuad(40, 40, 80, 80);
	Node score;
	score.set_geometry(score_shape);
	score.get_transform().SetRotateZ(glm::half_pi<float>());
	score.set_program(&score_shader, score_uniform);

	glClearDepthf(1.0f);
	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glEnable(GL_DEPTH_TEST);


	auto lastTime = std::chrono::high_resolution_clock::now();

	bool show_logs = true;
	bool show_gui = true;
	bool shader_reload_failed = false;
	bool show_basis = false;
	float basis_thickness_scale = 1.0f;
	float basis_length_scale = 1.0f;
	float angle = 0.0f;
	float rotation_speed = 0.01f;
	glm::vec3 acc = glm::vec3(0.0f);
	float friction = -0.05f;
	glm::vec3 friction_vec = glm::vec3(1.0f, 0.0f, 1.0f);
	float acc_speed = 0.5f;
	float max_acc = 2.0f;
	glm::mat3 turning = glm::mat3(glm::vec3(cos(angle), 0.0f, 0.0f), glm::vec3(0.0f,1.0f,0.0f), glm::vec3(0.0f,0.0f,-sin(angle)));
	float camera_follow_distance = 20.0f;
	glm::mat3 camera_follow_matrix;
	glm::vec3 pos = glm::vec3(0.0f);
	glm::vec3 vel = glm::vec3(0.0f);
	while (!glfwWindowShouldClose(window)) {
		auto const nowTime = std::chrono::high_resolution_clock::now();
		auto const deltaTimeUs = std::chrono::duration_cast<std::chrono::microseconds>(nowTime - lastTime);
		lastTime = nowTime;
		camera_position = mCamera.mWorld.GetTranslation();
		auto& io = ImGui::GetIO();
		inputHandler.SetUICapture(io.WantCaptureMouse, io.WantCaptureKeyboard);

		glfwPollEvents();
		inputHandler.Advance();
		mCamera.Update(deltaTimeUs, inputHandler);

		if (inputHandler.GetKeycodeState(GLFW_KEY_R) & JUST_PRESSED) {
			shader_reload_failed = !program_manager.ReloadAllPrograms();
			if (shader_reload_failed)
				tinyfd_notifyPopup("Shader Program Reload Error",
				                   "An error occurred while reloading shader programs; see the logs for details.\n"
				                   "Rendering is suspended until the issue is solved. Once fixed, just reload the shaders again.",
				                   "error");
		}
		if (inputHandler.GetKeycodeState(GLFW_KEY_F3) & JUST_RELEASED)
			show_logs = !show_logs;
		if (inputHandler.GetKeycodeState(GLFW_KEY_F2) & JUST_RELEASED)
			show_gui = !show_gui;
		if (inputHandler.GetKeycodeState(GLFW_KEY_F11) & JUST_RELEASED)
			mWindowManager.ToggleFullscreenStatusForWindow(window);

		// Retrieve the actual framebuffer size: for HiDPI monitors,
		// you might end up with a framebuffer larger than what you
		// actually asked for. For example, if you ask for a 1920x1080
		// framebuffer, you might get a 3840x2160 one instead.
		// Also it might change as the user drags the window between
		// monitors with different DPIs, or if the fullscreen status is
		// being toggled.
		int framebuffer_width, framebuffer_height;
		glfwGetFramebufferSize(window, &framebuffer_width, &framebuffer_height);
		glViewport(0, 0, framebuffer_width, framebuffer_height);

		if (inputHandler.GetKeycodeState(GLFW_KEY_D) & PRESSED)
			angle -= rotation_speed;
		if (inputHandler.GetKeycodeState(GLFW_KEY_A) & PRESSED)
			angle += rotation_speed;
		if (inputHandler.GetKeycodeState(GLFW_KEY_W) & PRESSED) {
			acc.x += acc_speed;
			acc.z += acc_speed;
			if (acc.x > max_acc) acc.x = max_acc;
			if (acc.z > max_acc) acc.z = max_acc;
		}
		if (inputHandler.GetKeycodeState(GLFW_KEY_S) & PRESSED) {
			acc.x = 0;
			acc.z = 0;
		}
		if (inputHandler.GetKeycodeState(GLFW_KEY_W) & JUST_RELEASED) {
			acc.x = 0;
			acc.z = 0;
		}
		if (inputHandler.GetKeycodeState(GLFW_KEY_ENTER) & JUST_PRESSED) {
			if (game_state == RUNNING) game_state = PAUSED;
			else if (game_state == PAUSED) game_state = RUNNING;
			else if (game_state == NOT_STARTED) game_state = RUNNING;

		}
		//
		// Todo: If you need to handle inputs, you can do it here
		//


		mWindowManager.NewImGuiFrame();

		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
		float dt;
		switch (game_state) {

		case RUNNING:
			elapsed_time_s += std::chrono::duration<float>(deltaTimeUs).count();
			dt = std::chrono::duration<float>(deltaTimeUs).count();
			remaining_time -= dt;
			if (remaining_time < 0)
			{
				game_state = GAME_OVER;
				break;
			}
			camera_follow_matrix = glm::mat3(camera_follow_distance * glm::vec3(cos(angle), 0.0f, 0.0f), glm::vec3(0.0f, 5.0f, 0.0f), camera_follow_distance * glm::vec3(0.0f, 0.0f, -sin(angle)));
			turning = glm::mat3(glm::vec3(cos(angle), 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -sin(angle)));
			glm::vec3 speed_loss = (friction * vel + friction_vec * friction) * dt;
			vel += acc * dt + speed_loss;
			if (glm::distance(vel, glm::vec3(0.0f)) > 40) {
				vel = 40.0f * glm::normalize(vel);
			}
			pos += turning * vel * dt;
			if (!shader_reload_failed) {
				mCamera.mWorld.SetTranslate(camera_follow_matrix * camera_displacement + pos);
				mCamera.mWorld.LookAt(pos + glm::vec3(10.0f * cos(angle), 0.0f, 10.0f * -sin(angle)));
				glDisable(GL_DEPTH_TEST);
				skybox.get_transform().SetTranslate(glm::vec3(camera_position));
				skybox.render(mCamera.GetWorldToClipMatrix());

				glEnable(GL_DEPTH_TEST);
				surface.get_transform().SetTranslate(glm::vec3(pos));
				surface.render(mCamera.GetWorldToClipMatrix());
				
				for (Node& g : goal_posts) {
					auto col_check = testBoatTorus(pos, g.get_transform().GetTranslation(), g.get_transform().GetFront(), 20.0f, 3.0f);
					if (col_check == INSIDE)
					{
						g.get_transform().SetTranslate(random_pos(pos, score_counter*50.0f));
						g.get_transform().Rotate(random_angle(glm::two_pi<float>()), glm::vec3(1, 0, 0));
						std::cout << "pos: " << pos << "torus_pos: " << g.get_transform().GetTranslation() << '\n';
						++score_counter;
						remaining_time += 3.0f;
					}
					else if (col_check == HIT) {
						if (glm::distance(vel, glm::vec3(0, 0, 0)) > 0) {
							vel *= -1.0f;
						}
						pos += turning * vel * dt;
						vel *= 0.5f;
						if (glm::distance(vel, glm::vec3(0, 0, 0)) > -10) {
							vel = 10.0f * glm::normalize(vel);
						}
						
					}

					g.render(mCamera.GetWorldToClipMatrix());
				}

				for (Node& b : boat) {
					b.get_transform().SetTranslate(pos);
					b.get_transform().SetRotate(angle, glm::vec3(0, 1, 0));
					b.render(mCamera.GetWorldToClipMatrix());
				}
			}
			break;
		case NOT_STARTED:
			dt = 0;
			bonobo::displayTexture(glm::vec2(-1, -1), glm::vec2(1, 1), not_running_texture, sampler, glm::vec4(0, 1, 2, 3), glm::vec2(framebuffer_width, framebuffer_height));
			break;

		case PAUSED:
			dt = 0;
			bonobo::displayTexture(glm::vec2(-1, -1), glm::vec2(1, 1), pause_texture, sampler,glm::vec4(0, 1, 2, 3), glm::vec2(framebuffer_width, framebuffer_height));
			break;
		case GAME_OVER:
			dt = 0;
			bonobo::displayTexture(glm::vec2(-1, -1), glm::vec2(1, 1), game_over_texture, sampler, glm::vec4(0, 1, 2, 3), glm::vec2(framebuffer_width, framebuffer_height));
			break;
		}

		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		//
		// Todo: If you want a custom ImGUI window, you can set it up
		//       here
		//
		bool const opened = ImGui::Begin("Scene Controls", nullptr, ImGuiWindowFlags_None);
		if (opened) {
			ImGui::Checkbox("Show basis", &show_basis);
			ImGui::SliderFloat("Basis thickness scale", &basis_thickness_scale, 0.0f, 100.0f);
			ImGui::SliderFloat("Basis length scale", &basis_length_scale, 0.0f, 100.0f);
		}
		ImGui::End();

		bool const game_info = ImGui::Begin("Game Info", nullptr, ImGuiWindowFlags_None);
		if (game_info) {
			
			std::string score_string("Score: " + std::to_string(score_counter));
			std::string speed_string("Speed: " + std::to_string(glm::distance(vel, glm::vec3(0, 0, 0))));
			std::string time_string;
			if (game_state != GAME_OVER) {
				time_string = "Remaining time:" + std::to_string(remaining_time);
			}
			else {
				time_string = "Time Survived:" + std::to_string(int(elapsed_time_s))+" seconds!!!";
			}
			ImGui::Text(score_string.c_str());
			ImGui::Text(speed_string.c_str());
			ImGui::Text(time_string.c_str());
		}
		ImGui::End();

		if (show_basis)
			bonobo::renderBasis(basis_thickness_scale, basis_length_scale, mCamera.GetWorldToClipMatrix());
		if (show_logs)
			Log::View::Render();
		mWindowManager.RenderImGuiFrame(show_gui);

		glfwSwapBuffers(window);
	}
}

int main()
{
	std::setlocale(LC_ALL, "");

	Bonobo framework;

	try {
		edaf80::Assignment5 assignment5(framework.GetWindowManager());
		assignment5.run();
	} catch (std::runtime_error const& e) {
		LogError(e.what());
	}
}
