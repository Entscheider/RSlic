#include <QApplication>
#include "mainwindow.h"

int main(int argc, char **argv) {
	QApplication app(argc, argv);
	MainWindow w;
#ifndef Q_OS_OSX
    QIcon icon(":/superpi.png");
    w.setWindowIcon(icon);
#endif
	w.show();
	return app.exec();
}
