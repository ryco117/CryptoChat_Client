#ifndef CLIENT_H
#define CLIENT_H

#define SCRYPT_WORK_VALUE 131072

#ifdef WINDOWS
	#define _WIN32_WINNT  0x0500
	#pragma comment(lib, "Ws2_32.lib")

	#include <winsock2.h>
	#include <Ws2tcpip.h>
	#include <windows.h>
	#include <Ntsecapi.h>
#else
	#include <sys/socket.h>
	#include <unistd.h>
	#include <arpa/inet.h>
	#include <netdb.h>
	#include "CloseSocket.cpp"
#endif

#include <stdint.h>
#include <string>
#include <vector>
#include <string.h>

#include "Contact.cpp"
#include "Conversation.cpp"

#include "crypto/fortuna.h"
#include "crypto/ecdh.h"
#include "crypto/base64.cpp"

class Client
{
private:
	unsigned char zeroArray[48];
	bool seedSuccessfull;
	FortunaPRNG fprng;
	AES crypt;
	fd_set master;
	fd_set read_fds;
	unsigned int fdmax;
	timeval wait;

	//Crypto Keys and User Info
	uint8_t	servPublic[32];
	uint8_t encPrivate[48];
	uint8_t userPrivate[32];
	uint8_t	userPublic[32];
	uint8_t userIV[16];
	uint8_t userSalt[16];
	uint8_t userPasswordKey[32];
	uint32_t userID;
	uint32_t rand;
	uint32_t convID;
	uint32_t contactID;

	//Networking
	int Server;														//Socket for connecting to server
	sockaddr_in socketInfo;
	bool serverConnected;
	bool signedIn;

	//Vars that most functions will use
	int err;
	std::string errStr;
	char buffer[4096];
	uint8_t info;
	int size;

public:
	std::vector<Contact> contacts;
	std::vector<Conversation> conversations;

	std::string serverAddr;
	uint16_t serverPort;
	bool useProxy;
	std::string proxyAddr;
	uint16_t proxyPort;

	Client();
	~Client();
	std::string GetError();
	bool SeedSuccessfull();
	bool ServerConnected();
	bool SignedIn();
	bool SetServer(std::string server);
	bool SetProxy(std::string proxy);
	unsigned int GetUserID();
	const char* GetPublicKey();

	//Interface With Server
	int ConnectToServer();
	bool DataWaiting();
	bool ReceiveData();
	void Disconnect();
	bool GetServersPublicKey();
	unsigned int CreateUser(const char* password);
	bool GetInfo(uint8_t info_arg = 0);
	bool Login(unsigned int userID, const char* password);
	bool GetUsersPublicKey(uint32_t contactID);
	bool AddUserToContacts(uint32_t contactID, const char* nickName = 0, unsigned int nickLen = 0);
	unsigned int CreateConversation(unsigned int contactID);
	bool AddToConversation(unsigned int contactID, unsigned int convID);
	bool SendMessage(uint32_t convID, const char* msg, unsigned int msgLen);
	bool FetchContacts();
	bool RemoveContact(uint32_t contactID);
	bool LeaveConversation(uint32_t convID);
	bool FetchConversations();
	bool FetchMessages();
	bool UpdateNickname(uint32_t contactID, const char* nickName = 0, unsigned int nickLen = 0);

private:
	int GetConvIndex(uint32_t convID);
	int GetContactIndex(uint32_t contactID);
	bool SeedPRNG(FortunaPRNG& fprng);
	int recvr(int socket, char* buffer, int length, int flags);
};

#endif // CLIENT_H