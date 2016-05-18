#include "mainwindow.h"
#include "utils.h"
#include <Pixel/RSlic2.h>
#include <Pixel/RSlic2Util.h>
#include <Pixel/RSlic2Draw.h>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QToolBar>
#include <QFrame>
#include <QDockWidget>
#include <QPushButton>
#include <QStatusBar>
#include <QMessageBox>
#include <QDropEvent>
#include <QDragEnterEvent>
#include <QApplication>
#include <QStyle>
#include <QMimeData>
#include <QUrl>

#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
#define QStringLiteral QString
#endif

struct superpixeldata {
	vector<RSlic::Pixel::Slic2P> slicv;
	cv::Mat mat;
	bool drawContour = true;
	bool fillCluster = false;
	int currentIdx = 0;
	Vec3b color = Vec3b(0,0,0);

	RSlic::Pixel::Slic2P currentItem(){
		assert(0<= currentIdx && currentIdx <= slicv.size());
		return slicv[currentIdx];
	}
};

WorkerObject::WorkerObject(ThreadPoolP p, QObject *parent) : QObject(parent), pool(p) {
}

void WorkerObject::work(cv::Mat m, SlicSetting *settings, QWidget* tabwdg) {
	std::chrono::time_point<std::chrono::system_clock> start, end;
	start = std::chrono::system_clock::now();

	int step = sqrt(m.cols * m.rows / settings->count);
	auto grad = RSlic::Pixel::buildGrad(m);
	emit message(tr("Initializing Slic ..."));
	auto slic = RSlic::Pixel::Slic2::initialize(utils::makeLabIfNecessary(m), grad, step, settings->stiffness, pool);
	if (slic.get() == nullptr) {
		emit failed(tr("Wrong Parameter"), tr("No Superpixel can be build. May you should play with the parameters"));
		return;
	}
	emit message(tr("Iterating"));
	vector<RSlic::Pixel::Slic2P> iterators;
	if (settings->saveIterations)
		iterators.reserve(settings->iteration);
	for (int i = 0; i < settings->iteration; i++) {
		if (settings->slico)
			slic = slic->iterateZero(RSlic::Pixel::distanceColor());
		else
			slic = slic->iterate(RSlic::Pixel::distanceColor());
		if (settings->saveIterations)
			iterators.push_back(slic);
		emit message(tr("Iterating %1 of %2").arg(i + 1).arg(settings->iteration));
		emit progress((i + 1) * 100 / (settings->iteration + 1));
	}
	emit message(tr("Finalizing ..."));
	slic = slic->finalize(RSlic::Pixel::distanceColor());

	end = std::chrono::system_clock::now();
	std::chrono::duration<double> elapsed_seconds = end-start;

	iterators.push_back(slic);
	emit finished(std::move(iterators),tabwdg);
	emit message(tr("Needed %1 sec").arg(elapsed_seconds.count()));
}

MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent), settings(nullptr) {
		qRegisterMetaType<RSlic::Pixel::Slic2P>();
		qRegisterMetaType<cv::Mat>("cv::Mat const&");
		qRegisterMetaType<SlicSetting *>("SlicSetting*");
		qRegisterMetaType<RSlic::Pixel::Slic2P>("shared_ptr<Slic2P>const&");
		qRegisterMetaType<vector<RSlic::Pixel::Slic2P>>("vector<Slic2P>const&");

		this->setWindowTitle(QStringLiteral("SuperPixelGui"));
		this->setUnifiedTitleAndToolBarOnMac(true);
		this->setAcceptDrops(true);
		tabs = new QTabWidget(this);
		tabs->setMovable(true);
		tabs->setDocumentMode(true);
		tabs->setTabsClosable(true);
		connect(tabs, SIGNAL(currentChanged(int)), SLOT(updateTitleForIndex(int)));
		connect(tabs, SIGNAL(currentChanged(int)), SLOT(updateActions()));
		connect(tabs, SIGNAL(tabCloseRequested(int)),SLOT(closeTab(int)));
		setCentralWidget(tabs);

		worker = new WorkerObject(std::make_shared<ThreadPool>(std::thread::hardware_concurrency()));
		worker->moveToThread(&workerThread);
		workerThread.start();

		connect(worker, SIGNAL(failed(
						const QString &, const QString &)), this, SLOT(onWorkerFailed(
							const QString &, const QString &)), Qt::QueuedConnection);
		connect(worker, SIGNAL(finished(vector<RSlic::Pixel::Slic2P>,QWidget*)), this, SLOT(onWorkerFinished(vector<RSlic::Pixel::Slic2P>,QWidget*)), Qt::QueuedConnection);
		connect(worker, SIGNAL(progress(int)), this, SLOT(onWorkerProgress(int)), Qt::QueuedConnection);
		connect(worker, SIGNAL(message(
						const QString&)), this, SLOT(onWorkerMessage(
							const QString &)));

		colorDlg = new QColorDialog(this);
		colorDlg->setCurrentColor(QColor(0,0,0));
		connect(colorDlg,SIGNAL(colorSelected(QColor)),SLOT(setDrawColor(QColor)));

		auto style = QApplication::style();

		QToolBar *bar = this->addToolBar(tr("File"));
		QToolBar* viewbar = this->addToolBar(tr("View"));
		bar->addAction(style->standardIcon(QStyle::SP_DialogOpenButton), "Load...", this, SLOT(loadImage()));
		bar->addAction(style->standardIcon(QStyle::SP_DialogSaveButton), "Save...", this, SLOT(saveImage()));
		colorClusterAction = new QAction("Color", this);
		colorClusterAction->setCheckable(true);
		colorClusterAction->setChecked(true);
		connect(colorClusterAction, SIGNAL(triggered(bool)), SLOT(setClusterFill(bool)));
		viewbar->addAction(colorClusterAction);
		drawContourAction = new QAction("Draw", this);
		drawContourAction->setCheckable(true);
		colorClusterAction->setChecked(true);
		connect(drawContourAction, SIGNAL(triggered(bool)), SLOT(setClusterContour(bool)));
		viewbar->addAction(drawContourAction);
		viewbar->addAction("Set Color",colorDlg,SLOT(open()));
		bar->addSeparator();
		goPrevAction = bar->addAction(style->standardIcon(QStyle::SP_ArrowLeft), "Prev", this, SLOT(goPrev()));
		goNextAction = bar->addAction(style->standardIcon(QStyle::SP_ArrowRight), "Next", this, SLOT(goNext()));
		bar->addSeparator();
		bar->addAction("Dupl", this, SLOT(duplicate()));
		bar->addAction("Close", this, SLOT(closeCurrentTab()));;
		slicIdxSlider = new QSlider(Qt::Horizontal);
		slicIdxSlider->setTracking(false);
		slicIdxSlider->setValue(0);
		slicIdxSlider->setMaximum(1);
		slicIdxSlider->setEnabled(true);
		connect(slicIdxSlider,SIGNAL(valueChanged(int)),this,SLOT(setSlicIdx(int)));
		viewbar->addSeparator();
		viewbar->addWidget(slicIdxSlider);

		QDockWidget *dock = new QDockWidget("Slic", this);
		settingWdg = new SlicSettingWidget(dock);
		connect(settingWdg, SIGNAL(activate()), this, SLOT(doSuperpixel()));
		dock->setWidget(settingWdg);
		this->addDockWidget(Qt::LeftDockWidgetArea, dock);

		progressbar = new QProgressBar(this);
		progressbar->setRange(0, 100);
		this->statusBar()->addPermanentWidget(progressbar);
	}

void MainWindow::setSlicIdx(int i){
	auto& data = currentData();
	if (data == nullptr) return;
	data->currentIdx = i;
	repaintImg();
}

void MainWindow::setDrawColor(const QColor & c){
	auto & data = currentDataConst();
	auto oldColor = data->color;
	data->color = Vec3b(c.blue(),c.green(),c.red());
	if (data->drawContour && data->color != oldColor)
		repaintImg();
}

void MainWindow::goNext() {
	ImgWdg *imgwdg = qobject_cast<ImgWdg*>(tabs->currentWidget());
	if (imgwdg == nullptr) return;
	imgwdg->goNext();
	updateActions();
}

void MainWindow::goPrev() {
	ImgWdg *imgwdg = qobject_cast<ImgWdg*>(tabs->currentWidget());
	if (imgwdg == nullptr) return;
	imgwdg->goPrev();
	updateActions();
}

void MainWindow::onWorkerProgress(int i) {
	progressbar->setValue(i);
}

void MainWindow::onWorkerFinished(vector<RSlic::Pixel::Slic2P> res,QWidget* wdg) {
	settingWdg->setEnabled(true);
	this->statusBar()->clearMessage();
	progressbar->setValue(0);

	int idx = tabs->indexOf(wdg);
	if (idx < 0) return; // Tab was closed while processing
	ImgWdg *imgwdg = qobject_cast<ImgWdg*>(wdg);
	if (imgwdg == nullptr) return;
	tabs->setCurrentIndex(idx);

	cv::Mat m(imgwdg->getMat());
	imgwdg->showImg(m);
	auto more = imgwdg->currentImage();
	if (more != nullptr) {
		superpixeldata *data = new superpixeldata;
		data->mat = m;
		data->slicv = std::move(res);
		data->currentIdx = data->slicv.size()-1;
		data->drawContour = drawContourAction->isChecked();
		data->fillCluster = colorClusterAction->isChecked();
		auto color = colorDlg->currentColor();
		data->color = Vec3b(color.blue(),color.red(),color.green());
		more->setMoreData(std::unique_ptr<superpixeldata>(data));
	}
	repaintImg();
	updateActions();
}

void MainWindow::updateActions() {
	ImgWdg *imgwdg;
	auto & data = currentDataConst(&imgwdg);
	if (imgwdg == nullptr) return ;
	goNextAction->setEnabled(imgwdg->hasNext());
	goPrevAction->setEnabled(imgwdg->hasPrev());
	slicIdxSlider->setEnabled(false);

	if (data == nullptr) return;
	drawContourAction->setChecked(data->drawContour);
	colorClusterAction->setChecked(data->fillCluster);
	colorDlg->setCurrentColor(QColor(data->color[2],data->color[1],data->color[0]));
	if (data->slicv.size() > 1){
		slicIdxSlider->setEnabled(true);
		slicIdxSlider->setMaximum(data->slicv.size()-1);
		slicIdxSlider->setValue(data->currentIdx);
	}
}

void MainWindow::setClusterFill(bool b) {
	auto &data = currentDataConst();
	if (data == nullptr) return;
	if (b!=data->fillCluster){
		auto& data = currentData();
		data->fillCluster = b;
		repaintImg();
	}
}

void MainWindow::setClusterContour(bool b) {
	auto& data = currentDataConst();
	if (data == nullptr) return;
	if (b!=data->drawContour){
		auto &data = currentData();
		data->drawContour = b;
		repaintImg();
	}
}

void MainWindow::onWorkerMessage(const QString &msg) {
	this->statusBar()->showMessage(msg);
}

void MainWindow::onWorkerFailed(const QString &title, const QString &msg) {
	QMessageBox::critical(this, title, msg);
	progressbar->setValue(0);
	settingWdg->setEnabled(true);
	this->statusBar()->clearMessage();
}

void MainWindow::doSuperpixel() {
	ImgWdg *imgwdg = qobject_cast<ImgWdg*>(tabs->currentWidget());
	if (imgwdg == nullptr) {
		loadImage();
		return;
	}

	if (settings != nullptr) delete settings;
	settings = settingWdg->buildSetting();
	auto pixmap = imgwdg->getPixmap();
	if (pixmap.isNull()) {
		loadImage();
		return;
	}
	auto mat = imgwdg->getMat();
	settingWdg->setEnabled(false);
	QMetaObject::invokeMethod(worker, "work", Qt::QueuedConnection, Q_ARG(cv::Mat, mat), Q_ARG(SlicSetting *, settings),Q_ARG(QWidget*,imgwdg));
}

Vec3b operator+(const Vec3d& a,const Vec3b &b){
	return Vec3b(a[0]+b[0],a[1]+b[1],a[2]+b[2]);
}

void MainWindow::repaintImg() {
	ImgWdg* imgwdg;
	auto & data = currentDataConst(&imgwdg);
	if (data == nullptr) return;
	auto more = imgwdg->currentImage();
	cv::Mat m(data->mat);
	Mat grad = RSlic::Pixel::buildGrad(m);
	if (data->fillCluster) {
		m = RSlic::Pixel::drawCluster(m, data->currentItem()->getClusters());
	}
	if (data->drawContour) {
		m = RSlic::Pixel::contourCluster(m, data->currentItem()->getClusters(), data->color);
	}
	more->setImg(m);
	imgwdg->reloadPixmap();
}

void MainWindow::duplicate() {
	ImgWdg *imgwdg = qobject_cast<ImgWdg*>(tabs->currentWidget());
	QString title = tabs->tabText(tabs->currentIndex());
	if (imgwdg == nullptr) return;
	ImgWdg *newone = new ImgWdg(imgwdg, this);
	tabs->addTab(newone, QStringLiteral("%1+").arg(title));
}

void MainWindow::closeCurrentTab() {
	int idx = tabs->currentIndex();
	if (idx < 0) return;
	auto wdg = this->tabs->widget(idx);
	this->tabs->removeTab(idx);
	wdg->deleteLater();
}


MainWindow::~MainWindow() {
	if (settings != nullptr) delete settings;
	workerThread.quit();
	workerThread.wait();
}

#include <opencv2/highgui/highgui.hpp>

void MainWindow::loadImage() {
	QString file = QFileDialog::getOpenFileName(this);
	if (file.isNull()) return;
	addImage(file);
}

void MainWindow::addImage(const QString &file) {
	auto imgwdg = addImgWdg(file);
#define USE_CV
#ifdef USE_CV
	cv::Mat m = cv::imread(file.toStdString());
	imgwdg->showImg(m);
#else
	QImage img(file);
	imgwdg->showImg(img);
#endif
}

void MainWindow::saveImage() {
	QString file = QFileDialog::getSaveFileName(this);
	if (file.isNull()) return;
	ImgWdg *imgwdg = (ImgWdg *) tabs->currentWidget();
	if (imgwdg == nullptr) return;
	imgwdg->getImg().save(file);
}

ImgWdg *MainWindow::addImgWdg(const QString &title) {
	auto imgwdg = new ImgWdg(this);
	tabs->addTab(imgwdg, title);
	return imgwdg;
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event) {
	if (event->mimeData()->hasImage() || event->mimeData()->hasUrls() || event->mimeData()->hasText()) {
		event->acceptProposedAction();
	}
}

void MainWindow::dropEvent(QDropEvent *event) {
	if (event->mimeData()->hasImage()) {
		QImage img = qvariant_cast<QImage>(event->mimeData()->imageData());
		ImgWdg *w = addImgWdg("drop content");
		w->showImg(img);
		event->acceptProposedAction();
	} else if (event->mimeData()->hasUrls()) {
		auto urls = event->mimeData()->urls();
		for (auto url : urls) {
			if (!url.isLocalFile()) continue;
			auto filename = url.toLocalFile();
			if (!QFile::exists(filename)) continue;
			addImage(filename);
		}
		event->acceptProposedAction();
	} else if (event->mimeData()->hasText()) {
		auto text = event->mimeData()->text();
		if (!QFile::exists(text)) return;
		addImage(text);
		event->acceptProposedAction();
	}
}

namespace {
	inline QSpinBox *createBox(int min, int max, int def, QWidget *parent = nullptr) {
		auto res = new QSpinBox(parent);
		res->setMinimum(min);
		res->setMaximum(max);
		res->setValue(def);
		return res;
	}

	inline QHBoxLayout *titleMe(QWidget *w, const QString &title, QWidget *parent = nullptr) {
		QHBoxLayout *res = new QHBoxLayout(parent);
		res->addWidget(new QLabel(title));
		res->addWidget(w);
		return res;
	}
}

SlicSettingWidget::SlicSettingWidget(QWidget *parent) : QWidget(parent) {
	QPushButton *b = new QPushButton(tr("Do"));
	connect(b, SIGNAL(pressed()), this, SIGNAL(activate()));
	iterationBox = createBox(1, 1000, 10);
	stiffnessBox = createBox(1, 250, 40);
	countBox = createBox(10, 1000, 400);
	slicoBox = new QGroupBox(tr("No Slico"), this);
	slicoBox->setCheckable(true);
	saveBox = new QCheckBox(tr("Save Iterations"), this);
	saveBox->setChecked(true);

	slicoBox->setLayout(titleMe(stiffnessBox, tr("Stiffness")));

	QVBoxLayout *lay = new QVBoxLayout();
	lay->addWidget(b);
	lay->addLayout(titleMe(iterationBox, tr("Iterations")));
	lay->addLayout(titleMe(countBox, tr("Number of Superpixels")));
	lay->addWidget(slicoBox);
	QFrame * line = new QFrame();
	line->setFrameShape(QFrame::HLine);
	line->setFrameShadow(QFrame::Sunken);
	lay->addWidget(line);
	lay->addWidget(saveBox);
	lay->addStretch();

	setLayout(lay);
}

SlicSetting *SlicSettingWidget::buildSetting() {
	SlicSetting *res = new SlicSetting;
	res->slico = !slicoBox->isChecked();
	res->iteration = iterationBox->value();
	res->stiffness = stiffnessBox->value();
	res->count = countBox->value();
	res->saveIterations = saveBox->isChecked();
	return res;
}

void MainWindow::updateTitleForIndex(int idx) {
	if (idx < 0) return;
	auto name = tabs->tabText(idx);
	setWindowTitle(QStringLiteral("%1 - SuperPixelGui").arg(name));
}

void MainWindow::closeTab(int idx) {
	if (idx < 0) return;
	auto wdg = this->tabs->widget(idx);
	this->tabs->removeTab(idx);
	wdg->deleteLater();
}

const std::unique_ptr<superpixeldata> &MainWindow::currentDataConst(ImgWdg** currentWdg) const{
	if (currentWdg != nullptr)
		*currentWdg = nullptr;
	ImgWdg *imgwdg = qobject_cast<ImgWdg*>(tabs->currentWidget());
	if (imgwdg == nullptr) return nullp;
	if (currentWdg != nullptr)
		*currentWdg = imgwdg;
	const TripleImageConversion* const  more = imgwdg->currentImage();
	if (more == nullptr) return nullp;
	return more->moreData<superpixeldata>();
}

std::unique_ptr<superpixeldata> &MainWindow::currentData(ImgWdg** currentWdg){
	if (currentWdg != nullptr)
		*currentWdg = nullptr;
	ImgWdg *imgwdg = qobject_cast<ImgWdg*>(tabs->currentWidget());
	if (imgwdg == nullptr) return nullp;
	if (currentWdg != nullptr)
		*currentWdg = imgwdg;
	auto more = imgwdg->currentImage();
	if (more == nullptr) return nullp;
	return more->moreData<superpixeldata>();
}
