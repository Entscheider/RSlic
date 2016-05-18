#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QSpinBox>
#include <QGroupBox>
#include <3rd/ThreadPool.h>
#include <opencv2/core/core.hpp>
#include <QPixmap>
#include <QImage>
#include "imgwdg.h"
#include <Pixel/RSlic2.h>
#include <QAction>
#include <QProgressBar>
#include <QThread>
#include <QTabWidget>
#include <QSlider>
#include <QColorDialog>
#include <QCheckBox>

class ImgWdg;
class SlicSettingWidget;
struct SlicSetting;

Q_DECLARE_METATYPE(RSlic::Pixel::Slic2P)
Q_DECLARE_METATYPE(vector<RSlic::Pixel::Slic2P>)
Q_DECLARE_METATYPE(cv::Mat)
Q_DECLARE_METATYPE(SlicSetting*)

// Computes Superpixel in different Thread.
// But communicates with Gui-Thread
class WorkerObject : public QObject {
	Q_OBJECT
public:
	WorkerObject(ThreadPoolP p, QObject *parent = nullptr);

public slots:
	void work(cv::Mat m, SlicSetting *setting, QWidget* wdg);
signals:
	void message(const QString &);

	void progress(int i);

	void finished(vector<RSlic::Pixel::Slic2P> res, QWidget* wdg);

	void failed(const QString &title, const QString &msg);

private:
	ThreadPoolP pool;
};

struct superpixeldata;
class MainWindow : public QMainWindow {
	Q_OBJECT
public:
	explicit MainWindow(QWidget *parent = nullptr);

	virtual ~MainWindow();

	signals:

public slots:
	void loadImage();

	void doSuperpixel();

	void repaintImg();

	void goPrev();

	void goNext();

	void duplicate();

	void closeCurrentTab();

	void addImage(const QString &filename);

	void saveImage();

protected:
	void dragEnterEvent(QDragEnterEvent *event);

	void dropEvent(QDropEvent *event);

	ImgWdg *addImgWdg(const QString &title);

	std::unique_ptr<superpixeldata> &currentData(ImgWdg** currentWdg = nullptr);
	const std::unique_ptr<superpixeldata> &currentDataConst(ImgWdg** currentWdg = nullptr) const;

private slots:
	void onWorkerMessage(const QString &msg);

	void onWorkerProgress(int i);

	void onWorkerFinished(vector<RSlic::Pixel::Slic2P> res, QWidget*wdg);

	void onWorkerFailed(const QString &title, const QString &msg);

	void updateTitleForIndex(int idx);

	void closeTab(int idx);

	void updateActions();

	void setClusterFill(bool);

	void setClusterContour(bool);

	void setSlicIdx(int idx);

	void setDrawColor(const QColor &);

private:
	QTabWidget *tabs;
	WorkerObject *worker;
	QThread workerThread;
	QProgressBar *progressbar;
	SlicSetting *settings;
	SlicSettingWidget *settingWdg;
	QSlider * slicIdxSlider;
	QAction *colorClusterAction, *drawContourAction, *goNextAction, *goPrevAction;
	std::unique_ptr<superpixeldata> nullp;
	QColorDialog * colorDlg;
};

struct SlicSetting {
	constexpr SlicSetting() noexcept: slico(false), iteration(10), stiffness(40), count(400), saveIterations(true) {}

	bool slico;
	int iteration;
	int stiffness;
	int count;

	bool saveIterations;
};

class SlicSettingWidget : public QWidget {
	Q_OBJECT
public:
	explicit SlicSettingWidget(QWidget *parent = nullptr);

	SlicSetting *buildSetting(); // Result have to be removed by hand
signals:
	void activate();

private:
	QSpinBox *iterationBox, *stiffnessBox, *countBox;
	QGroupBox *slicoBox;
	QCheckBox *saveBox;
};


#endif // MAINWINDOW_H
