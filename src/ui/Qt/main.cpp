#include "shambles.hpp"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	Shambles w;
	w.show();
	return a.exec();
}
