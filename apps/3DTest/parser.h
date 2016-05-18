#ifndef PARSER_H
#define PARSER_H

#include <iostream>
#include <sys/stat.h>
#include <vector>

bool file_exist(const char *filename);

std::ostream &operator<<(std::ostream &str, const std::vector<std::string> &list);

struct MainSetting {
	MainSetting() : count(32400), stiffness(40),
	threadcount(-1), iterations(10), slico(false) {}

	std::vector<std::string> filenames;
	std::string outputfile;
	int count;
	int stiffness;
	int iterations;
	bool slico;
	int threadcount;

	void print() {
		std::cout << "[] Iterations " << iterations << std::endl;
		std::cout << "[] Count " << count << std::endl;
		std::cout << "[] stiffness " << stiffness << std::endl;
		std::cout << "[] filenames " << filenames << std::endl;
	}
};

void printHelp(char *name);

MainSetting *parseSetting(int argc, char **argv);

#endif // PARSER_H
