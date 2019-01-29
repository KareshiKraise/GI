#ifndef SHADER_H
#define SHADER_H

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

#include <GL/glew.h>

#include "GL_CALL.h"


class Shader
{
public:
	unsigned int ID;
	// constructor generates the shader on the fly
	// ------------------------------------------------------------------------
	Shader(const char* vertexPath, const char* fragmentPath, const char* geometryPath = nullptr, const char* computePath = nullptr)
	{
		// 1. retrieve the vertex/fragment source code from filePath
		std::string vertexCode;
		std::string fragmentCode;
		std::string geometryCode;
		std::string computeCode;

		std::ifstream vShaderFile;
		std::ifstream fShaderFile;
		std::ifstream gShaderFile;
		std::ifstream cShaderFile;

		// ensure ifstream objects can throw exceptions:
		vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		gShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		cShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		try
		{
			
			if (vertexPath != nullptr && fragmentPath != nullptr)
			{// open files
				vShaderFile.open(vertexPath);
				fShaderFile.open(fragmentPath);
				std::stringstream vShaderStream, fShaderStream;
				// read file's buffer contents into streams
				vShaderStream << vShaderFile.rdbuf();
				fShaderStream << fShaderFile.rdbuf();
				// close file handlers
				vShaderFile.close();
				fShaderFile.close();
				// convert stream into string
				vertexCode = vShaderStream.str();
				fragmentCode = fShaderStream.str();
			}
			// if geometry shader path is present, also load a geometry shader
			if (geometryPath != nullptr)
			{
				gShaderFile.open(geometryPath);
				std::stringstream gShaderStream;
				gShaderStream << gShaderFile.rdbuf();
				gShaderFile.close();
				geometryCode = gShaderStream.str();
			}
			// if compute shader path is present, also load a geometry shader
			if (computePath != nullptr) {
				cShaderFile.open(computePath);
				std::stringstream cShaderStream;
				cShaderStream << cShaderFile.rdbuf();
				cShaderFile.close();
				computeCode = cShaderStream.str();
			}
		}
		catch (std::ifstream::failure e)
		{
			std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ" << std::endl;
			std::cout << e.what() << std::endl;
		}

		unsigned int vertex_s, fragment_s;
		if (vertexPath != nullptr && fragmentPath != nullptr)
		{
			const char* vShaderCode = vertexCode.c_str();
			const char * fShaderCode = fragmentCode.c_str();
			//const char * gShaderCode = geometryCode.c_str();
			//const char * cShaderCode = computeCode.c_str();
			// 2. compile shaders			
			int success;
			char infoLog[512];
			// vertex shader
			vertex_s = glCreateShader(GL_VERTEX_SHADER);
			GLCall(glShaderSource(vertex_s, 1, &vShaderCode, NULL));
			GLCall(glCompileShader(vertex_s));
			checkCompileErrors(vertex_s, "VERTEX");
			// fragment Shader
			fragment_s = glCreateShader(GL_FRAGMENT_SHADER);
			GLCall(glShaderSource(fragment_s, 1, &fShaderCode, NULL));
			GLCall(glCompileShader(fragment_s));
			checkCompileErrors(fragment_s, "FRAGMENT");
		}
		// if geometry shader is given, compile geometry shader
		unsigned int geometry_s;
		if (geometryPath != nullptr)
		{
			const char * gShaderCode = geometryCode.c_str();
			geometry_s = glCreateShader(GL_GEOMETRY_SHADER);
			GLCall(glShaderSource(geometry_s, 1, &gShaderCode, NULL));
			GLCall(glCompileShader(geometry_s));
			checkCompileErrors(geometry_s, "GEOMETRY");
		}
		unsigned int compute_s;
		if (computePath != nullptr)
		{
			const char * cShaderCode = computeCode.c_str();
			compute_s = glCreateShader(GL_COMPUTE_SHADER);
			GLCall(glShaderSource(compute_s, 1, &cShaderCode, NULL));
			GLCall(glCompileShader(compute_s));
			checkCompileErrors(compute_s, "COMPUTE");
		}

		// shader Program
		ID = glCreateProgram();

		if (vertexPath != nullptr && fragmentPath != nullptr)
		{
			GLCall(glAttachShader(ID, vertex_s));
			GLCall(glAttachShader(ID, fragment_s));
		}

		if (geometryPath != nullptr)
		{
			std::cout << "has geometry shader will attach" << std::endl;
			glAttachShader(ID, geometry_s);
		}

		if (computePath != nullptr)
		{
			std::cout << "has compute shader will attach" << std::endl;
			glAttachShader(ID, compute_s);
		}

		GLCall(glLinkProgram(ID));
		checkCompileErrors(ID, "PROGRAM");

		// delete the shaders as they're linked into our program now and no longer necessary

		if (vertexPath != nullptr && fragmentPath != nullptr)
		{
			GLCall(glDeleteShader(vertex_s));
			GLCall(glDeleteShader(fragment_s));
		}

		if (geometryPath != nullptr)
		{
			std::cout << " has geometry shader will delete" << std::endl;
			glDeleteShader(geometry_s);
		}
		if (computePath != nullptr)
		{
			std::cout << " has compute shader will delete" << std::endl;
			glDeleteShader(compute_s);
		}


	}
	// activate the shader
	// ------------------------------------------------------------------------
	void use()
	{
		GLCall(glUseProgram(ID));
	}
	// utility uniform functions
	// ------------------------------------------------------------------------
	void setBool(const std::string &name, bool value) const
	{
		GLCall(glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value));
	}
	// ------------------------------------------------------------------------
	void setInt(const std::string &name, int value) const
	{
		GLCall(glUniform1i(glGetUniformLocation(ID, name.c_str()), value));
	}
	// ------------------------------------------------------------------------
	void setFloat(const std::string &name, float value) const
	{
		GLCall(glUniform1f(glGetUniformLocation(ID, name.c_str()), value));
	}
	// ------------------------------------------------------------------------
	void setVec2(const std::string &name, const glm::vec2 &value) const
	{
		GLCall(glUniform2fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]));
	}
	void setVec2(const std::string &name, float x, float y) const
	{
		GLCall(glUniform2f(glGetUniformLocation(ID, name.c_str()), x, y));
	}
	// ------------------------------------------------------------------------
	void setVec3(const std::string &name, const glm::vec3 &value) const
	{
		GLCall(glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]));
	}

	void setVec3(const std::string &name, float x, float y, float z) const
	{
		GLCall(glUniform3f(glGetUniformLocation(ID, name.c_str()), x, y, z));
	}
	// ------------------------------------------------------------------------
	void setVec4(const std::string &name, const glm::vec4 &value) const
	{
		GLCall(glUniform4fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]));
	}
	void setVec4(const std::string &name, float x, float y, float z, float w)
	{
		GLCall(glUniform4f(glGetUniformLocation(ID, name.c_str()), x, y, z, w));
	}
	// ------------------------------------------------------------------------
	void setMat2(const std::string &name, const glm::mat2 &mat) const
	{
		GLCall(glUniformMatrix2fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]));
	}
	// ------------------------------------------------------------------------
	void setMat3(const std::string &name, const glm::mat3 &mat) const
	{
		GLCall(glUniformMatrix3fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]));
	}
	// ------------------------------------------------------------------------
	void setMat4(const std::string &name, const glm::mat4 &mat) const
	{
		GLCall(glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]));
	}

private:
	// utility function for checking shader compilation/linking errors.
	// ------------------------------------------------------------------------
	void checkCompileErrors(GLuint shader, std::string type)
	{
		GLint success;
		GLchar infoLog[1024];
		if (type != "PROGRAM")
		{
			GLCall(glGetShaderiv(shader, GL_COMPILE_STATUS, &success));
			if (!success)
			{
				GLCall(glGetShaderInfoLog(shader, 1024, NULL, infoLog));
				std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
			}
		}
		else
		{
			GLCall(glGetProgramiv(shader, GL_LINK_STATUS, &success));
			if (!success)
			{
				GLCall(glGetProgramInfoLog(shader, 1024, NULL, infoLog));
				std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
			}
		}
	}
};
#endif

