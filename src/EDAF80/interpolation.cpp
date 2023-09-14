#include "interpolation.hpp"

glm::vec3
interpolation::evalLERP(glm::vec3 const& p0, glm::vec3 const& p1, float const x)
{
	glm::vec2 X = glm::vec2(1, x);
	glm::mat2 mat = glm::mat2(glm::vec2(1, -1), glm::vec2(0, 1));
	auto p_x = glm::dot(X,(mat * glm::vec2(p0.x, p1.x)));
	auto p_y = glm::dot(X,(mat *  glm::vec2(p0.y, p1.y)));
	auto p_z = glm::dot(X,(mat *  glm::vec2(p0.z, p1.z)));
	return glm::vec3(p_x, p_y,p_z);
}

glm::vec3
interpolation::evalCatmullRom(glm::vec3 const& p0, glm::vec3 const& p1,
                              glm::vec3 const& p2, glm::vec3 const& p3,
                              float const t, float const x)
{
	glm::vec4 X = glm::vec4(1, x, x * x, x * x * x);
	glm::mat4 mat = glm::mat4(glm::vec4(0, -t, 2 * t, -t), glm::vec4(1, 0, t - 3, 2 - t), glm::vec4(0, t, 3 - 2 * t, t - 2), glm::vec4(0, 0, -t, t));
	auto p_x = glm::dot(X,mat * glm::vec4(p0.x, p1.x, p2.x, p3.x));
	auto p_y = glm::dot(X,mat * glm::vec4(p0.y, p1.y, p2.y, p3.y));
	auto p_z = glm::dot(X,mat * glm::vec4(p0.z, p1.z, p2.z, p3.z));
	return glm::vec3(p_x, p_y, p_z);
}
