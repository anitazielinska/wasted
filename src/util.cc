#include "util.hh"

#include <sstream>
#include <fstream>

using namespace std;

bool readFile(string path, string &data) {
	ifstream file(path, ios::in);
	if (!file.is_open()) {
		fprintf(stderr, "error: couldn't open file '%s'\n", path.c_str());
		return false;
	}
	stringstream stream;
	stream << file.rdbuf();
	file.close();
	data = stream.str();
	return true;
}

