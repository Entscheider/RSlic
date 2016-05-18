#include <opencv2/highgui/highgui.hpp>
#include <iostream>
#include <RSlic2H.h>
#include <3rd/ThreadPool.h>

using namespace RSlic;

inline void showImg(const ClusterSet &cl, const cv::Mat &img, const string &name, bool white) {
	int gray = white ? 255 : 0;
	Vec3b color = white ? Vec3b(255, 255, 255) : Vec3b(0, 0, 0);
	switch (img.type()) {
		case CV_8UC3:
			//cvtColor(img,res,CV_Lab2BGR);
			cv::imshow(name, contourCluster(drawCluster(img, cl), cl, color));
			cv::imshow(name + "orig", contourCluster(img, cl, color));
			break;
		case CV_8UC1:
			cv::imshow(name, contourCluster<uint8_t>(drawCluster(img, cl), cl, gray));
			cv::imshow(name + "orig", contourCluster<uint8_t>(img, cl, gray));
			break;
	}
}

inline void showClusters(const ClusterSet &cl, const cv::Mat &img, const std::string &name) {
	//cv::Mat res;
	showImg(cl, img, name, false);
	char key = -1;
	while (key != 'q') { 
		key = cv::waitKey(-1);
		if (key == 'b') {
			showImg(cl, img, name, false);
		} else if (key == 'w') {
			showImg(cl, img, name, true);
		}
	}
}

#include <sys/stat.h>

bool file_exist(const char *filename) {
	struct stat buffer;
	return (stat(filename, &buffer) == 0);
}

struct MainSetting {
	MainSetting() : count(400), stiffness(40), iterations(10), slico(false), threadcount(-1) {
	}

	string filename;
	int count;
	int stiffness;
	int iterations;
	bool slico;
	int threadcount;

	int guessthreadcount() {
		if (threadcount <= 0)
			return std::thread::hardware_concurrency();
		return threadcount;
	}

	void print() {
		std::cout << "[] Iterations " << iterations << endl;
		std::cout << "[] Count " << count << endl;
		std::cout << "[] stiffness " << stiffness << endl;
		std::cout << "[] filename " << filename << endl;
		std::cout << "[] slico " << slico << endl;
		std::cout << "[] threadcount " << threadcount << endl;
	}
};

void printHelp(char *name) {
	MainSetting *tmp = new MainSetting;
	cout << "Create Superpixel from an image" << endl;
	cout << name << " [-c ...] [-m ...] [-i ...] [-o] [-h] [-t ...] filename " << endl;
	cout << "-c a: Set the number of superpixel to a (a is a number, default " << tmp->count << ")" << endl;
	cout << "-m a: Set stiffness to a (a is a number, default " << tmp->stiffness << ")" << endl;
	cout << "-i a: Set iteration count to a (a is a number, default " << tmp->iterations << ")" << endl;
	cout << "-0: Use Slico (zero-parameter variant of Slic, default " << tmp->slico << ", -m will be ignored)" << endl;
	cout << "-t a: Set the number of thread to be used to a (a is a number, default " << tmp->threadcount << ")" << endl;
	cout << "-h: Print this help" << endl;
	cout << "filename: Set the filename of the image to convert (filename is the path)" << endl;
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
		} else if (strcmp(argv[i], "-0") == 0) {
			res->slico = true;
		} else if (strcmp(argv[i], "-h") == 0) {
			delete res;
			printHelp(argv[0]);
			return nullptr;
		} else
			res->filename = argv[i];
	}
	if (res->filename.empty()) { //FIXME: Check for whitespace and similar 
		printHelp(argv[0]);
		delete res;
		return nullptr;
	}
	if (!file_exist(res->filename.c_str())) {
		cout << "[Error] File " << res->filename << " does not exists" << std::endl;
		delete res;
		return nullptr;
	}
	return res;
}

int main(int argc, char **argv) {
    MainSetting *settings = parseSetting(argc, argv);
	if (settings == nullptr) return -1;
	if (settings->slico) settings->stiffness = 1; 
#ifdef DEBUG_ME
	settings->print();
#endif

	auto img = cv::imread(settings->filename, cv::IMREAD_UNCHANGED);
	if (img.type() == CV_8UC4) {
		cv::cvtColor(img, img, CV_BGRA2BGR);
	}
	auto grad = buildGrad(img); 

	cout << "Image Type: " << getType(img) << ", Gradient: " << getType(grad) << endl;
	
	auto s = sqrt(img.cols * img.rows / settings->count);
	
	Slic2P slic;
	ThreadPoolP pool = std::make_shared<ThreadPool>(settings->guessthreadcount());
	Mat img_lab;

	switch (img.type()) {
		case CV_8UC3:
			cvtColor(img, img_lab, CV_BGR2Lab); // See paper
			slic = Slic2::initialize(img_lab, grad,  s, settings->stiffness, pool);
			break;
		case CV_8UC1:
			slic = Slic2::initialize(img, grad,  s, settings->stiffness, pool);
			break;
		default:
			cout << "This image type is not currently supported " << endl;
			return -1;
	}
	cv::imshow("orig", img);

#ifdef DEBUG_ME
	showClusters(slic->getClusters(), img, "res");
#endif
	// Iteration
	std::cout << "* Process: 0 from " << settings->iterations << std::flush;
	for (int i = 0; i < settings->iterations; i++) {
		std::cout << '\r' << "* Process: " << i + 1 << " from " << settings->iterations << std::flush;
		slic = RSlic::Pixel::iteratingHelper(slic,img.type(),settings->slico);
#ifdef DEBUG_ME
		showClusters(slic->getClusters(), img, std::string("res_")+std::to_string(i));
#endif
	}

	std::cout << std::endl << "* Finalize Clusters..." << std::flush;
	if (img.type() == CV_8UC1){
		distanceGray g;
		slic = slic->finalize<distanceGray>(g); 
	}else{
		distanceColor c;
		slic = slic->finalize<distanceColor>(c);
	}
	std::cout << " Finish" << std::endl << "* Drawing Clusters..." << std::flush;
	std::cout << " Finish" << std::endl << "Press q to quit" << std::endl << std::flush;
	delete settings;
	showClusters(slic->getClusters(), img, "result");
}
