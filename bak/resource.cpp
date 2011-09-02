#include "resource.hpp"

namespace resource {

int num_resources() { return 4; }

char resource_id(int n) {
	static const char* const ResourceStr = "rgbw";
	return ResourceStr[n];
}
}
