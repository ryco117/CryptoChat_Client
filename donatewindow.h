#ifndef DONATEWINDOW_H
#define DONATEWINDOW_H

#include <QDialog>

namespace Ui {
class DonateWindow;
}

class DonateWindow : public QDialog
{
	Q_OBJECT

public:
	explicit DonateWindow(QWidget *parent = 0);
	~DonateWindow();

private:
	Ui::DonateWindow *ui;
};

#endif // DONATEWINDOW_H