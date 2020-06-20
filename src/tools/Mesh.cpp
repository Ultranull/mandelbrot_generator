#include "Mesh.h"

#include "Buffer.h"



void Mesh::bindmesh() {

	vaObject = VertexArray();
	Buffer *vbuffer = vaObject.bindBuffer<Vertex>("vertexes", GL_ARRAY_BUFFER);
	vbuffer->setData(vertices, GL_STATIC_DRAW);
	vbuffer->bindPointer(0, 3, GL_FLOAT, (void*)offsetof(Vertex, pos));
	vbuffer->bindPointer(1, 2, GL_FLOAT, (void*)offsetof(Vertex, UV));
}

Mesh::Mesh() {
}



Mesh::Mesh(std::vector<Vertex> t_verts) :
	vertices(t_verts){
	bindmesh();
}


void Mesh::renderVertices(GLuint method) {
	vaObject.bind();
	glDrawArrays(method, 0, vertices.size());
	vaObject.unbind();
}

void Mesh::cleanup() {
	vaObject.cleanup();
}
