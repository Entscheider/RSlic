#include "exporter.h"
#include <iostream>
#include <fstream>
using namespace std;
using namespace RSlic::Voxel;

// Return coordinates of points which are between clusters
// mask.empty => ignore mask 
vector<Vec3i> contourVertex(Slic3P p, const Mat &mask) {
	Mat_<ClusterInt> clusters = p->getClusters().getClusterLabel();
	int w = clusters.size[1];
	int h = clusters.size[0];
	int d = clusters.size[2];
	vector<Vec3i> res;
	for (int x = 0; x < w - 1; x++) {
		for (int y = 0; y < h - 1; y++) {
			for (int t = 0; t < d - 1; t++) {
				if (clusters.at<ClusterInt>(y, x, t) != clusters.at<ClusterInt>(y, x, t + 1)
						|| clusters.at<ClusterInt>(y, x, t) != clusters.at<ClusterInt>(y, x + 1, t)
						|| clusters.at<ClusterInt>(y, x, t) != clusters.at<ClusterInt>(y + 1, x, t)){
					if (!mask.empty() && mask.at<uint8_t>(y,x,t) == 0) continue;
					res.push_back(Vec3i(x, y, t));
				}
			}
		}
	}
	return res;
}

void exportPLY(const vector<Vec3i> &vertex, const string &filename,RSlic::Voxel::MovieCacheP img, const string & comments) {
	ofstream out;
	out.open(filename);
	if (out.fail()) {
		cout << "[ERROR] Opening " << filename << " failed" << endl;
		return;
	}
	out << "ply" << endl;
	if (!comments.empty())
		out << "comment " <<comments << endl;
	out << "format ascii 1.0 " << endl;
	out << "element vertex " << vertex.size() << endl;
	//out<<"property list uchar int vertex_index"<<endl;
	out << "property int x" << endl
		<< "property int y" << endl
		<< "property int z" << endl
		<< "property uchar red "<<endl
		<< "property uchar green"<<endl
		<<"property uchar blue"<<endl;
	out << "end_header" << endl;
	for (const Vec3i &vec: vertex) {
		int x = vec[0];
		int y = vec[1];
		int z = vec[2];
		Vec3b color(0,0,0);
		if (img->type() == CV_8UC1) color = Vec3b::all(img->at<uint8_t>(y,x,z));
		else if (img->type()==CV_8UC3) color = img->at<Vec3b>(y,x,z);
		int r = color[1] , g = color[0],b = color[2];
		out << x << ' ' << y << ' ' << z
			<< ' ' << r <<' ' << g << ' ' << b <<endl;
	}
	out.close();
}




#include <opencv2/highgui/highgui.hpp>
template <typename T>
void showNrIntern(cv::Mat labels, int i, bool tIgnore, Mat &img, T value){
	for (int x = 1; x < img.cols - 1; x++) {
		for (int y = 1; y < img.rows - 1; y++) {
			if (labels.at<ClusterInt>(y, x, i) < 0){
				std::cout<<"Fail";
				continue;
			}
			if ((labels.at<ClusterInt>(y, x, i) != labels.at<ClusterInt>(y, x + 1, i)
						|| labels.at<ClusterInt>(y, x, i) != labels.at<ClusterInt>(y, x - 1, i)
						|| labels.at<ClusterInt>(y, x, i) != labels.at<ClusterInt>(y + 1, x, i)
						|| labels.at<ClusterInt>(y, x, i) != labels.at<ClusterInt>(y - 1, x, i))
					&& (tIgnore || (labels.at<ClusterInt>(y, x, i) == labels.at<ClusterInt>(y, x, i-1)
							&& labels.at<ClusterInt>(y, x, i) == labels.at<ClusterInt>(y, x, i+1)
							))
				 )
				img.at<T>(y, x) = value;
		}
	}
}

// Get a 2-D image visualization
// tIgnore: ignore time-dimension for cluster contour
void showNr(Slic3P p, int i, bool tIgnore) {
	auto img = p->getImg()->matAt(i).clone();
	auto labels = p->getClusters().getClusterLabel();
	if (img.type()==CV_8UC3){
		cv::cvtColor(img, img, cv::COLOR_Lab2BGR);
		showNrIntern<Vec3b>(labels,i,tIgnore,img,Vec3b(255,255,255));
	}else{
		showNrIntern<uint8_t>(labels,i,tIgnore,img,255);
	}

	imshow("Hi", img);
}
#include <unordered_set>

// Show first image
// and handling for key events
void showFirst(Slic3P p) {
	int i = 0;
	const int d = p->getImg()->duration();
	bool tIgnore(false);
	showNr(p, i, tIgnore);
	char c;
	do {
		c = waitKey(-1);
		if (c == 's') {
			i = min(i + 1, d - 1);
			showNr(p, i, tIgnore);
		}
		else if (c == 't'){
			tIgnore = !tIgnore;
			showNr(p,i,tIgnore);
		}
		else if (c == 'a') {
			i = max(i - 1, 0);
			showNr(p, i, tIgnore);
		}else if (c=='d'){
			auto labels = p->getClusters().getClusterLabel();
			std::cout<<"Nr. "<<i<<std::endl;
			std::unordered_set<ClusterInt> set;
			auto img = p->getImg()->matAt(i);
			for (int x = 1; x < img.cols - 1; x++) {
				for (int y = 1; y < img.rows - 1; y++) {
					set.insert(labels.at<ClusterInt>(y,x,i));
				}
			}
			for (ClusterInt c : set){
				std::cout<<c<<",";
			}
			std::cout<<std::endl;
		}
	} while (c != 'q');
}
