#pragma once

#include <map>
#include <vector>
#include <string>
#include<glad/glad.h>
#include "glm/glm.hpp"


	struct Shader {
		GLuint id;
		GLuint type;

		std::string filename;
		std::string content;

		Shader() {}
		Shader(std::string file, GLuint t);
		Shader(GLuint t, std::string c);

		void Save();

		std::string Compile();

		void cleanup();
	};

	class Program {
		GLuint programID;
		Shader* vertex, *fragment;
	public:
		Program(Shader* vert, Shader* frag);
		Program() {}
		~Program();

		std::string Link();

		void cleanup();

		void setUniform(std::string name, glm::vec2 *v);
		void setUniform(std::string name, glm::vec3 *v);
		void setUniform(std::string name, glm::mat4 *m);
		void setUniform(std::string name, float f);
		void setUniform(std::string name, int i);
		void setUniform(std::string name, unsigned int i);


		void bind();

		GLuint getProgramID();
	};
