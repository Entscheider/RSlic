#include "parser.h"
#include <cstring>

using namespace std;

bool file_exist(const char *filename) {
	struct stat buffer;
	return (stat(filename, &buffer) == 0);
}

ostream &operator<<(ostream &str, const vector<string> &list) {
	str << "[";
	for (const string s: list) {
		str << s << ",";
	}
	str << "]";
	return str;
}


void printHelp(char *name) {
	MainSetting *tmp = new MainSetting;
	cout << "Program to create Supervoxel from images" << endl;
	cout << name << " [-c ...] [-m ...] [-i ...] [-h] [-0] [-t ...] filename1 filename2 ... filename n [-o ...] " << endl;
	cout << "-c a: Set the number of Supervoxels to a (a is a number, default " << tmp->count << ")" << endl;
	cout << "-m a: Set stiffness to a (a is a number, default " << tmp->stiffness << ")" << endl;
	cout << "-i a: Set iteration count to a (a is a number, default " << tmp->iterations << ")" << endl;
	cout << "-o a: Set the output ply file to a. If not set, there will be no export." << endl;
	cout << "-t a: Set the number of thread to be used. -1 does automatically detecting. (a is a number, default " << tmp->threadcount << ")" << endl;
	cout << "-0: Use Slico algorithms. -m will be ignored (default "<< (tmp->slico?"true":"false")<<")" << endl;
	cout << "-h: Print this help" << endl;
	cout << "filenames: Set the filename of the image to convert (filename is a list the path, required)" << endl;
	delete tmp;
}

MainSetting *parseSetting(int argc, char **argv) {
	if (argc < 2) {
		std::cout << "[Error] Not enough arguments" << std::endl;
		printHelp(argv[0]);
		return nullptr;
	}
	MainSetting *res = new MainSetting();
	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-c") == 0 && i + 1 < argc) {
			res->count = atoi(argv[i + 1]);
			i++;
		} else if (strcmp(argv[i], "-m") == 0 && i + 1 < argc) {
			res->stiffness = atoi(argv[i + 1]);
			i++;
		} else if (strcmp(argv[i], "-i") == 0 && i + 1 < argc) {
			res->iterations = atoi(argv[i + 1]);
			i++;
		} else if (strcmp(argv[i], "-h") == 0) {
			delete res;
			printHelp(argv[0]);
			return nullptr;
		} else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
			res->outputfile = argv[i + 1];
			i++;
		} else if (strcmp(argv[i], "-t") == 0 && i + 1 < argc) {
			res->threadcount = atoi(argv[i + 1]);
			i++;
		} else if (strcmp(argv[i], "-0") == 0) {
			res->slico = true;
		}else {
			char *filename = argv[i];
			if (!file_exist(filename)) {
				cout << "[Error] File " << filename << " does not exists" << std::endl;
				delete res;
				return nullptr;
			}
			res->filenames.push_back(filename);
		}
	}
	if (res->filenames.empty()){ //FIXME: Check for whitespace and similar 
		printHelp(argv[0]);
		delete res;
		return nullptr;
	}
	return res;
}
