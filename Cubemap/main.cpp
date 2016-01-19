#include "common.h"
#include "app.h"

#include <glm/gtx/string_cast.hpp>

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


s32 main() {
	try{
		App app{};
		app.Run();
	}catch(...){
		Log("Exception") << "Something happened";
	}

	return 0;
}