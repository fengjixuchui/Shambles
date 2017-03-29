#ifndef __SHAMBLES_H_
#define __SHAMBLES_H_

#include <QtWidgets/QMainWindow>
#include "ui_shambles.h"

class Shambles : public QMainWindow
{
	Q_OBJECT

public:
	Shambles(QWidget *parent = 0);
	~Shambles();

private slots:
	void browse();
	void build();

private:
	Ui::ShamblesClass ui;

};

#endif // __SHAMBLES_H_
