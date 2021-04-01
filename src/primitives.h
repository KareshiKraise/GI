#pragma once

#include "VAO.h"
#include <GL/glew.h>
#include "GL_CALL.h"

//basic quad
struct quad
{
	GLuint quadVAO;
	GLuint quadVBO;
	vao qvao;

	void renderQuad()
	{
		if (qvao.vao_id == 0)
		{
			float quadVertices[] = {
				// positions        // texture Coords
				-1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
				-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
				1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
				1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
			};

			qvao.init(quadVertices, sizeof(quadVertices));
			qvao.bind();
			qvao.set_vertexarrays(3, 5 * sizeof(float), 0, GL_FLOAT);
			qvao.set_vertexarrays(2, 5 * sizeof(float), 3*(sizeof(float)), GL_FLOAT);
			qvao.unbind();
			//GLCall(glGenVertexArrays(1, &quadVAO));
			//GLCall(glGenBuffers(1, &quadVBO));
			//GLCall(glBindVertexArray(quadVAO));
			//GLCall(glBindBuffer(GL_ARRAY_BUFFER, quadVBO));
			//GLCall(glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW));
			//GLCall(glEnableVertexAttribArray(0));
			//GLCall(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0));
			//GLCall(glEnableVertexAttribArray(1));
			//GLCall(glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float))));
		}

		GLCall(glBindVertexArray(qvao.vao_id));
		GLCall(glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));
		GLCall(glBindVertexArray(0));
	}
};

//basic cube
struct Cube {
	GLuint cubeVAO;
	GLuint cubeVBO;
	vao cvao;

	void renderCube()
	{
		if (cvao.vao_id == 0)
		{
			float cube[] = {
				// positions          
				-1.0f,  1.0f, -1.0f,
				-1.0f, -1.0f, -1.0f,
				1.0f, -1.0f, -1.0f,
				1.0f, -1.0f, -1.0f,
				1.0f,  1.0f, -1.0f,
				-1.0f,  1.0f, -1.0f,

				-1.0f, -1.0f,  1.0f,
				-1.0f, -1.0f, -1.0f,
				-1.0f,  1.0f, -1.0f,
				-1.0f,  1.0f, -1.0f,
				-1.0f,  1.0f,  1.0f,
				-1.0f, -1.0f,  1.0f,

				1.0f, -1.0f, -1.0f,
				1.0f, -1.0f,  1.0f,
				1.0f,  1.0f,  1.0f,
				1.0f,  1.0f,  1.0f,
				1.0f,  1.0f, -1.0f,
				1.0f, -1.0f, -1.0f,

				-1.0f, -1.0f,  1.0f,
				-1.0f,  1.0f,  1.0f,
				1.0f,  1.0f,  1.0f,
				1.0f,  1.0f,  1.0f,
				1.0f, -1.0f,  1.0f,
				-1.0f, -1.0f,  1.0f,

				-1.0f,  1.0f, -1.0f,
				1.0f,  1.0f, -1.0f,
				1.0f,  1.0f,  1.0f,
				1.0f,  1.0f,  1.0f,
				-1.0f,  1.0f,  1.0f,
				-1.0f,  1.0f, -1.0f,

				-1.0f, -1.0f, -1.0f,
				-1.0f, -1.0f,  1.0f,
				1.0f, -1.0f, -1.0f,
				1.0f, -1.0f, -1.0f,
				-1.0f, -1.0f,  1.0f,
				1.0f, -1.0f,  1.0f
			};

			cvao.init(cube, sizeof(cube));
			cvao.bind();
			cvao.set_vertexarrays(3, 3 * sizeof(float), 0, GL_FLOAT);
			cvao.unbind();

			//GLCall(glGenVertexArrays(1, &cubeVAO));
			//GLCall(glGenBuffers(1, &cubeVBO));
			//GLCall(glBindVertexArray(cubeVAO));
			//GLCall(glBindBuffer(GL_ARRAY_BUFFER, cubeVBO));
			//GLCall(glBufferData(GL_ARRAY_BUFFER, sizeof(cube), &cube, GL_STATIC_DRAW));
			//GLCall(glEnableVertexAttribArray(0));
			//GLCall(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0));
		}

		GLCall(glBindVertexArray(cvao.vao_id));
		GLCall(glDrawArrays(GL_TRIANGLES, 0, 36));
		GLCall(glBindVertexArray(0));
	}
};

