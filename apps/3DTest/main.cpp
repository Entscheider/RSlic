#include <iostream>
#include <RSlic3H.h>
#include <3rd/ThreadPool.h>

#include "parser.h"
#include "exporter.h"

using namespace RSlic::Voxel;
using namespace std;

template<typename T>
auto measureTime(std::function<T()> function) -> std::tuple<T, std::chrono::duration<double>> {
	std::chrono::time_point<std::chrono::system_clock> start, end;
	start = std::chrono::system_clock::now();
	T res = function();
	end = std::chrono::system_clock::now();
	return std::make_tuple(res, end - start);
}

template<typename T>
auto measureTimeV(std::function<T()> function) ->  std::chrono::duration<double> {
	std::chrono::time_point<std::chrono::system_clock> start, end;
	start = std::chrono::system_clock::now();
	function();
	end = std::chrono::system_clock::now();
	return end - start;
}

MovieCacheP loadFiles(MainSetting *s) {
	return std::make_shared<SimpleMovieCache>(s->filenames);
}

int main(int argc, char **argv) {
	MainSetting *settings = parseSetting(argc, argv);
	if (settings == nullptr) return -1;

	MovieCacheP img = loadFiles(settings);
	cout << "Width: " << img->width() << ", Height: " << img->height()
		<< " Duration: " << img->duration() << endl;
	auto s = pow(img->width() * img->height() * img->duration() *1.0 / settings->count,0.333); 
	if (std::min(img->duration(), std::min(img->width(), img->height())) <= s / 2) {
		cout << "[Warning] Not enough pictures. At least " << s << " pictures are recommended" << endl;
	}
	Slic3P slic;
	if (settings->threadcount <=0) settings->threadcount=std::thread::hardware_concurrency();
	ThreadPoolP pool = std::make_shared<ThreadPool>(settings->threadcount);
	slic = Slic3::initialize(img, buildGradColor, s, settings->stiffness, pool);

	if (slic.get() == nullptr) {
		cout << "[ERROR] Not able to perform the algorithm. May you should play with the parameters." << endl;
		return -1;
	}
	//showFirst(slic);

	// Iteration
	std::cout << "* Process: 0 of " << settings->iterations << std::flush;
	auto res = measureTimeV<void>([&]() {
			for (int i = 0; i < settings->iterations; i++) {
			std::cout << '\r' << "* Process: " << i + 1 << " of " << settings->iterations << std::flush;
			slic = RSlic::Voxel::iterateHelper(slic,img->type(),settings->slico);
			if (slic == nullptr){
			cout << "This image type is not currently supported " << endl;
			exit(EXIT_FAILURE);
			}
			//showFirst(slic);
			}
			});
	cout<<" - Needed "<<res.count()<<" Seconds";
	//showFirst(slic);
	std::cout << std::endl << "* Finalize Clusters..." << std::flush;
	slic = slicFunHelper(img->type(),slic->finalize); //Cluster finalizing
	showFirst(slic);
	std::cout << " Finish" << std::endl;
	if (!settings->outputfile.empty()) {
		cout << "* Exporting " << std::flush;
		Mat mask;
		stringstream str;
		str << "iterations: "<<settings->iterations << ", stiffness: "<<settings->stiffness <<", count: "<<settings->count;
		str << ", Slico?: "<< (settings->slico?"true":"false") ;
		exportPLY(contourVertex(slic,mask), settings->outputfile,img,str.str());
		std::cout << " Finish" << std::endl;
	}
	delete settings;
}
