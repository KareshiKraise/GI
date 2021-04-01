#pragma once
#include <vector>
#include <random>
#include <glm/glm.hpp>

namespace sample_generator {

	std::vector<glm::vec2> gen_uniform_samples2d(unsigned int s, float min, float max);

	std::vector<glm::vec2> gen_normal_samples2d(unsigned int s, float min, float max);
	
	std::vector<glm::vec2> gen_uniform_circle(unsigned int s, float min, float max);

	std::vector<glm::vec2> gen_hammersley2d(unsigned int N);

}
