#include "util.h"

#include <sstream>

using std::string;

std::vector<string> split (const string &s, char delim) {
	std::vector<string> result;
	std::stringstream ss (s);
    string item;

    while (getline (ss, item, delim)) {
        result.push_back (item);
    }

    return result;
}
