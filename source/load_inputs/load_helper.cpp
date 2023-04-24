#include "load_helper.h"

#include <vector>
#include <string>

std::string trim(const std::string& s)
{
	size_t first = s.find_first_not_of(" \t\n\r\f\v");
	if (first == std::string::npos) {
		return "";
	}
	size_t last = s.find_last_not_of(" \t\n\r\f\v");
	return s.substr(first, last - first + 1);
}

std::vector<std::string> tokenize(std::string s, std::string delim)
{
	std::vector<std::string> tokenized_vec;

	size_t start, end = -1 * delim.size();
	do {
		start = end + delim.size();
		end = s.find(delim, start);
		tokenized_vec.push_back(trim(s.substr(start, end - start)));
	} while (end != -1);

	return tokenized_vec;
}