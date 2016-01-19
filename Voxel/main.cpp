#include "voxelchunk.h"
#include "shader.h"
#include "common.h"

#include <SDL2/SDL.h>
#include <glm/gtx/string_cast.hpp>

#include <chrono>

std::ostream& operator<<(std::ostream& o, const vec2& v) {
	return o << glm::to_string(v);
}
std::ostream& operator<<(std::ostream& o, const vec3& v) {
	return o << glm::to_string(v);
}
std::ostream& operator<<(std::ostream& o, const vec4& v) {
	return o << glm::to_string(v);
}

std::ostream& operator<<(std::ostream& o, const mat3& v) {
	return o << glm::to_string(v);
}
std::ostream& operator<<(std::ostream& o, const mat4& v) {
	return o << glm::to_string(v);
}

void APIENTRY DebugCallback(u32 source, u32 type, u32 id, u32 severity, s32 length, const char* msg, void*) {
	if(type != GL_DEBUG_TYPE_ERROR_ARB && type != GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB) return;

	static Log logger{"GLDebug"};
	logger << msg;

	(void) source;
	(void) id;
	(void) severity;
	(void) length;
}

// Run this once per frame before drawing all the meshes.
// You still need to separately set the 'transform' uniform for every mesh.
void setup_uniforms(ShaderProgram& shader/*, float camera_pos[4], GLuint tex1, GLuint tex2*/) {
	shader.Use();
	for(s32 i=0; i < STBVOX_UNIFORM_count; ++i) {
		stbvox_uniform_info sui;
		if (stbvox_get_uniform_info(&sui, i)) {
			GLint loc = shader.GetUniform(sui.name);
			if (loc != 0) {
				switch (i) {
					case STBVOX_UNIFORM_ambient: {
						vec4 data[4] {vec4{0.f}};
						data[0] = glm::normalize(vec4(1,0.5,1,1));
						data[1] = vec4(0.5f);
						data[2] = vec4(1, 0.9, 0.9, 1);

						glUniform4fv(loc, sui.array_length, &data[0].x);
					}	break;

					case STBVOX_UNIFORM_normals:     // you never want to override this
					case STBVOX_UNIFORM_texgen:      // you never want to override this
						glUniform3fv(loc, sui.array_length, sui.default_value);
						break;

					default: break;
				}
			}
		}
	}
}

static Log logger{"Main"};

s32 main() {
	constexpr u32 wwidth = 800;
	constexpr u32 wheight = 600;

	if(SDL_Init(SDL_INIT_EVERYTHING) < 0){
		logger << "SDL init failed";
		return 1;
	}

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);
	
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);

	auto window = SDL_CreateWindow("Voxel", 
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, wwidth, wheight, SDL_WINDOW_OPENGL);

	if(!window) {
		logger << "Window creation failed";
		return 1;
	}

	auto glctx = SDL_GL_CreateContext(window);
	if(!glctx) {
		logger << "OpenGL context creation failed";
		throw 0;
	}

	u32 vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glEnable(GL_MULTISAMPLE);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
	glDebugMessageCallbackARB(DebugCallback, nullptr);

	glClearColor(0.05,0.05,0.05,0);

	ShaderProgram program;
	{	Shader vsh{"shaders/voxel.vs"};
		Shader fsh{"shaders/voxel.fs"};

		vsh.Compile();
		fsh.Compile();
		program.Attach(vsh);
		program.Attach(fsh);
		program.Link();
	}

	mat4 projectionMatrix = glm::perspective((f32)PI/4.f, 4.f/3.f, 0.001f, 1000.0f);
	mat4 viewMatrix = mat4(1.f);
	mat4 modelMatrix = glm::translate<f32>(-0.2f,-0.2f,-1.f);

	VoxelChunk chunk{32,32,24};
	chunk.modelMatrix = modelMatrix;

	for(u32 x = 0; x < chunk.width; x++)
	for(u32 y = 0; y < chunk.height; y++) {
		chunk.SetBlock(x,y,0, rand()%2 + 1);
		chunk.SetColor(x,y,0, 100,200,100);
	}

	for(u32 x = 0; x < chunk.width; x++)
	for(u32 y = 0; y < chunk.height; y++) {
		chunk.SetBlock(x,y,1, (chunk.GetBlock(x,y,0)==1 && rand()%5==0)?7:0);
		chunk.SetColor(x,y,1, 127,255,127);
	}
	
	for(u32 x = 0; x < chunk.width; x++) {
		chunk.SetBlock(x,0,4, 1);
		chunk.SetColor(x,0,4, 255,0,0);
	}
	for(u32 y = 0; y < chunk.height; y++) {
		chunk.SetBlock(0,y,5, 1);
		chunk.SetColor(0,y,5, 255,0,0);
	}

	chunk.GenerateMesh();
	logger << "Num quads generated: " << chunk.numQuads;

	glEnableVertexAttribArray(0);

	vec2 cameraRot {0,0};
	vec3 cameraPos {0,0,0.f};
	s8 keys[5] = {false};
	f32 dt = 0.001f;
	f32 t = 0.f;

	using std::chrono::duration;
	using std::chrono::duration_cast;
	using std::chrono::high_resolution_clock;

	auto begin = high_resolution_clock::now();

	bool running = true;
	while(running){
		SDL_Event e;
		while(SDL_PollEvent(&e)){
			if(e.type == SDL_QUIT) running = false;
			switch(e.type){
				case SDL_QUIT: running = false; break;
				case SDL_KEYDOWN: {
					auto key = e.key.keysym.sym;
					switch(key) {
					case SDLK_ESCAPE: running = false; break;
					case SDLK_w: keys[0] = true; break;
					case SDLK_s: keys[1] = true; break;
					case SDLK_a: keys[2] = true; break;
					case SDLK_d: keys[3] = true; break;
					case SDLK_LSHIFT: keys[4] = true; break;
					}
				} break;
				case SDL_KEYUP: {
					switch(e.key.keysym.sym) {
					case SDLK_w: keys[0] = false; break;
					case SDLK_s: keys[1] = false; break;
					case SDLK_a: keys[2] = false; break;
					case SDLK_d: keys[3] = false; break;
					case SDLK_LSHIFT: keys[4] = false; break;
					}
				} break;

				case SDL_MOUSEBUTTONDOWN: {
					vec4 hcpos = vec4{cameraPos, 1.f};
					hcpos = glm::inverse(chunk.modelMatrix * glm::rotate<f32>(-PI/2.f, vec3{1,0,0})) * hcpos - vec4{1.f};
					logger << hcpos;

					chunk.SetBlock((u32)hcpos.x, (u32)hcpos.y, (u32)hcpos.z, 1);
					chunk.SetColor((u32)hcpos.x, (u32)hcpos.y, (u32)hcpos.z, 0, 0, 255);
				} break;
			}
		}

		{	s32 mx, my;
			SDL_GetMouseState(&mx, &my);
			
			f32 mdx = mx / (f32)wwidth * 2.f - 1.f;
			f32 mdy = my / (f32)wheight * 2.f - 1.f;
			
			SDL_WarpMouseInWindow(window, wwidth/2, wheight/2);

			auto nyaw = mdx * 2.0 * PI * dt * 7.f;
			auto npitch = mdy * 2.0 * PI * dt * 7.f;
			constexpr f32 limit = PI/2.f;

			cameraRot.x = clamp(cameraRot.x + npitch, -limit, limit);
			cameraRot.y += nyaw;

			auto rotMatrix = mat4(1.f);
			rotMatrix = glm::rotate(rotMatrix, cameraRot.x, vec3{1,0,0});
			rotMatrix = glm::rotate(rotMatrix, cameraRot.y, vec3{0,1,0});

			vec4 inputDir {
				keys[3] - keys[2], 0, keys[1] - keys[0], 0
			};

			inputDir = inputDir * rotMatrix;
			cameraPos += vec3(inputDir) * dt * 6.f * (1.f + keys[4]*3.f);

			viewMatrix = glm::translate(rotMatrix, -cameraPos);
		}

		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

		chunk.modelMatrix = glm::translate<f32>(-(chunk.width/2.f), -8.f, (chunk.height/2.f));

		setup_uniforms(program);
		glUniformMatrix4fv(program.GetUniform("view_projection"), 1, false, 
			glm::value_ptr(projectionMatrix * viewMatrix));

		chunk.Render(program);

		SDL_GL_SwapWindow(window);
		SDL_Delay(1);

		auto end = high_resolution_clock::now();
		dt = duration_cast<duration<f32>>(end-begin).count();
		t += dt;
		begin = end;

		string fps = "FPS: " + std::to_string(1.f/dt) + " NumTris: " + std::to_string(chunk.numQuads*2);
		SDL_SetWindowTitle(window, fps.data());
	}

	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}