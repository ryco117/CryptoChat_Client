#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget* parent) :QMainWindow(parent), ui(new Ui::MainWindow)
{
	ui->setupUi(this);
	client = new Client();
	localChanges = false;
	timer.setInterval(200);												//Send an event every 200 ms after login
	connect(&timer, SIGNAL(timeout()), this, SLOT(Update()));

	CreateActions();
	ui->actionLogout->setVisible(false);
	ui->actionGet_Public_Key->setVisible(false);
	ui->actionSync->setVisible(false);

	View(ui->networkingWidget);

	ConnectToServer();
	this->resize(QSize(500, 400));

	//Set Tab Order
	setTabOrder(ui->userLine, ui->passwordLine);
	setTabOrder(ui->passwordLine, ui->loginButton);
	setTabOrder(ui->loginButton, ui->createUserButton);

	//Set Up Right Click Menus
	convsContextMenu.push_back(ui->actionLeaveConv);
	openConvIndex = -1;
	selectedConvIndex = -1;
	ui->conversationListWidget->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(ui->conversationListWidget, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(userConvListMenuRequested(QPoint)));
	connect(ui->conversationListWidget, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(OpenConv(QListWidgetItem*)));

	contactsContextMenu.push_back(ui->actionContactPublicKey);
	contactsContextMenu.push_back(ui->actionUpdateNickname);
	contactsContextMenu.push_back(ui->actionRemoveContact);
	contactIndex = -1;
	ui->contactListWidget->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(ui->contactListWidget, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(userContactListMenuRequested(const QPoint&)));
	connect(ui->contactListWidget, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(OpenConvWithContact(QListWidgetItem*)));

	if(client->ServerConnected())
		View(ui->loginWidget);
}

void MainWindow::CreateActions()
{
	//Menu Bar Actions
	connect(ui->actionLogin, SIGNAL(triggered()), this, SLOT(LoginAction()));
	connect(ui->actionCreate_Account, SIGNAL(triggered()), this, SLOT(Create_AccountAction()));
	connect(ui->actionLogout, SIGNAL(triggered()), this, SLOT(LogoutAction()));
	connect(ui->actionGet_Public_Key, SIGNAL(triggered()), this, SLOT(GetPublicKeyAction()));

	connect(ui->actionView_Contacts, SIGNAL(triggered()), this, SLOT(View_ContactsAction()));
	connect(ui->actionAdd_Contact, SIGNAL(triggered()), this, SLOT(Add_ContactAction()));
	connect(ui->addContactButton, SIGNAL(clicked()), this, SLOT(Add_ContactAction()));

	connect(ui->actionConversations, SIGNAL(triggered()), this, SLOT(ConversationsAction()));
	connect(ui->actionSync, SIGNAL(triggered()), this, SLOT(Sync()));

	connect(ui->actionNetworking, SIGNAL(triggered()), this, SLOT(NetworkingAction()));
	connect(ui->actionClient, SIGNAL(triggered()), this, SLOT(ClientOptionsAction()));

    connect(ui->actionHelp, SIGNAL(triggered()), this, SLOT(Help()));
    connect(ui->actionAbout, SIGNAL(triggered()), this, SLOT(About()));
	connect(ui->actionLicense, SIGNAL(triggered()), this, SLOT(License()));
	connect(ui->actionCurve_License, SIGNAL(triggered()), this, SLOT(CurveLicense()));
	connect(ui->actionDonate, SIGNAL(triggered()), this, SLOT(Donate()));

	//Convs Context Menu
	connect(ui->actionAddToConv, SIGNAL(triggered()), this, SLOT(AddContactToConv()));
	connect(ui->actionLeaveConv, SIGNAL(triggered()), this, SLOT(LeaveConv()));

	//Contacts Context Menu
	connect(ui->actionContactPublicKey, SIGNAL(triggered()), this, SLOT(GetContactPublicKey()));
	connect(ui->actionUpdateNickname, SIGNAL(triggered()), this, SLOT(UpdateNickname()));
	connect(ui->actionRemoveContact, SIGNAL(triggered()), this, SLOT(RemoveContact()));
}

bool MainWindow::ConnectToServer()
{
	client->useProxy = ui->proxyCB->isChecked();
	if(!client->SetServer(ui->serverAddrLine->text().toStdString()))
	{
		DisplayMsg("Error", "Incorrectly formatted server address", QMessageBox::Critical, QMessageBox::Ok);
		return false;
	}
	if(client->useProxy)
	{
		if(!client->SetProxy(ui->proxyAddrLine->text().toStdString()))
		{
			DisplayMsg("Error", "Incorrectly formatted proxy address", QMessageBox::Critical, QMessageBox::Ok);
			return false;
		}
	}

	err = client->ConnectToServer();
	if(err != 0)
	{
		client->Disconnect();
		DisplayMsg("Couldn't Connect To Server", tr("Could not connect to server at ") + tr(client->serverAddr.c_str())+ tr("<br/>Error: ") + QString::number(err) + tr(client->GetError().c_str()),\
				   QMessageBox::Critical, QMessageBox::Ok);
		return false;
	}

	return true;
}


void MainWindow::RefreshContacts()
{
	ui->contactListWidget->clear();
	for(unsigned int i = 0; i < client->contacts.size(); i++)
	{
		QListWidgetItem* item = new QListWidgetItem((client->contacts[i].HasNickname())?tr(client->contacts[i].GetNickname()):QString::number(client->contacts[i].GetContactID()),\
							ui->contactListWidget);
		item->setIcon(QIcon(":/new/img/contact.png"));
	}
}

void MainWindow::RefreshConvs()
{
	ui->conversationListWidget->clear();
	for(unsigned int i = 0; i < client->conversations.size(); i++)
	{
		//cout << "Adding conversation ID " << client->conversations[i].GetConvID() << "\n";
		QListWidgetItem* item = new QListWidgetItem(QString::number(client->conversations[i].GetConvID()), ui->conversationListWidget);
		item->setIcon((client->conversations[i].GetUsersNum() > 2)?QIcon(":/new/img/conv_multiple.png"):QIcon(":/new/img/conv_single.png"));
	}
}

void MainWindow::RefreshMessages()
{
	ui->messagesListWidget->clear();
	if(openConvIndex == -1)
		return;

	std::vector<Msg> msgs = client->conversations[openConvIndex].GetMsgs();
	for(unsigned int i = 0; i < msgs.size(); i++)
	{
		new QListWidgetItem(msgs[i].msg, ui->messagesListWidget);
	}
}

void MainWindow::View(QWidget* w)
{
	QObjectList l = ui->centralWidget->children();
	for(int i = 0; i < l.size(); i++)
	{
		if(l[i]->isWidgetType())
			((QWidget*)l[i])->hide();
	}
	w->show();
}

int MainWindow::DisplayMsg(QString title, QString msg, QMessageBox::Icon level, QMessageBox::StandardButton buttons)
{
	msgBox = new QMessageBox(this);
	msgBox->setWindowTitle(title);
    msgBox->setText(msg);

	msgBox->setTextFormat(Qt::RichText);
	msgBox->setTextInteractionFlags(Qt::TextBrowserInteraction);

    msgBox->setIcon(level);
    msgBox->setStandardButtons(buttons);
    int r = msgBox->exec();
    msgBox->deleteLater();
	return r;
}

void MainWindow::DisplayClientError()
{
	DisplayMsg("Error", client->GetError().c_str(), QMessageBox::Critical, QMessageBox::Ok);
}

//SLOTS START HERE!!
//--------------------------------------------------------------------------------------------

void MainWindow::Update()
{
	if(!client->SignedIn())
		return;

	if(client->DataWaiting())
	{
		if(!client->ReceiveData())
			DisplayClientError();

		RefreshMessages();
	}
	timer.start();
}

void MainWindow::OpenConvWithContact(QListWidgetItem* item)
{
	//If conversation with only contact exists, open it, else, ask if create conversation
	contactIndex = item->listWidget()->currentRow();
	unsigned int contactID = client->contacts[contactIndex].GetContactID();
	int convWithUser = -1;
	for(int i = 0; i < (int)client->conversations.size(); i++)
	{
		std::vector<uint32_t> convUsers = client->conversations[i].GetUsers();
		if(convUsers.size() == 2)
		{
			if(convUsers[0] == contactID || convUsers[1] == contactID)
			{
				convWithUser = i;
				break;
			}
		}
	}

	if(convWithUser != -1)
	{
		QListWidgetItem* conv = ui->conversationListWidget->item(convWithUser);
		ui->conversationListWidget->setCurrentRow(convWithUser);
		OpenConv(conv);
	}
	else
	{
		int r = DisplayMsg("Create Conversation", "Create conversation with contact?", QMessageBox::Information, QMessageBox::Yes | QMessageBox::No);
		if(r == QMessageBox::Yes)
		{
			CreateConv();
		}
	}
}

void MainWindow::OpenConv(QListWidgetItem* item)
{
	View(ui->messagingWidget);
	openConvIndex = item->listWidget()->currentRow();
	ui->messageLineEdit->setFocus();
	RefreshMessages();
	return;
}

void MainWindow::CreateConv()
{
	if(contactIndex == -1)
		return;

	//Create conversation with contact
	uint32_t convID = client->CreateConversation(client->contacts[contactIndex].GetContactID());
	if(convID == 0)
		DisplayClientError();
	/*
	else
		DisplayMsg("Success!", tr("Conversation ") + QString::number(convID) +tr(" was successfully created!"), QMessageBox::Information, QMessageBox::Ok);/**/

	RefreshConvs();
	contactIndex = -1;
}

void MainWindow::userConvListMenuRequested(const QPoint& pos)
{
	QPoint globalPos = ui->conversationListWidget->mapToGlobal(pos);	//Map the global position to the userlist
	QModelIndex t = ui->conversationListWidget->indexAt(pos);
	if(t.isValid())
	{
		convsContextMenu.clear();
		if(client->conversations[t.row()].GetCreatorID() == client->GetUserID())
			convsContextMenu.push_front(ui->actionAddToConv);
		convsContextMenu.push_back(ui->actionLeaveConv);

		ui->conversationListWidget->item(t.row())->setSelected(true);	//even a right click will select the item
		selectedConvIndex = t.row();
		QMenu::exec(convsContextMenu, globalPos);
	}
}

void MainWindow::userContactListMenuRequested(const QPoint& pos)
{
	QPoint globalPos = ui->contactListWidget->mapToGlobal(pos);	//Map the global position to the userlist
	QModelIndex t = ui->contactListWidget->indexAt(pos);
	if(t.isValid())
	{
		ui->contactListWidget->item(t.row())->setSelected(true);	//even a right click will select the item
		contactIndex = t.row();
		QMenu::exec(contactsContextMenu, globalPos);
	}
}

void MainWindow::GetContactPublicKey()
{
	if(contactIndex == -1)
		return;

	char* pubKey = Base64Encode(client->contacts[contactIndex].GetPublicKey(), 32);
	DisplayMsg("Public Key", pubKey, QMessageBox::Information, QMessageBox::Ok);
	delete[] pubKey;
	contactIndex = -1;
}

void MainWindow::UpdateNickname()
{
	if(contactIndex == -1)
		return;

	char nickname[32];
	memset(nickname, 0, 32);
	GetPasswordWidget w(nickname, this);
	w.setWindowTitle("Set Nickname");
	w.SetLabel("Nickname");
	w.SetEcho(QLineEdit::Normal);
	w.SetMaxLength(31);
	w.SetPlaceHolder("Nickname may be empty");
	if(w.exec() == QDialog::Accepted)
	{
		if(!client->UpdateNickname(client->contacts[contactIndex].GetContactID(), nickname, strlen(nickname)))
			DisplayClientError();

		RefreshContacts();
	}
	contactIndex = -1;
}

void MainWindow::RemoveContact()
{
	if(contactIndex == -1)
		return;

	if(DisplayMsg("Remove Contact", "Are you sure you want to remove this contact?", QMessageBox::Warning, QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
	{
		if(!client->RemoveContact(client->contacts[contactIndex].GetContactID()))
			DisplayClientError();

		RefreshContacts();
	}
	contactIndex = -1;
}

void MainWindow::AddContactToConv()
{
	if(selectedConvIndex == -1)
		return;

	char* idStr = new char[21];
	memset(idStr, 0, 21);
	GetPasswordWidget w(idStr, this);
	w.setWindowTitle("Contact ID");
	w.SetLabel("Contact ID to add");
	w.SetEcho(QLineEdit::Normal);
	w.SetMaxLength(20);
	if(w.exec() == QDialog::Accepted)
	{
		stringstream ss;
		int ID = 0;
		ss << idStr;
		ss >> ID;
		if(ID > 0)
		{
			if(!client->AddToConversation(ID, client->conversations[selectedConvIndex].GetConvID()))
				DisplayClientError();
		}

		RefreshConvs();
	}
	delete[] idStr;
	selectedConvIndex = -1;
}

void MainWindow::LeaveConv()
{
	if(selectedConvIndex == -1)
		return;

	uint32_t id = client->conversations[selectedConvIndex].GetConvID();
	if(DisplayMsg("Warning", tr("Are you sure you want to leave conversation ") + QString::number(id)\
				  + tr("?"), QMessageBox::Warning, QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
		return;

	if(!client->LeaveConversation(id))
		DisplayClientError();

	RefreshConvs();
	selectedConvIndex = -1;
}

void MainWindow::LoginAction()
{
	View(ui->loginWidget);
}

void MainWindow::Create_AccountAction()
{
	if(client->SignedIn())
		LogoutAction();

	char* passwd = new char[512];
	memset(passwd, 0, 512);

	GetPasswordWidget w(passwd, this);
	w.setWindowTitle("Choose A Password");
	if(w.exec() == QDialog::Accepted)
	{
		if(passwd[0] != '\0')
		{
			if(!client->ServerConnected())
				ConnectToServer();

			uint32_t userID = client->CreateUser(passwd);
			if(userID)
				DisplayMsg("Success!", tr("Account <b>ID ") + QString::number(userID) +tr("</b> was successfully created!"), QMessageBox::Information, QMessageBox::Ok);
			else
				DisplayClientError();
		}
		else
			DisplayMsg("Error", "Password can not be empty for security!", QMessageBox::Warning, QMessageBox::Ok);
	}
	memset(passwd, 0, 512);
}

void MainWindow::LogoutAction()
{
	client->Disconnect();
	timer.stop();
	ui->actionLogout->setVisible(false);
	ui->actionGet_Public_Key->setVisible(false);
	ui->actionSync->setVisible(false);
	this->setWindowTitle("CryptoChat Client");

	ui->contactListWidget->clear();
	client->contacts.clear();
	ui->conversationListWidget->clear();
	ui->messagesListWidget->clear();
	client->conversations.clear();

	View(ui->loginWidget);
}

void MainWindow::GetPublicKeyAction()
{
	char* pubKey = Base64Encode(client->GetPublicKey(), 32);
	DisplayMsg("Public Key", pubKey, QMessageBox::Information, QMessageBox::Ok);
	delete[] pubKey;
}

void MainWindow::Sync()
{
	if(localChanges)
	{
		if(DisplayMsg("Warning", "Syncing with server <b>will overwrite</b> your local changes to contact public keys.<br/>Continue?",\
				   QMessageBox::Warning, QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
			return;
	}

	//Clear UI as well as contact
	ui->contactListWidget->clear();
	client->contacts.clear();
	ui->conversationListWidget->clear();
	ui->messagesListWidget->clear();
	client->conversations.clear();

	if(!client->FetchContacts())
	{
		DisplayMsg("Error", client->GetError().c_str(), (client->GetError() == "No contacts added")?QMessageBox::Information:QMessageBox::Critical, QMessageBox::Ok);
		return;
	}
	if(client->contacts.size() > 0)
		RefreshContacts();

	if(!client->FetchConversations())
	{
		DisplayMsg("Error", client->GetError().c_str(), (client->GetError() == "No conversations created")?QMessageBox::Information:QMessageBox::Critical, QMessageBox::Ok);
		return;
	}
	if(client->conversations.size() > 0)
		RefreshConvs();

	if(!client->FetchMessages())
	{
		DisplayClientError();
		return;
	}
	return;
}

void MainWindow::View_ContactsAction()
{
	View(ui->contactsWidget);
}

void MainWindow::Add_ContactAction()
{
	if(client->SignedIn())
	{
		uint32_t contactID;
		char* nickname = new char[32];
		memset(nickname, 0, 32);
		uint32_t nickLen;

		GetContactWidget w(contactID, nickname, nickLen, this);
		w.setWindowTitle("Contact");
		if(w.exec() == QDialog::Accepted)
		{
			if(!client->AddUserToContacts(contactID, nickname, nickLen))
				DisplayClientError();

			RefreshContacts();
		}
	}
	else
	{
		DisplayMsg("Error", "Action is not possible until signed in", QMessageBox::Critical, QMessageBox::Ok);
	}
}

void MainWindow::ConversationsAction()
{
	View(ui->convsWidget);
}

void MainWindow::NetworkingAction()
{
	View(ui->networkingWidget);
}

void MainWindow::ClientOptionsAction()
{
	View(ui->clientOptionsWidget);
}

void MainWindow::Help()
{
    msgBox = new QMessageBox(this);
	DisplayMsg("Help", "Don't understand how all this encryption stuff fits togther?<br/>\
					   <b>Consult the book of knowledge!</b><br/>\
					   <a href=\"http://en.wikipedia.org/wiki/Public-key_cryptography\">Public Key Cryptography</a><br/>\
					   <a href=\"http://en.wikipedia.org/wiki/Elliptic_curve_Diffie%E2%80%93Hellman\">ECDH</a><br/>\
					   <a href=\"http://en.wikipedia.org/wiki/Symmetric-key_algorithm\">Symmetric Key Cryptography</a><br/>\
					   <a href=\"http://en.wikipedia.org/wiki/Advanced_Encryption_Standard\">AES</a><br/>\
					   <a href=\"http://en.wikipedia.org/wiki/Scrypt\">Scrypt</a><br/><br/>\
					   Still have questions? Maybe I'll make a website explaining usage in detail... Someday...",\
			   QMessageBox::Information, QMessageBox::Ok);
}

void MainWindow::About()
{
	DisplayMsg("About", "TESTING 123!", QMessageBox::Information, QMessageBox::Ok);
}

void MainWindow::Donate()
{
	DonateWindow w(this);
	w.setWindowTitle("Donate");
	w.exec();
}

void MainWindow::License()
{
    DisplayMsg("License", "Copyright (C) 2015  Ryan Andersen<br/>\
						 <br/>\
												This program is free software: you can redistribute it and/or modify<br/>\
												it under the terms of the GNU General Public License as published by<br/>\
												the Free Software Foundation, either version 3 of the License, or<br/>\
												(at your option) any later version.<br/>\
						 <br/>\
												This program is distributed in the hope that it will be useful,<br/>\
												but WITHOUT ANY WARRANTY; without even the implied warranty of<br/>\
												MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the<br/>\
												GNU General Public License for more details.<br/>\
						 <br/>\
												You should have received a copy of the GNU General Public License<br/>\
												along with this program.  If not, see <a href=\"http://www.gnu.org/licenses/\">&lt;http://www.gnu.org/licenses/&gt;</a>.",\
			   QMessageBox::Information, QMessageBox::Ok);
}

void MainWindow::CurveLicense()
{
    msgBox = new QMessageBox(this);
	msgBox->setWindowTitle("curve25519-donna License");
    msgBox->setText(tr("Copyright 2008, Google Inc.<br/>\
						All rights reserved.<br/>\
						<br/>\
						Redistribution and use in source and binary forms, with or without\
						modification, are permitted provided that the following conditions are\
						met:\
						<p style=\"text-indent: 50px\">* Redistributions of source code must retain the above copyright\
						notice, this list of conditions and the following disclaimer.</p>\
						<p style=\"text-indent: 50px\">* Redistributions in binary form must reproduce the above\
						copyright notice, this list of conditions and the following disclaimer\
						in the documentation and/or other materials provided with the\
						distribution.</p>\
						<p style=\"text-indent: 50px\">* Neither the name of Google Inc. nor the names of its\
						contributors may be used to endorse or promote products derived from\
						this software without specific prior written permission.</p>\
						THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS\
						\"AS IS\" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT\
						LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR\
						A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT\
						OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,\
						SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT\
						LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,\
						DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY\
						THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT\
						(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE\
						OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.<br/>\
						<br/>\
						curve25519-donna: Curve25519 elliptic curve, public key function<br/>\
						<a href=\"http://code.google.com/p/curve25519-donna\">http://code.google.com/p/curve25519-donna/</a><br/>\
						Adam Langley &lt;agl@imperialviolet.org&gt;<br/>\
						<br/>\
						Derived from public domain C code by Daniel J. Bernstein &lt;djb@cr.yp.to&gt;<br/>\
						More information about curve25519 can be found here<br/>\
						<a href=\"http://cr.yp.to/ecdh.html\">http://cr.yp.to/ecdh.html</a><br/>\
						<br/>\
						djb's sample implementation of curve25519 is written in a special assembly\
						language called qhasm and uses the floating point registers.<br/>\
						<br/>\
						This is, almost, a clean room reimplementation from the curve25519 paper. It\
						uses many of the tricks described therein. Only the crecip function is taken\
						from the sample implementation."));
	msgBox->setTextFormat(Qt::RichText);
    msgBox->setIcon(QMessageBox::Information);
    msgBox->setStandardButtons(QMessageBox::Ok);
	QSpacerItem* horizontalSpacer = new QSpacerItem(700, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);
	QGridLayout* lay = (QGridLayout*)msgBox->layout();
	lay->addItem(horizontalSpacer, lay->rowCount(), 0, 1, lay->columnCount());
    msgBox->exec();
    msgBox->deleteLater();
}

//BUTTONS HERE!!!
//-----------------------------------------------------------------------------------------------------------

void MainWindow::on_passwordLine_returnPressed()
{
	on_loginButton_clicked();
}

void MainWindow::on_loginButton_clicked()
{
	if(client->SignedIn())
		LogoutAction();

	unsigned int userID = ui->userLine->text().toLong();
	if(userID > 0)
	{
		if(!client->ServerConnected())
			ConnectToServer();

		const char* pass = ui->passwordLine->text().toUtf8().data();
		if(client->Login(userID, pass))
		{
			this->setWindowTitle("CryptoChat Client [Signed In]");
			ui->actionLogout->setVisible(true);
			ui->actionGet_Public_Key->setVisible(true);
			ui->actionSync->setVisible(true);

			if(ui->autoSyncCB->isChecked())
				Sync();

			timer.start();	//Start timer now!
			View(ui->contactsWidget);
		}
		else
		{
			DisplayClientError();
		}
	}
	else
		DisplayMsg("Error", "Incorrectly formatted user ID", QMessageBox::Critical, QMessageBox::Ok);
}

void MainWindow::on_createUserButton_clicked()
{
	Create_AccountAction();
}

void MainWindow::on_serverAddrLine_returnPressed()
{
	if(client->ServerConnected())
	{
		if(client->SignedIn())
			timer.stop();
		client->Disconnect();
	}

	ConnectToServer();
	if(client->ServerConnected())
		View(ui->loginWidget);
}

void MainWindow::on_proxyCB_toggled(bool checked)
{
	ui->proxyAddrLine->setEnabled(checked);
	ui->proxyConnectButton->setEnabled(checked);
	client->useProxy = true;
}

void MainWindow::on_proxyAddrLine_returnPressed()
{
	on_proxyConnectButton_clicked();
}

void MainWindow::on_proxyConnectButton_clicked()
{
	on_serverAddrLine_returnPressed();
}

void MainWindow::on_messageLineEdit_returnPressed()
{
	QByteArray q = ui->messageLineEdit->text().toUtf8();
	if(q.size() > 0 && q.size() < 4096)						//highest server will accept for one message
	{
		uint32_t id = client->conversations[openConvIndex].GetConvID();
		if(client->SendMessage(id, q.data(), q.size()))
			ui->messageLineEdit->clear();
		else
			DisplayClientError();

		RefreshMessages();
	}
}

MainWindow::~MainWindow()
{
	ui->messagesListWidget->clear();
	ui->contactListWidget->clear();
	ui->conversationListWidget->clear();

	if(client->ServerConnected())
	{
		if(client->SignedIn())
			timer.stop();

		cout << "Safe disconnect\n";
		client->Disconnect();
	}
	delete ui;
}