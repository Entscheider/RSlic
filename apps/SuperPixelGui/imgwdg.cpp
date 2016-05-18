#include "imgwdg.h"
#include "utils.h"
#include <QVBoxLayout>

TripleImageConversion::TripleImageConversion() : img(nullptr), mat(nullptr), data(nullptr) {
}

void TripleImageConversion::setImg(const QImage &img) {
	auto tmp = new QImage(img);
	setImg(QPixmap::fromImage(img));
	this->img = tmp;
}


void TripleImageConversion::setImg(const QPixmap &px) {
	this->pxm = px;
	if (img != nullptr) delete img;
	img = nullptr;
	if (mat != nullptr) delete mat;
	mat = nullptr;
}

void TripleImageConversion::setImg(const cv::Mat &mat) {
	auto tmp = new cv::Mat(mat);
	setImg(utils::mat2QImage(mat));
	this->mat = tmp;
}


QImage TripleImageConversion::getImg() {
	if (img == nullptr) {
		img = new QImage(pxm.toImage());
	}
	return *img;
}

cv::Mat TripleImageConversion::getMat() {
	if (mat == nullptr) {
		mat = new cv::Mat(utils::qImage2Mat(getImg()));
	}
	return *mat;
}

QPixmap TripleImageConversion::getPixmap() const {
	return pxm;
}

bool TripleImageConversion::isSet() const {
	return !pxm.isNull();
}


void TripleImageConversion::clean() {
	if (data != nullptr) {
		data->decrement();
		if (data->zeroReference()) delete data;
	}
	if (img != nullptr) delete img;
	if (mat != nullptr) delete mat;
	img = nullptr;
	pxm = QPixmap();
	mat = nullptr;
	data = nullptr;
}

TripleImageConversion::~TripleImageConversion() {
	clean();
}

void ImgWdg::init() {
	this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	this->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	pitem = new QGraphicsPixmapItem();
	scene = new QGraphicsScene();
	scene->addItem(pitem);
	setScene(scene);
}

ImgWdg::ImgWdg(QWidget *parent) : QGraphicsView(parent) {
	init();
}

ImgWdg::ImgWdg(ImgWdg *other, QWidget *parent) :  QGraphicsView(parent), history(other->history) {
	init();
	reloadPixmap();
}

void ImgWdg::resizeEvent(QResizeEvent *event) {
	QGraphicsView::resizeEvent(event);
	this->fitInView(pitem, Qt::KeepAspectRatio);
}

TripleImageConversion *ImgWdg::currentImage() {
	return history.getItem();
}

const TripleImageConversion *ImgWdg::currentImage() const {
	return history.getItem();
}

TripleImageConversion *ImgWdg::createOne() {
	history.pushItem();
	history.getItem()->clean();
	return history.getItem();
}


void ImgWdg::reloadPixmap() {
	auto current = currentImage();
	if (current == nullptr) return;
	pitem->setPixmap(current->getPixmap());
	this->fitInView(pitem, Qt::KeepAspectRatio);
}

void ImgWdg::showImg(const QImage &img) {
	createOne()->setImg(img);
	reloadPixmap();
}

void ImgWdg::showImg(const QPixmap &px) {
	createOne()->setImg(px);
	reloadPixmap();
}

void ImgWdg::showImg(const cv::Mat &mat) {
	createOne()->setImg(mat);
	reloadPixmap();
}

void ImgWdg::replaceImg(const QImage & img){
    auto item = history.getItem();
    item->clean();
    item->setImg(img);
    reloadPixmap();
}

void ImgWdg::replaceImg(const QPixmap & img){
    auto item = history.getItem();
    item->clean();
    item->setImg(img);
    reloadPixmap();
}

void ImgWdg::replaceImg(const cv::Mat & img){
    auto item = history.getItem();
    item->clean();
    item->setImg(img);
    reloadPixmap();
}


QImage ImgWdg::getImg() {
	auto current = currentImage();
	if (current != nullptr)
		return current->getImg();
	return QImage();
}

cv::Mat ImgWdg::getMat() {
	auto current = currentImage();
	if (current != nullptr)
		return current->getMat();
	return cv::Mat();
}

QPixmap ImgWdg::getPixmap() const {
	auto current = currentImage();
	if (current != nullptr)
		return current->getPixmap();
	return QPixmap();
}

void ImgWdg::goNext() {
	history.redo();
	reloadPixmap();
}

void ImgWdg::goPrev() {
	history.undo();
	reloadPixmap();
}


bool ImgWdg::hasNext() {
	return history.redoAvailable();
}

bool ImgWdg::hasPrev() {
	return history.undoAvailable();
}


ImgWdg::~ImgWdg() {
}


