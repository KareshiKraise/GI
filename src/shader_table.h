#pragma once

#include "program.h"
#include <unordered_map>
#include <string>
#include <fstream>
#include <vector>
#include <algorithm>

struct shader_table {	
	//text file modeled as *pass* = *path*
	shader_table() {

	}

	shader_table(std::string path){
		init_from_file(path);
	}

	void init_from_file(std::string path) {
		std::ifstream file(path);

		std::string str;
		while (std::getline(file, str))
		{
			std::string pass = str.substr(0, str.find_first_of("="));
			std::string path = str.substr(str.find_first_of("=") + 1, std::string::npos);
			auto vec = split(path, ",");
			program shader;

			std::for_each(vec.begin(), vec.end(), [](std::string& entry) {entry.insert(0, "shaders/"); });
			
			if (vec.size() == 1)
				shader.init(nullptr, nullptr, nullptr , vec[0].c_str());

			if (vec.size() == 2)
				shader.init(vec[0].c_str(), vec[1].c_str());

			else if (vec.size() == 3)
			{					
				auto has_geometry = vec[2].find("gshader");
				auto has_compute = vec[2].find("cshader");
				if (has_geometry != std::string::npos)
				{
					//std::cout << "has geoemtry" << std::endl;
					shader.init(vec[0].c_str(), vec[1].c_str(), vec[2].c_str(), nullptr);
				}
				else if (has_compute != std::string::npos) {
					//std::cout << "has compute" << std::endl;
					shader.init(vec[0].c_str(), vec[1].c_str(), nullptr, vec[2].c_str());
				}
			}
			else if (vec.size() == 4) {
				shader.init(vec[0].c_str(), vec[1].c_str(), vec[2].c_str(), vec[3].c_str());
			}
			table[pass] = shader;
		}
		file.close();
	}

	void add_shader(std::string pass, program shader) {
		table[pass] = shader;
	}

	std::vector<std::string> split(std::string msg, std::string separator) {
		std::vector<std::string> list;
		auto pos = 0;
		while ((pos = msg.find(separator)) != std::string::npos)
		{
			list.emplace_back(msg.substr(0, pos));
			msg.erase(0, pos + separator.length());
		}
		list.emplace_back(msg.substr(0, pos));
		return list;
	}


	std::unordered_map<std::string, program> table;
};
