#pragma once

#include <vector>

#include <glad/glad.h>
#include "glm/glm.hpp"

#include "VertexArray.h"

struct Vertex {
	glm::vec3 pos       = glm::vec3(0);
	glm::vec2 UV        = glm::vec2(0);
};
class Mesh {

public:
	void bindmesh();
	VertexArray vaObject;
	std::vector<Vertex> vertices;

	Mesh();
	Mesh(std::vector<Vertex>);
	~Mesh() {}

	void renderVertices(GLuint);
	void cleanup();
};

