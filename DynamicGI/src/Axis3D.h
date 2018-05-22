#pragma once

//Draw 3D Axis for debug and orientation purpuses
#include <GL/glew.h>
#include "GL_CALL.h"
#include "shader.h"

 struct Axis3D {
	GLuint VAO;
	GLuint VBO;	

	void renderAxis() {

		if (0 == VAO) {

			float axis_vert[] = {
				//pos				//color
				0.0f, 0.0f, 0.0f,   1.0f, 0.0f, 0.0f,
				1.0f, 0.0f, 0.0f,   1.0f, 0.0f, 0.0f,

				0.0f, 0.0f, 0.0f,   0.0f, 1.0f, 0.0f,
				0.0f, 1.0f, 0.0f,   0.0f, 1.0f, 0.0f,

				0.0f, 0.0f, 0.0f,   0.0f, 0.0f, 1.0f,
				0.0f, 0.0f, 1.0f,   0.0f, 0.0f, 1.0f,
			};

			GLCall(glGenVertexArrays(1, &VAO));
			GLCall(glGenBuffers(1, &VBO));
			GLCall(glBindVertexArray(VAO));
			GLCall(glBindBuffer(GL_ARRAY_BUFFER, VBO));
			GLCall(glBufferData(GL_ARRAY_BUFFER, sizeof(axis_vert), &axis_vert, GL_STATIC_DRAW));
			GLCall(glEnableVertexAttribArray(0));
			GLCall(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0));
			GLCall(glEnableVertexAttribArray(1));
			GLCall(glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float))));
		}
		
		GLCall(glBindVertexArray(VAO));
		GLCall(glDrawArrays(GL_LINES, 0, 6));
		GLCall(glBindVertexArray(0));	
		

	};
	


};

