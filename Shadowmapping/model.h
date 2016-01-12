#ifndef MODEL_H
#define MODEL_H

#include "common.h"

struct ShaderProgram;

struct Model {
	struct SubModel {
		u32 begin;
		u32 count;

		vec3 color;
	};

	std::vector<SubModel> subModels;
	u32 vbo = 0;
	u32 nbo = 0;
	u32 ibo = 0;
	u32 numIndices = 0;

	void Load(const string&);
	void Draw(ShaderProgram&);

private:
	std::map<string, vec3> ParseMaterials(const string&);
};

#endif