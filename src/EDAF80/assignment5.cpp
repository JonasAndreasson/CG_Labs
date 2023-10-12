#include "assignment5.hpp"

#include "config.hpp"
#include "core/Bonobo.h"
#include "core/FPSCamera.h"
#include "core/helpers.hpp"
#include "core/node.hpp"
#include "core/ShaderProgramManager.hpp"
#include "parametric_shapes.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <glm/common.hpp>
#include <imgui.h>
#include <tinyfiledialogs.h>
#include <clocale>
#include <stdexcept>

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

bool testBoatTorus(glm::vec3 boat_pos, glm::vec3 torus_pos, glm::vec3 torus_normal,float torus_outer_radius, float torus_inner_radius) {
	float disc_radius = torus_outer_radius - torus_inner_radius;
	if (!testSphereSphere(boat_pos, 0.01f, torus_pos, disc_radius)) return false;
	glm::vec3 pq = (glm::vec3(0, 1, 0) * disc_radius);
	glm::vec3 pr = (torus_normal * disc_radius);
	glm::vec3 plane_coef = glm::cross(pq, pr);
	glm::vec3 norm = torus_inner_radius*glm::normalize(plane_coef);
	float g = glm::dot(boat_pos, plane_coef);
	float lbound = glm::dot(torus_pos - norm, plane_coef);
	float ubound = glm::dot(torus_pos + norm, plane_coef);
	return  (g >= lbound && g <= ubound) && testSphereSphere(boat_pos, 0.01f, torus_pos, disc_radius);
}

void edaf80::Assignment5::run()
{
	// Set up the camera
	glm::vec3 camera_displacement = glm::vec3(-1.0f, 1.0f, -1.0f);
	mCamera.mWorld.SetTranslate(camera_displacement);
	mCamera.mMouseSensitivity = glm::vec2(0.003f);
	mCamera.mMovementSpeed = glm::vec3(0.0f); // 3 m/s => 10.8 km/h
	mCamera.mWorld.LookAt(glm::vec3(2.0f,1.0f,0.0f));
	auto camera_position = mCamera.mWorld.GetTranslation();
	float elapsed_time_s = 0.0f;
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
		goal.get_transform().SetTranslate(glm::vec3(10 + i * 100, 0, 0));
		goal.get_transform().SetRotateZ(glm::half_pi<float>()); //Stands it up on it's edge
		goal.get_transform().Rotate(0.5f*i, glm::vec3(1, 0, 0)); //rotates it around the world y axis
		goal.add_texture("rainbow", rainbow_texture, GL_TEXTURE_2D);
		goal.set_program(&goal_shader);
		goal_posts.push_back(goal);
	}



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
	float acc_speed = 0.1f;
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
		elapsed_time_s += std::chrono::duration<float>(deltaTimeUs).count();
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
			acc.x =0;
			acc.z = 0;
		}
		if (inputHandler.GetKeycodeState(GLFW_KEY_W) & JUST_RELEASED) {
			acc.x = 0;
			acc.z = 0;
		}
		float dt = std::chrono::duration<float>(deltaTimeUs).count();
		//acc += friction * friction_vec;
		camera_follow_matrix =  glm::mat3(camera_follow_distance * glm::vec3(cos(angle), 0.0f, 0.0f), glm::vec3(0.0f, 5.0f, 0.0f), camera_follow_distance * glm::vec3(0.0f, 0.0f, -sin(angle)));
		turning = glm::mat3(glm::vec3(cos(angle), 0.0f, 0.0f), glm::vec3(0.0f,1.0f,0.0f), glm::vec3(0.0f, 0.0f, -sin(angle)));
		glm::vec3 speed_loss = friction * friction_vec * dt;
		vel += acc * dt + speed_loss;
		if (vel.x < 0) {
			vel.x = 0;
		}
		if (vel.z < 0) {
			vel.z = 0;
		}
		if (glm::distance(vel, glm::vec3(0.0f)) > 20) {
			vel = 20.0f * glm::normalize(vel);
		}
		pos += turning * vel * dt;

		//
		// Todo: If you need to handle inputs, you can do it here
		//


		mWindowManager.NewImGuiFrame();

		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);


		if (!shader_reload_failed) {
			mCamera.mWorld.SetTranslate(camera_follow_matrix *camera_displacement + pos);
			mCamera.mWorld.LookAt(pos+glm::vec3(10.0f*cos(angle), 0.0f, 10.0f*-sin(angle)));
			glDisable(GL_DEPTH_TEST);
			skybox.get_transform().SetTranslate(glm::vec3(camera_position));
			skybox.render(mCamera.GetWorldToClipMatrix());
			glEnable(GL_DEPTH_TEST);
			surface.get_transform().SetTranslate(glm::vec3(pos));
			surface.render(mCamera.GetWorldToClipMatrix());
			for (Node& b : boat) {
				b.get_transform().SetTranslate(pos);
				b.get_transform().SetRotate(angle, glm::vec3(0, 1, 0));
				b.render(mCamera.GetWorldToClipMatrix());
			}
			for (Node& g : goal_posts) {
				if (testBoatTorus(pos, g.get_transform().GetTranslation(), g.get_transform().GetFront(), 20.0f, 3.0f)) g.get_transform().SetTranslate(pos+glm::vec3(40,0,40));
				g.render(mCamera.GetWorldToClipMatrix());
			}
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
