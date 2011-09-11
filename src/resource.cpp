#include "asserts.hpp"
#include "resource.hpp"

namespace resource {

namespace {
	static const char* const ResourceStr = "gfbshz";
}

int num_resources() { return 6; }

char resource_id(int n) {
	return ResourceStr[n];
}

const char* resource_name(int n)
{
	const char* ResourceNames[] = {
		"gold",
		"food",
		"blood",
		"scrolls",
		"honor",
		"z",
	};

	return ResourceNames[n];
}

const char* resource_icon(int n)
{
	const char* ResourceIcons[] = {
		"magic-icon-g",
		"magic-icon-f",
		"magic-icon-b",
		"magic-icon-s",
		"magic-icon-h",
		"magic-icon-z",
	};

	return ResourceIcons[n];
}

int resource_index(char c) {
	for(int n = 0; ResourceStr[n]; ++n) {
		if(ResourceStr[n] == c) {
			return n;
		}
	}

	ASSERT_LOG(false, "BAD RESOURCE ID: " << c);
}

const int* resource_color(int n) {
	static const int Colors[] = {
		255, 255, 255,
		128, 128, 128,
		0, 196, 0,
		255, 0, 0,
		0, 0, 255,
		196, 128, 0,
	};

	return &Colors[3*n];
}
}
