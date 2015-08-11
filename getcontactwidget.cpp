#include "getcontactwidget.h"
#include "ui_getcontactwidget.h"

GetContactWidget::GetContactWidget(unsigned int& contactID, char* nickname, unsigned int& nickLen, QWidget *parent = 0) :
	QDialog(parent),
	ui(new Ui::GetContactWidget)
{
	ui->setupUi(this);
	this->contactID = &contactID;
	this->nickname = nickname;
	this->nickLen = &nickLen;
	ui->contactIDLine->setFocus();

    Qt::WindowFlags flags = this->windowFlags();
    if(flags.testFlag(Qt::WindowContextHelpButtonHint))
        this->setWindowFlags(flags & ~Qt::WindowContextHelpButtonHint);
}

void GetContactWidget::on_contactIDLine_textEdited(const QString &arg1)
{
	bool ok = false;
	arg1.toLong(&ok, 10);
	QByteArray q = ui->nicknameLine->text().toUtf8();
	if(ok && q.size() < 32)
	{
		ui->addContactButton->setEnabled(true);
	}
	else
		ui->addContactButton->setEnabled(false);
}

void GetContactWidget::on_nicknameLine_textEdited(const QString &arg1)
{
	on_contactIDLine_textEdited(ui->contactIDLine->text());
}

void GetContactWidget::on_addContactButton_clicked()
{
	*contactID = ui->contactIDLine->text().toLong();
	QByteArray q = ui->nicknameLine->text().toUtf8();
	int size = q.size();
	if(size > 0)
	{
		memcpy(nickname, q.data(), size);
		*nickLen = size;
	}
	else
	{
		nickname[0] = '\0';
		*nickLen = 0;
	}
	this->accept();
}

GetContactWidget::~GetContactWidget()
{
	delete ui;
}
