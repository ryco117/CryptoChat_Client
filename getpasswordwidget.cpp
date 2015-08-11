#include "getpasswordwidget.h"
#include "ui_getpasswordwidget.h"

GetPasswordWidget::GetPasswordWidget(char* passwd, QWidget *parent) :
	QDialog(parent),
	ui(new Ui::GetPasswordWidget)
{
	Passwd = passwd;
	ui->setupUi(this);
	ui->PasswordField->setFocus();

    Qt::WindowFlags flags = this->windowFlags();
    if(flags.testFlag(Qt::WindowContextHelpButtonHint))
        this->setWindowFlags(flags & ~Qt::WindowContextHelpButtonHint);
}

void GetPasswordWidget::SetLabel(QString str)
{
	ui->passLabel->setText(str);
}

void GetPasswordWidget::SetEcho(int e)
{
	ui->PasswordField->setEchoMode(e);
}

void GetPasswordWidget::SetMaxLength(int max)
{
	ui->PasswordField->setMaxLength(max);
}

void GetPasswordWidget::SetPlaceHolder(QString p)
{
	ui->PasswordField->setPlaceholderText(p);
}

void GetPasswordWidget::on_PasswordField_textEdited(const QString &arg1)
{
	QByteArray p = arg1.toUtf8();
	if(p.size() > ui->PasswordField->maxLength())
		ui->PasswordField->setText(QString::fromUtf8(Passwd, strlen(Passwd)));
	else
	{
		memcpy(Passwd, p.data(), p.size());
		memset(&Passwd[p.size()], 0, (ui->PasswordField->maxLength() + 1) - p.size());
	}

	memset(p.data(), 0, p.size());
	p.clear();
}

GetPasswordWidget::~GetPasswordWidget()
{
	int n = ui->PasswordField->text().size();
	for(int i = 0; i < n; i++)
	{
		ui->PasswordField->text()[i] = '\0';
	}
	delete ui;
}