#ifndef RESOURCE_HPP_INCLUDED
#define RESOURCE_HPP_INCLUDED

namespace resource {
int num_resources();
char resource_id(int n);
const char* resource_name(int n);
const char* resource_icon(int n);
int resource_index(char c);
const int* resource_color(int n);
}

#endif
