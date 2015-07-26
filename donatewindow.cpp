#include "donatewindow.h"
#include "ui_donatewindow.h"

DonateWindow::DonateWindow(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::DonateWindow)
{
	ui->setupUi(this);
}

DonateWindow::~DonateWindow()
{
	delete ui;
}