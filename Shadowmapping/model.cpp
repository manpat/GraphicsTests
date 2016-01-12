#include <fstream>

#include "model.h"
#include "shader.h"

static Log l("Model");

void Model::Load(const string& fname) {	
	std::ifstream file(fname, file.binary);
	if(!file) {
		l << "Model file " << fname << " open failed";
		throw "";
	}

	string filePath = "";
	{
		auto it = fname.find_last_of('/');
		if(it != fname.npos){
			filePath = fname.substr(0, it+1);
		}
	}

	string contents;

	struct Command {
		union {
			struct {
				u32 v[3];
				u32 n;
			};
			vec3* mat;
		};
		u8 type;
	};

	std::map<string, vec3> materials;
	std::vector<vec3> vertices;
	std::vector<vec3> normals;
	std::vector<Command> commands;

	while(file) {
		file >> contents;
		if(contents.size() == 0) continue;

		switch(contents[0]) {
			default:
			case 'o':
			case 's':
			case '#': {
				file.ignore(256, '\n');
			} break;

			case 'm': {
				if(contents == "mtllib") {
					file >> contents;
					materials = ParseMaterials(filePath + contents);
				}
			} break;

			case 'u': {
				if(contents == "usemtl") {
					file >> contents;
					auto it = materials.find(contents);
					if(it != materials.end()) {
						Command cmd;
						cmd.type = 1;
						cmd.mat = &it->second;
						commands.push_back(cmd);
					}
				}
			} break;

			case 'v': {
				vec3 v;
				file >> v.x >> v.y >> v.z;
				if(contents.size() > 1 && contents[1] == 'n') {
					normals.push_back(glm::normalize(v));
				}else{
					vertices.push_back(v);
				}
			} break;

			case 'f': {
				Command f;
				f.type = 0;

				file >> f.v[0];
				file.ignore(2);
				file >> f.n;

				file >> f.v[1];
				file.ignore(128, ' ');
				file >> f.v[2];

				commands.push_back(f);
			} break;
		}
	}

	std::vector<vec3> vBuf;
	std::vector<vec3> nBuf;
	std::vector<u32>  iBuf;
	vBuf.reserve(vertices.size());
	nBuf.reserve(vertices.size());
	iBuf.reserve(vertices.size());

	// <v,n>, i
	std::map<std::pair<u32,u32>, u32> indexCache;

	for(auto& f: commands) {
		switch(f.type) {
			case 0: {
				std::pair<u32,u32> vnPair{0, f.n};

				for(u32 i = 0; i < 3; i++) {
					vnPair.first = f.v[i];

					auto it = indexCache.find(vnPair);
					if(it == indexCache.end()) {
						u32 idx = vBuf.size();
						iBuf.push_back(idx);
						vBuf.push_back(vertices[vnPair.first-1]);
						nBuf.push_back(normals[f.n-1]);
						indexCache[vnPair] = idx;
					}else{
						iBuf.push_back(it->second);
					}
				}
			} break;
			
			case 1: {
				if(subModels.size() > 0) {
					subModels.back().count = iBuf.size() - subModels.back().begin;
				}
				subModels.push_back({(u32)iBuf.size(), 0, *f.mat});
			}
		}
	}

	subModels.back().count = iBuf.size() - subModels.back().begin;

	numIndices = iBuf.size();
	glGenBuffers(1, &vbo);
	glGenBuffers(1, &nbo);
	glGenBuffers(1, &ibo);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, vBuf.size() * sizeof(vec3), vBuf.data(), GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, nbo);
	glBufferData(GL_ARRAY_BUFFER, nBuf.size() * sizeof(vec3), nBuf.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, iBuf.size() * sizeof(u32), iBuf.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

std::map<string, vec3> Model::ParseMaterials(const string& matlib) {
	std::map<string, vec3> ret;

	std::ifstream file(matlib, file.binary);
	if(!file) {
		l << "Material file " << matlib << " open failed";
		throw "";
	}

	string contents;
	string mtlName = "";

	while(file) {
		file >> contents;
		if(contents.size() == 0) continue;

		switch(contents[0]) {
			default: file.ignore(256, '\n'); break;

			case 'n': if(contents == "newmtl") {
				file >> mtlName;
			} break;

			case 'K': if(contents[1] == 'd') {
				vec3 c;
				file >> c.r;
				file >> c.g;
				file >> c.b;
				ret[mtlName] = c;
			}
		}
	}

	return ret;
}


void Model::Draw(ShaderProgram& program) {
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glVertexAttribPointer(program.GetAttribute("vertex"), 3, GL_FLOAT, false, 0, 0);

	glBindBuffer(GL_ARRAY_BUFFER, nbo);
	glVertexAttribPointer(program.GetAttribute("normal"), 3, GL_FLOAT, false, 0, 0);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
	// glDrawElements(GL_TRIANGLES, numIndices, GL_UNSIGNED_INT, nullptr);

	for(auto& sm: subModels) {
		glUniform3fv(program.GetUniform("color"), 1, &sm.color.r);
		glDrawElements(GL_TRIANGLES, sm.count, GL_UNSIGNED_INT, (void*)(sm.begin*sizeof(u32)));
	}
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

