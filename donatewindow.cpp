#include "donatewindow.h"
#include "ui_donatewindow.h"

DonateWindow::DonateWindow(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::DonateWindow)
{
	ui->setupUi(this);

    Qt::WindowFlags flags = this->windowFlags();
    if(flags.testFlag(Qt::WindowContextHelpButtonHint))
        this->setWindowFlags(flags & ~Qt::WindowContextHelpButtonHint);
}

DonateWindow::~DonateWindow()
{
	delete ui;
}