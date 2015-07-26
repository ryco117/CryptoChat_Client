#ifndef GETPASSWORDWIDGET_H
#define GETPASSWORDWIDGET_H

#include <QDialog>

namespace Ui {
class GetPasswordWidget;
}

class GetPasswordWidget : public QDialog
{
	Q_OBJECT

public:
	explicit GetPasswordWidget(char* passwd, QWidget *parent = 0);
	void SetLabel(QString str);
	void SetEcho(int e);
	void SetMaxLength(int max);
	void SetPlaceHolder(QString p);
	~GetPasswordWidget();

private:
	Ui::GetPasswordWidget *ui;
	char* Passwd;

private slots:
	void on_PasswordField_textEdited(const QString &arg1);
};

#endif // GETPASSWORDWIDGET_H
