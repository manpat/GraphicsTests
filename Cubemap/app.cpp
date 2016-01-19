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

	constexpr u32 WindowWidth = 800;
	constexpr u32 WindowHeight = 600;

	window = SDL_CreateWindow("Cube mapping", 
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WindowWidth, WindowHeight, SDL_WINDOW_OPENGL);
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
	// glEnable(GL_CULL_FACE);
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
	glDebugMessageCallbackARB(DebugCallback, nullptr);

	glClearColor(0.05,0.05,0.05,0);

	ShaderProgram forwardProgram{};
	ShaderProgram bufferProgram{};
	ShaderProgram deferProgram{};
	ShaderProgram reflectProgram{};
	ShaderProgram cubeProgram{};
	{	Shader vsh{"shaders/forward.vs"};
		Shader fsh{"shaders/forward.fs"};
		Shader reflsh{"shaders/reflect.fs"};

		vsh.Compile();
		fsh.Compile();
		forwardProgram.Attach(vsh);
		forwardProgram.Attach(fsh);
		forwardProgram.Link();
		forwardProgram.Use();

		reflsh.Compile();
		reflectProgram.Attach(vsh);
		reflectProgram.Attach(reflsh);
		reflectProgram.Link();
	}
	{	Shader vsh{"shaders/buffer.vs"};
		Shader fsh{"shaders/buffer.fs"};
		Shader defersh{"shaders/deferred.fs"};
		Shader cubesh{"shaders/cube.fs"};

		vsh.Compile();
		fsh.Compile();
		bufferProgram.Attach(vsh);
		bufferProgram.Attach(fsh);
		bufferProgram.Link();

		defersh.Compile();
		deferProgram.Attach(vsh);
		deferProgram.Attach(defersh);
		deferProgram.Link();

		cubesh.Compile();
		cubeProgram.Attach(vsh);
		cubeProgram.Attach(cubesh);
		cubeProgram.Link();
	}

	u32 forwardFBO, cubeFBO;
	u32 fbTargets[4] {0};
	u32 fbTargetTypes[4] {GL_RGBA8, GL_RGBA32F, GL_RGBA8, GL_DEPTH_COMPONENT24};
	u32 fbTargetFormats[4] {GL_RGBA, GL_RGB, GL_RGB, GL_DEPTH_COMPONENT};
	u32 fbTargetAttach[4] {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_DEPTH_ATTACHMENT};

	glGenFramebuffers(1, &forwardFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, forwardFBO);
	glDrawBuffers(3, fbTargetAttach);
	glGenTextures(4, fbTargets);

	for(u32 i = 0; i < 4; i++) {
		glBindTexture(GL_TEXTURE_2D, fbTargets[i]);
		glTexImage2D(GL_TEXTURE_2D, 0, fbTargetTypes[i], WindowWidth, WindowHeight, 0, fbTargetFormats[i], GL_UNSIGNED_BYTE, nullptr);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		glFramebufferTexture2D(GL_FRAMEBUFFER, fbTargetAttach[i], GL_TEXTURE_2D, fbTargets[i], 0);
	}
	glBindTexture(GL_TEXTURE_2D, 0);

	constexpr u32 cubemapSize = 1024;
	u32 cubeMap;

	glGenFramebuffers(1, &cubeFBO);
	glGenTextures(1, &cubeMap);
	glBindFramebuffer(GL_FRAMEBUFFER, cubeFBO);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMap);

	glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	std::vector<u8> testData(cubemapSize * cubemapSize * 4, 255);

	for (u32 face = 0; face < 6; face++) {
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, GL_RGBA,
			cubemapSize, cubemapSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, &testData[0]);
	}

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X, cubeMap, 0);

	{	u32 depthbuffer;
		glGenRenderbuffers(1, &depthbuffer);
		glBindRenderbuffer(GL_RENDERBUFFER, depthbuffer);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, cubemapSize, cubemapSize);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthbuffer);
	}

	glDrawBuffer(GL_COLOR_ATTACHMENT0);

	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	mat4 projectionMatrix = glm::perspective((f32)PI/4.f, 4.f/3.f, 0.001f, 1000.0f);
	mat4 viewMatrix;
	mat4 modelMatrix = mat4(1.f);

	glPointSize(4.f);

	u32 quad;
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

	Model sphere;
	sphere.Load("models/sphere.obj");

	glEnableVertexAttribArray(forwardProgram.GetAttribute("vertex"));
	glEnableVertexAttribArray(forwardProgram.GetAttribute("normal"));

	u32 displayBuffer = 0;
	vec2 cameraRot {0,0};
	vec3 cameraPos {0,1.f,4.f};
	f32 dt = 0.001f;
	f32 t = 0.f;

	s8 keys[5] = {false};

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
					case SDLK_6: displayBuffer = 5; break;
					case SDLK_7: displayBuffer = 6; break;
					case SDLK_8: displayBuffer = 7; break;
					case SDLK_9: displayBuffer = 8; break;
					case SDLK_0: displayBuffer = 9; break;

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
				default: break;
			}
		}

		{
			s32 mx, my;
			SDL_GetMouseState(&mx, &my);
			
			f32 mdx = mx / (f32)WindowWidth * 2.f - 1.f;
			f32 mdy = my / (f32)WindowHeight * 2.f - 1.f;
			
			SDL_WarpMouseInWindow(window, WindowWidth/2, WindowHeight/2);

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

		// Forward
		forwardProgram.Use();
		glBindFramebuffer(GL_FRAMEBUFFER, forwardFBO);

		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

		glUniformMatrix4fv(forwardProgram.GetUniform("model"), 1, GL_FALSE, glm::value_ptr(modelMatrix));
		glUniformMatrix4fv(forwardProgram.GetUniform("view"), 1, GL_FALSE, glm::value_ptr(viewMatrix));
		glUniformMatrix4fv(forwardProgram.GetUniform("projection"), 1, GL_FALSE, glm::value_ptr(projectionMatrix));

		model.Draw(forwardProgram);

		// Cubemap
		glBindFramebuffer(GL_FRAMEBUFFER, cubeFBO);
		glViewport(0,0,cubemapSize,cubemapSize);

		mat4 cubeProjection = glm::perspective<f32>(PI/2.f, 1.f, 0.1f, 1000.0f);
		static mat4 cubeViews[6] {
			// +x, -x, +y, -y, +z, -z
			glm::rotate<f32>(PI/2.f, vec3{0,1,0}),
			glm::rotate<f32>(-PI/2.f, vec3{0,1,0}),

			glm::rotate<f32>(PI/2.f, vec3{1,0,0}),
			glm::rotate<f32>(-PI/2.f, vec3{1,0,0}),

			glm::rotate<f32>(PI, vec3{0,1,0}),
			mat4{1.f},
		};
		mat4 cubeTrans = glm::translate<f32>(0, 5 * sin(t*0.01f), 0) * glm::scale<f32>(1,-1,1);

		for(u32 i = 0; i < 6; i++) {
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, cubeMap, 0);
			glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

			glUniformMatrix4fv(forwardProgram.GetUniform("view"), 1, false, glm::value_ptr(cubeViews[i] * cubeTrans));
			glUniformMatrix4fv(forwardProgram.GetUniform("projection"), 1, false, glm::value_ptr(cubeProjection));
			model.Draw(forwardProgram);
		}

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0,0,WindowWidth,WindowHeight);

		// Post
		if(!displayBuffer){
			deferProgram.Use();

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, fbTargets[0]);
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, fbTargets[1]);
			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_2D, fbTargets[2]);
			glActiveTexture(GL_TEXTURE3);
			glBindTexture(GL_TEXTURE_2D, fbTargets[3]);

			glUniform1i(deferProgram.GetUniform("colorBuffer"), 0);
			glUniform1i(deferProgram.GetUniform("positionBuffer"), 1);
			glUniform1i(deferProgram.GetUniform("normalBuffer"), 2);
			glUniform1i(deferProgram.GetUniform("depthBuffer"), 3);
		}else{
			glActiveTexture(GL_TEXTURE0);
			// if(displayBuffer > 2){
				cubeProgram.Use();
				glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMap);
				glUniform1i(cubeProgram.GetUniform("cubeMap"), 0);
				glUniform1i(cubeProgram.GetUniform("face"), displayBuffer-1);
			// }else{
			// 	bufferProgram.Use();
			// 	glBindTexture(GL_TEXTURE_2D, fbTargets[displayBuffer-1]);	
			// 	glUniform1i(bufferProgram.GetUniform("buffer"), 0);
			// }
		}

		glBindBuffer(GL_ARRAY_BUFFER, quad);
		glVertexAttribPointer(deferProgram.GetAttribute("vertex"), 2, GL_FLOAT, false, 0, 0);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glBindTexture(GL_TEXTURE_2D, 0);

		reflectProgram.Use();
		glClear(GL_DEPTH_BUFFER_BIT);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMap);

		glUniform1i(reflectProgram.GetUniform("cubeMap"), 0);
		glUniform3fv(reflectProgram.GetUniform("cameraPos"), 1, glm::value_ptr(cameraPos));

		// mat4 mirrorMat = glm::rotate(glm::rotate(t, vec3{0,1,0}), t*2.f, vec3{1,0,0});
		glUniformMatrix4fv(reflectProgram.GetUniform("model"), 1, GL_FALSE, glm::value_ptr(cubeTrans));
		glUniformMatrix4fv(reflectProgram.GetUniform("view"), 1, GL_FALSE, glm::value_ptr(viewMatrix));
		glUniformMatrix4fv(reflectProgram.GetUniform("projection"), 1, GL_FALSE, glm::value_ptr(projectionMatrix));
		
		sphere.Draw(reflectProgram);

		glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

		SDL_GL_SwapWindow(window);

		auto end = high_resolution_clock::now();
		dt = duration_cast<duration<f32>>(end-begin).count();
		t += dt;
		begin = end;
	}
}