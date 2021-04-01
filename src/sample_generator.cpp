#include "sample_generator.h"

namespace sample_generator {
	const float PI = 3.1415926f;
	const float PI_TWO = 6.2831853;

	std::vector<glm::vec2> gen_uniform_samples2d(unsigned int s, float min, float max) {
		std::uniform_real_distribution<float> randomFloats(min, max);		
		std::mt19937 gen;
		std::vector<glm::vec2> samples(s);

		for (int i = 0; i < s; i++) {
			glm::vec2 val{ 0,0 };
			val.x = randomFloats(gen);
			val.y = randomFloats(gen);
			samples[i] = val;
		}
		return samples;
	}

	std::vector<glm::vec2> gen_normal_samples2d(unsigned int s, float min, float max) {
		std::normal_distribution<float> randomFloats(min, max);
		std::mt19937 gen;
		std::vector<glm::vec2> samples(s);

		for (int i = 0; i < s; i++) {
			glm::vec2 val{ 0,0 };
			val.x = randomFloats(gen);
			val.y = randomFloats(gen);
			samples[i] = val;
		}
		return samples;
	}

	float radicalInverse_VdC(unsigned int bits) {
		bits = (bits << 16u) | (bits >> 16u);
		bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
		bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
		bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
		bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
		return float(bits) * 2.3283064365386963e-10; // / 0x100000000
	}

	glm::vec2 hammersley2d(unsigned int i, unsigned int N) {
		return glm::vec2(float(i) / float(N), radicalInverse_VdC(i));
	}

	std::vector<glm::vec2> gen_hammersley2d(unsigned int N) {
		std::vector<glm::vec2> samples(N);
		for (int i = 0; i < N; i++)
		{
			samples[i] = hammersley2d(i, N);
		}
		return samples;
	}

	std::vector<glm::vec2> gen_uniform_circle(unsigned int s, float min, float max) {	
		auto uniform = gen_uniform_samples2d(s, min, max);		
		for (int i = 0; i < s; i++) {
			glm::vec2 theta = PI_TWO * uniform[i];
			glm::vec2 r = sqrt(uniform[i]);
			float x = r.y * cos(theta.x);
			float y = r.y * sin(theta.x);
			uniform[i] = (glm::vec2(x, y) * 0.5f) + glm::vec2(0.5f, 0.5f);			
		}
		return uniform;
	}
}