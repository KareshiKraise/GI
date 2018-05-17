#pragma once

#include <GL/glew.h>
#include "GL_CALL.h"

struct quad
{
	GLuint quadVAO;
	GLuint quadVBO;

	void renderQuad()
	{
		if (quadVAO == 0)
		{
			float quadVertices[] = {
				// positions        // texture Coords
				-1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
				-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
				1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
				1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
			};


			GLCall(glGenVertexArrays(1, &quadVAO));
			GLCall(glGenBuffers(1, &quadVBO));
			GLCall(glBindVertexArray(quadVAO));
			GLCall(glBindBuffer(GL_ARRAY_BUFFER, quadVBO));
			GLCall(glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW));
			GLCall(glEnableVertexAttribArray(0));
			GLCall(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0));
			GLCall(glEnableVertexAttribArray(1));
			GLCall(glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float))));
		}

		GLCall(glBindVertexArray(quadVAO));
		GLCall(glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));
		GLCall(glBindVertexArray(0));
	}
};
