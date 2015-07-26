#ifndef GETCONTACTWIDGET_H
#define GETCONTACTWIDGET_H

#include <QDialog>

namespace Ui {
class GetContactWidget;
}

class GetContactWidget : public QDialog
{
	Q_OBJECT

public:
	explicit GetContactWidget(unsigned int& contactID, char* nickname, unsigned int& nickLen, QWidget *parent = 0);
	~GetContactWidget();

private:
	Ui::GetContactWidget *ui;
	unsigned int* contactID;
	char* nickname;
	unsigned int* nickLen;

private slots:
	void on_contactIDLine_textEdited(const QString &arg1);
	void on_addContactButton_clicked();
	void on_nicknameLine_textEdited(const QString &arg1);
};

#endif // GETCONTACTWIDGET_H
