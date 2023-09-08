#include "CelestialBody.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/trigonometric.hpp>
#include <stack>
#include "core/helpers.hpp"
#include "core/Log.h"

CelestialBody::CelestialBody(bonobo::mesh_data const& shape,
                             GLuint const* program,
                             GLuint diffuse_texture_id)
{
	_body.node.set_geometry(shape);
	_body.node.add_texture("diffuse_texture", diffuse_texture_id, GL_TEXTURE_2D);
	_body.node.set_program(program);
	
}

glm::mat4 CelestialBody::render(std::chrono::microseconds elapsed_time,
                                glm::mat4 const& view_projection,
                                glm::mat4 const& parent_transform,
                                bool show_basis)
{
	// Convert the duration from microseconds to seconds.
	auto const elapsed_time_s = std::chrono::duration<float>(elapsed_time).count();
	// If a different ratio was needed, for example a duration in
	// milliseconds, the following would have been used:
	// auto const elapsed_time_ms = std::chrono::duration<float, std::milli>(elapsed_time).count();
	//testign for pushe from school PC
	_body.spin.rotation_angle += _body.spin.speed*elapsed_time_s;
	_body.orbit.rotation_angle += _body.orbit.speed*elapsed_time_s;
	S = glm::scale(glm::mat4(1.0f), _body.scale); //2nd arg is the scale.
	R_1s = glm::rotate(glm::mat4(1.0f), _body.spin.rotation_angle,glm::vec3(0.0f, 1.0f,0.0f));
	R_2s = glm::rotate(glm::mat4(1.0f), _body.spin.axial_tilt, glm::vec3(0.0f, 0.0f, 1.0f));
	glm::mat4 R_3s= glm::rotate(glm::mat4(1.0f), -_body.orbit.rotation_angle, glm::vec3(0.0f, 1.0f, 0.0f));
	T_o = glm::translate(glm::mat4(1.0f), glm::vec3(_body.orbit.radius, 0.0f, 0.0f));
	R_1o = glm::rotate(glm::mat4(1.0f), _body.orbit.rotation_angle, glm::vec3(0.0f, 1.0f, 0.0f));
	R_2o = glm::rotate(glm::mat4(1.0f), _body.orbit.inclination, glm::vec3(0.0f, 0.0f, 1.0f));
	world = parent_transform*R_2o*  R_1o *  T_o * R_3s *  R_2s  * R_1s *S;
	transformed = parent_transform * R_2o * R_1o * T_o * R_3s * R_2s;

	if (show_basis)
	{
		bonobo::renderBasis(1.0f, 2.0f, view_projection, world);
	}
	if (_ring.is_set) {
		glm::mat4 ring_S = glm::scale(glm::mat4(1.0f), glm::vec3(_ring.scale,1.0f));
		glm::mat4 ring_R1 = glm::rotate(glm::mat4(1.0f), glm::half_pi<float>(), glm::vec3(1.0f, 0.0f, 0.0f));
		glm::mat4 ring_world = transformed *ring_R1 * ring_S;
		_ring.node.render(view_projection, ring_world);
	}

	// Note: The second argument of `node::render()` is supposed to be the
	// parent transform of the node, not the whole world matrix, as the
	// node internally manages its local transforms. However in our case we
	// manage all the local transforms ourselves, so the internal transform
	// of the node is just the identity matrix and we can forward the whole
	// world matrix.
	_body.node.render(view_projection, world);

	return transformed;
}

void CelestialBody::add_child(CelestialBody* child)
{
	_children.push_back(child);
}

std::vector<CelestialBody*> const& CelestialBody::get_children() const
{
	return _children;
}

void CelestialBody::set_orbit(OrbitConfiguration const& configuration)
{
	_body.orbit.radius = configuration.radius;
	_body.orbit.inclination = configuration.inclination;
	_body.orbit.speed = configuration.speed;
	_body.orbit.rotation_angle = 0.0f;
}

void CelestialBody::set_scale(glm::vec3 const& scale)
{
	_body.scale = scale;
}

void CelestialBody::set_spin(SpinConfiguration const& configuration)
{
	_body.spin.axial_tilt = configuration.axial_tilt;
	_body.spin.speed = configuration.speed;
	_body.spin.rotation_angle = 0.0f;
}

glm::vec3 CelestialBody::follow_vector() {
	glm::mat4 T_f = glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	return glm::vec3((R_2o*R_1o* T_f*T_o*glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)));
}

glm::vec3 CelestialBody::get_pos() {
	return glm::vec3((transformed * glm::vec4(0.0f,0.0f,0.0f,1.0f)));
}

void CelestialBody::set_ring(bonobo::mesh_data const& shape,
                             GLuint const* program,
                             GLuint diffuse_texture_id,
                             glm::vec2 const& scale)
{
	_ring.node.set_geometry(shape);
	_ring.node.add_texture("diffuse_texture", diffuse_texture_id, GL_TEXTURE_2D);
	_ring.node.set_program(program);

	_ring.scale = scale;

	_ring.is_set = true;
}
