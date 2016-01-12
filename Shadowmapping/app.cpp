#include "app.h"
#include "shader.h"
#include "model.h"

#include <chrono>

App::App() {}

App::~App() {
	logger << "Shutting down";
	SDL_GL_DeleteContext(glctx);
	SDL_Quit();
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

void App::Run() {
	if(SDL_Init(SDL_INIT_EVERYTHING) < 0){
		logger << "SDL init failed";
		throw 0;
	}

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);

	window = SDL_CreateWindow("Shadow mapping", 
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600, SDL_WINDOW_OPENGL);
	if(!window) {
		logger << "Window create failed";
		throw 0;
	}

	glctx = SDL_GL_CreateContext(window);
	if(!glctx) {
		logger << "OpenGL context creation failed";
		throw 0;
	}

	u32 vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
	glDebugMessageCallbackARB(DebugCallback, nullptr);

	glClearColor(0.05,0.05,0.05,0);

	ShaderProgram forwardProgram{};
	ShaderProgram bufferProgram{};
	ShaderProgram deferProgram{};
	{	Shader vsh{"shaders/forward.vs"};
		Shader fsh{"shaders/forward.fs"};

		vsh.Compile();
		fsh.Compile();
		forwardProgram.Attach(vsh);
		forwardProgram.Attach(fsh);
		forwardProgram.Link();
		forwardProgram.Use();
	}
	{	Shader vsh{"shaders/buffer.vs"};
		Shader fsh{"shaders/buffer.fs"};
		Shader defersh{"shaders/deferred.fs"};

		vsh.Compile();
		fsh.Compile();
		bufferProgram.Attach(vsh);
		bufferProgram.Attach(fsh);
		bufferProgram.Link();

		defersh.Compile();
		deferProgram.Attach(vsh);
		deferProgram.Attach(defersh);
		deferProgram.Link();
	}

	u32 forwardFBO, lightFBO;
	u32 fbTargets[4] {0};
	u32 fbTargetTypes[4] {GL_RGBA8, GL_RGBA32F, GL_RGBA8, GL_DEPTH_COMPONENT24};
	u32 fbTargetFormats[4] {GL_RGBA, GL_RGB, GL_RGB, GL_DEPTH_COMPONENT};
	u32 fbTargetAttach[4] {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_DEPTH_ATTACHMENT};

	glGenFramebuffers(1, &forwardFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, forwardFBO);
	glGenTextures(4, fbTargets);

	for(u32 i = 0; i < 4; i++) {
		glBindTexture(GL_TEXTURE_2D, fbTargets[i]);
		glTexImage2D(GL_TEXTURE_2D, 0, fbTargetTypes[i], 800, 600, 0, fbTargetFormats[i], GL_UNSIGNED_BYTE, nullptr);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		glFramebufferTexture2D(GL_FRAMEBUFFER, fbTargetAttach[i], GL_TEXTURE_2D, fbTargets[i], 0);
	}

	u32 lightDepthBuffer;
	glGenFramebuffers(1, &lightFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, lightFBO);

	glGenTextures(1, &lightDepthBuffer);
	glBindTexture(GL_TEXTURE_2D, lightDepthBuffer);
	constexpr u32 shadowmapSize = 4096;
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, shadowmapSize, shadowmapSize, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, nullptr);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LESS);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, lightDepthBuffer, 0);

	glBindTexture(GL_TEXTURE_2D, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	mat4 projectionMatrix = glm::perspective((f32)PI/4.f, 4.f/3.f, 0.001f, 1000.0f);
	mat4 viewMatrix;
	mat4 modelMatrix = mat4(1.f);

	constexpr f32 lightSize = 25.f;
	mat4 lightProjMatrix = glm::ortho(-lightSize, lightSize, -lightSize, lightSize, 0.1f, 100.0f);
	mat4 lightViewMatrix = 
		glm::translate(0.f, 0.f, -30.f) *
		glm::rotate<f32>(PI/4.f, 1, 0, 0) *
		glm::rotate<f32>(PI/4.f, 0, 1, 0)
		;

	glPointSize(4.f);

	u32 triangle, quad;

	{	std::vector<vec3> verts = {
			vec3{0, 1, 0},
			vec3{-0.8f, -0.7f, 0},
			vec3{0.8f, -0.7f, 0},
		};

		glGenBuffers(1, &triangle);
		glBindBuffer(GL_ARRAY_BUFFER, triangle);
		glBufferData(GL_ARRAY_BUFFER, verts.size()*sizeof(vec3), verts.data(), GL_STATIC_DRAW);
	}
	{	std::vector<vec2> verts = {
			vec2{-1, 1},
			vec2{ 1,-1},
			vec2{ 1, 1},
			
			vec2{-1, 1},
			vec2{-1,-1},
			vec2{ 1,-1},
		};

		glGenBuffers(1, &quad);
		glBindBuffer(GL_ARRAY_BUFFER, quad);
		glBufferData(GL_ARRAY_BUFFER, verts.size()*sizeof(vec2), verts.data(), GL_STATIC_DRAW);
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	Model model;
	model.Load("models/scene.obj");

	glEnableVertexAttribArray(forwardProgram.GetAttribute("vertex"));
	glEnableVertexAttribArray(forwardProgram.GetAttribute("normal"));

	u32 displayBuffer = 0;
	vec2 cameraRot {0,0};
	vec3 cameraPos {0,1.f,4.f};
	f32 dt = 0.001f;
	f32 t = 0.f;

	s8 keys[5] = {false};
	bool renderFromLight = false;

	using std::chrono::duration;
	using std::chrono::duration_cast;
	using std::chrono::high_resolution_clock;

	auto begin = high_resolution_clock::now();

	while(running) {
		SDL_Event e;
		while(SDL_PollEvent(&e)) {
			switch (e.type) {
				case SDL_QUIT: running = false; break;
				case SDL_KEYDOWN: {
					switch(e.key.keysym.sym) {
					case SDLK_ESCAPE: running = false; break;
					case SDLK_1: displayBuffer = 0; break;
					case SDLK_2: displayBuffer = 1; break;
					case SDLK_3: displayBuffer = 2; break;
					case SDLK_4: displayBuffer = 3; break;
					case SDLK_5: displayBuffer = 4; break;

					case SDLK_w: keys[0] = true; break;
					case SDLK_s: keys[1] = true; break;
					case SDLK_a: keys[2] = true; break;
					case SDLK_d: keys[3] = true; break;
					case SDLK_LSHIFT: keys[4] = true; break;

					case SDLK_l: renderFromLight ^= true; break;
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
				default: break;
			}
		}

		{
			s32 mx, my;
			SDL_GetMouseState(&mx, &my);
			
			f32 mdx = mx / 800.f * 2.f - 1.f;
			f32 mdy = my / 600.f * 2.f - 1.f;
			
			SDL_WarpMouseInWindow(window, 400, 300);

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

			if(!renderFromLight)
				viewMatrix = glm::translate(rotMatrix, -cameraPos);
		}
		
		lightViewMatrix = 
			glm::translate(0.f, 0.f, -25.f) *
			glm::rotate<f32>(PI/4.f + cos(t/8.f) * PI/6.f, 1, 0, 0) *
			glm::rotate<f32>(PI/16.f*t, 0, 1, 0);

		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

		// Forward
		forwardProgram.Use();
		glBindFramebuffer(GL_FRAMEBUFFER, forwardFBO);
		glDrawBuffers(3, fbTargetAttach);

		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

		glUniformMatrix4fv(forwardProgram.GetUniform("model"), 1, GL_FALSE, glm::value_ptr(modelMatrix));
		
		if(renderFromLight) {
			glUniformMatrix4fv(forwardProgram.GetUniform("view"), 1, GL_FALSE, glm::value_ptr(lightViewMatrix));
			glUniformMatrix4fv(forwardProgram.GetUniform("projection"), 1, GL_FALSE, glm::value_ptr(lightProjMatrix));
		}else{
			glUniformMatrix4fv(forwardProgram.GetUniform("view"), 1, GL_FALSE, glm::value_ptr(viewMatrix));
			glUniformMatrix4fv(forwardProgram.GetUniform("projection"), 1, GL_FALSE, glm::value_ptr(projectionMatrix));			
		}

		model.Draw(forwardProgram);

		glBindFramebuffer(GL_FRAMEBUFFER, lightFBO);
		glViewport(0,0,shadowmapSize,shadowmapSize);

		glClear(GL_DEPTH_BUFFER_BIT);
		glUniformMatrix4fv(forwardProgram.GetUniform("view"), 1, false, glm::value_ptr(lightViewMatrix));
		glUniformMatrix4fv(forwardProgram.GetUniform("projection"), 1, false, glm::value_ptr(lightProjMatrix));
		model.Draw(forwardProgram);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0,0,800,600);

		// Post
		if(!displayBuffer){
			deferProgram.Use();

			glUniformMatrix4fv(deferProgram.GetUniform("lightView"), 1, false, glm::value_ptr(lightViewMatrix));
			glUniformMatrix4fv(deferProgram.GetUniform("lightProj"), 1, false, glm::value_ptr(lightProjMatrix));

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, fbTargets[0]);
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, fbTargets[1]);
			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_2D, fbTargets[2]);
			glActiveTexture(GL_TEXTURE3);
			glBindTexture(GL_TEXTURE_2D, fbTargets[3]);
			glActiveTexture(GL_TEXTURE4);
			glBindTexture(GL_TEXTURE_2D, lightDepthBuffer);

			glUniform1i(deferProgram.GetUniform("colorBuffer"), 0);
			glUniform1i(deferProgram.GetUniform("positionBuffer"), 1);
			glUniform1i(deferProgram.GetUniform("normalBuffer"), 2);
			glUniform1i(deferProgram.GetUniform("depthBuffer"), 3);
			glUniform1i(deferProgram.GetUniform("lightDepth"), 4);
		}else{
			bufferProgram.Use();
			glActiveTexture(GL_TEXTURE0);
			// glBindTexture(GL_TEXTURE_2D, fbTargets[displayBuffer-1]);
			glBindTexture(GL_TEXTURE_2D, lightDepthBuffer);
			glUniform1i(bufferProgram.GetUniform("buffer"), 0);
		}

		glBindBuffer(GL_ARRAY_BUFFER, quad);
		glVertexAttribPointer(deferProgram.GetAttribute("vertex"), 2, GL_FLOAT, false, 0, 0);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glBindTexture(GL_TEXTURE_2D, 0);

		SDL_GL_SwapWindow(window);

		auto end = high_resolution_clock::now();
		dt = duration_cast<duration<f32>>(end-begin).count();
		t += dt;
		begin = end;
	}
}