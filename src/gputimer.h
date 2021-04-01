#pragma once
#include <GL/glew.h>
#include <string>
#include <iostream>

struct gputimer {
	gputimer(std::string n);
	~gputimer();
	double stop();
	GLuint timer_query;
	GLuint64 nanoseconds;
	std::string name;	
};
