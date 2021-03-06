#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMessageBox>
#include <QTimer>
#include <QFileDialog>
#include <QTextEdit>
#include <QDebug>
#include <QGridLayout>
#include <QSpacerItem>
#include <QListWidget>
#include <QListWidgetItem>
#include <QTableWidget>
#include <QTableWidgetItem>

#include <iostream>
#include "client.h"
#include "donatewindow.h"
#include "getpasswordwidget.h"
#include "getcontactwidget.h"

namespace Ui
{
	class MainWindow;
}

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow(QWidget* parent = 0);
	~MainWindow();

private:
	Client* client;
	Ui::MainWindow* ui;
	QTimer timer;
	QMessageBox* msgBox;
    QColor outgoingColor;
    QColor incomingColor;
	QList<QAction*> contactsContextMenu;
	int contactIndex;
	QList<QAction*> convsContextMenu;
	int selectedConvIndex;
	int openConvIndex;
	int err;
	bool localChanges;

	void CreateActions();
	bool ConnectToServer();
	void RefreshContacts();
	void RefreshConvs();
	void RefreshMessages();
	void View(QWidget* w);
    int DisplayMsg(QString title, QString msg, QMessageBox::Icon level = QMessageBox::Information, QMessageBox::StandardButton buttons = QMessageBox::Ok);
	void DisplayClientError();
    void AppendMsg(const Msg* msg);

private slots:
    //Weird Slots (Don't belong in other catagories completely)
	void Update();
	void OpenConvWithContact(QListWidgetItem* item);
	void OpenConv(QListWidgetItem* item);
    void CreateConv();

	//Right Click Slots
	void userConvListMenuRequested(const QPoint&);
	void userContactListMenuRequested(const QPoint&);

	//Contact Right Click Menu
	void GetContactPublicKey();
	void UpdateNickname();
	void SetContactPublicKey();
	void RemoveContact();

	//Conversation Right Click Menu
	void AddContactToConv();
	void GetMembers();
	void LeaveConv();

	//Menu Bar Actions
	void LoginAction();
	void Create_AccountAction();
	void LogoutAction();
	void GetPublicKeyAction();
	void Sync();
    void UpdatePasswordAction();
	void View_ContactsAction();
	void Add_ContactAction();
	void ConversationsAction();
	void NetworkingAction();
	void GetServerPublicKeyAction();
	void ClientOptionsAction();
	void Help();
	void About();
	void Donate();
	void License();
	void CurveLicense();
    void QtLicense();

	//Buttons, Line Edits, Check Boxes
	void on_passwordLine_returnPressed();
	void on_loginButton_clicked();
	void on_createUserButton_clicked();
	void on_serverAddrLine_returnPressed();
	void on_proxyCB_toggled(bool checked);
	void on_proxyAddrLine_returnPressed();
	void on_proxyConnectButton_clicked();
	void on_messageLineEdit_returnPressed();
	void on_enableAdvancedCB_toggled(bool checked);
	void on_threadsSlider_valueChanged();
};

#endif // MAINWINDOW_H