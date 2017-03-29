#include "shambles.hpp"
#include "builder.hpp"
#include <QFileDialog>
#include <qmessagebox.h>
#include <iostream>
#include <sstream>

#define PE_FILES "PE files (*.exe *.dll)"

Shambles::Shambles(QWidget *parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);

	connect(ui.toolButton, SIGNAL(released()), this, SLOT(browse()));
	connect(ui.pushButton, SIGNAL(released()), this, SLOT(build()));
}

Shambles::~Shambles()
{
	
}

void Shambles::browse()
{
	/* retrieve a file */
	QString filename = QFileDialog::getOpenFileName(this,
		tr("Open File"), "",
		tr(PE_FILES));
	
	if (!filename.isEmpty()) {
		ui.lineEdit->setText(filename);
		ui.pushButton->setEnabled(true);
	}
}

void Shambles::build()
{
	/* check that the file still exists in the path*/
	QString filepath_qstr(ui.lineEdit->text());
	while (!QFileInfo::exists(filepath_qstr))
	{
		QMessageBox::StandardButton reply = QMessageBox::warning(this, "Warning", "The specified file does not exist anymore. Click 'Retry' to try to read from the same location.", QMessageBox::Retry | QMessageBox::Cancel);
		if (reply == QMessageBox::Retry) continue;
		else return;
	}
	QByteArray filepath_qba = filepath_qstr.toLatin1();
	const char *filepath = filepath_qba.data();

	/* redirect stdout and wstdout for logging purposes */
	std::wstringstream wbuffer;
	std::wstreambuf *wcoutbuf = std::wcout.rdbuf();
	std::wcout.rdbuf(wbuffer.rdbuf());


	/* start building new file */
	BuildConfigurations bc;
	Build(filepath, bc);

	/* logging */
	QString qbuffer = QString::fromStdWString(wbuffer.str());
	ui.textEdit->append(qbuffer);
	std::wcout.rdbuf(wcoutbuf);
}