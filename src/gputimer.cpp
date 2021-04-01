#include "gputimer.h"
#include <fstream>

gputimer::gputimer(std::string n){
		name = n;
		nanoseconds = 0;		
		glGenQueries(1, &timer_query);
		glBeginQuery(GL_TIME_ELAPSED, timer_query);				
	}


gputimer::~gputimer() { 
	//stop();
}
	

double gputimer::stop() 
{
		glEndQuery(GL_TIME_ELAPSED);
		while (!nanoseconds) {
			glGetQueryObjectui64v(timer_query,
				GL_QUERY_RESULT_AVAILABLE,
				&nanoseconds);
		}
		glGetQueryObjectui64v(timer_query, GL_QUERY_RESULT, &nanoseconds);
		auto t = double(nanoseconds) / 1000000.0;
		std::cout << name << " time : " << t << "ms" << std::endl;		
		return t;

		//std::string msg = name + std::string(" time: ") + std::to_string(t) + std::string(" ms\n");
		//return msg;
		//glGetQueryObjectuiv(timer_query, GL_QUERY_RESULT, &nanoseconds);
}

//std::fstream file;
//file.open("test.txt", std::fstream::in | std::fstream::out | std::fstream::app);
//file << "oi oi oi oi oi oi ";
//file.close();