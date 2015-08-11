#include "client.h"

template <class T> std::vector<T> DropIndex(std::vector<T> v, unsigned int i);
bool IsIP(string& IP);
u_long Resolve(string& addr);
int recvr(int socket, char* buffer, int length, int flags);
int connect_timeout(int fd, const sockaddr* addr, socklen_t len, timeval timeout);

struct HashInfo
{
	bool* found;
	char* result;
	const char* match;
	const char* salt;
	unsigned int n;
	long long start;
	unsigned int inc;
};
void* FindHash(void* v);

Client::Client()
{
	memset(zeroArray, 0, 48);
	err = 0;
	seedSuccessfull = false;
	if(!SeedPRNG(fprng))											//If it's not secure, kill it!!
		return;
	seedSuccessfull = true;

	#ifdef WINDOWS
		//Why Windows needs this, I don't know... Why can't everthing just be a file???
		WSADATA wsaData;
		int error = WSAStartup(0x0202, &wsaData); //start and fill results into wsaData and output error
		if(error)
		{
			err = -1;
			return;					//Something went wrong
		}
		if(wsaData.wVersion != 0x0202)
		{
			err = -2;
			WSACleanup();			//Wrong wsaData version(not 2.2)
			return;
		}
	#endif

	//DEFAULTS
	FD_ZERO(&master);
	fdmax = 0;
	wait.tv_sec = 0;
	wait.tv_usec = 1000;
	serverConnected = false;
	signedIn = false;
	info = 0;

	userID = 0;
	memset(servPublic, 0, 32);
	memset(encPrivate, 0, 48);
	memset(userPrivate, 0, 32);
	memset(userPublic, 0, 32);
	memset(userPasswordKey, 0, 32);
	memset(userIV, 0, 16);
	memset(userSalt, 0, 16);
	threadNum = 2;

	rand = 0;
	Server = 0;
	serverAddr = "127.0.0.1";
	serverPort = 19486;
	useProxy = false;
	proxyAddr = "127.0.0.1";
	proxyPort = 5057;

    updateMessages = false;
    msgsConvIndex = -1;
    updateConvs = false;
    updateContacts = false;

	//READ A SETTINGS CONFIG HERE
	//read.settings();
}

bool Client::SeedPRNG(FortunaPRNG& fprng)
{
	//Properly Seed
	uint32_t* seed = new uint32_t[20];
	#ifdef WINDOWS
		RtlGenRandom(seed, sizeof(uint32_t) * 20);
	#else
		FILE* random;
		random = fopen ("/dev/urandom", "r");						//Unix provides it, why not use it
		if(random == NULL)
		{
			fprintf(stderr, "Cannot open /dev/urandom!\n");			//THIS IS BAD!!!!
			delete[] seed;
			return false;
		}
		for(int i = 0; i < 20; i++)
		{
			fread(&seed[i], sizeof(uint32_t), 1, random);
			srand(seed[i]); 		//seed the default random number generator
		}
		fclose(random);
	#endif
	fprng.Seed((unsigned char*)seed, sizeof(uint32_t) * 20);
	memset(seed, 0, sizeof(uint32_t) * 20);
	delete[] seed;
	return true;
}

std::string Client::GetError() const
{
	return errStr;
}

bool Client::SeedSuccessfull() const
{
	return seedSuccessfull;
}

bool Client::ServerConnected() const
{
	return serverConnected;
}

bool Client::SignedIn() const
{
	return signedIn;
}

bool Client::UpdateMessages()
{
	bool x = updateMessages;
	updateMessages = false;
	return x;
}

int Client::MsgsConvIndex()
{
    int x = msgsConvIndex;
    msgsConvIndex = -1;
    return x;
}

void Client::ClearMsgFlags()
{
    updateMessages = false;
    msgsConvIndex = -1;
}

bool Client::UpdateConvs()
{
	bool x = updateConvs;
	updateConvs = false;
	return x;
}

bool Client::UpdateContacts()
{
	bool x = updateContacts;
	updateContacts = false;
	return x;
}

bool Client::SetServer(std::string server)
{
	std::size_t i = server.find(":");
	if(i == std::string::npos)
		return false;
	if(server.find(":", i + 1) != std::string::npos)
		return false;

	serverAddr = server.substr(0, i);

	std::string port = server.substr(i + 1);
	if(port.size() == 0)
		return false;
	for(unsigned int j = 0; j < port.size(); j++)
	{
		if(!isdigit(port[j]))
			return false;
	}
	serverPort = atoi(port.c_str());
	return true;
}

bool Client::SetProxy(std::string proxy)
{
	std::size_t i = proxy.find(":");
	if(i == std::string::npos)
		return false;
	if(proxy.find(":", i + 1) != std::string::npos)
		return false;

	proxyAddr = proxy.substr(0, i);

	std::string port = proxy.substr(i + 1);
	if(port.size() == 0)
		return false;
	for(unsigned int j = 0; j < port.size(); j++)
	{
		if(!isdigit(port[j]))
			return false;
	}
	proxyPort = atoi(port.c_str());
	return true;
}

unsigned int Client::GetUserID() const
{
	return userID;
}

const char* Client::GetPublicKey() const
{
	return (const char*)userPublic;
}

const char* Client::GetServPublic() const
{
	return (const char*)servPublic;
}

void Client::SetThreadNum(unsigned char t)
{
	threadNum = t;
	if(threadNum < 1)
		threadNum = 1;
}

//Interface with Server!
//--------------------------------------------------------------------------------------------------------------------------------------

int Client::ConnectToServer()
{
	Server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);				//assign Server to a file descriptor (socket) that uses IP addresses, TCP
	memset(&socketInfo, 0, sizeof(socketInfo));						//Clear socketInfo to be filled with client stuff
	socketInfo.sin_family = AF_INET;								//uses IPv4 addresses

	timeval connectTimeout = {4, 0};
	if(!useProxy)
	{
		if(IsIP(serverAddr))
			socketInfo.sin_addr.s_addr = inet_addr(serverAddr.c_str());
		else
		{
			socketInfo.sin_addr.s_addr = Resolve(serverAddr);
			if(socketInfo.sin_addr.s_addr == 0)
			{
				Disconnect();
				return -1;
			}
		}

		socketInfo.sin_port = htons(serverPort);						//use serverPort
		if((err = connect_timeout(Server, (struct sockaddr*)&socketInfo, sizeof(socketInfo), connectTimeout)) != 0)
		{
			Disconnect();
			return -2;
		}
	}
	else
	{
		if(IsIP(proxyAddr))
			socketInfo.sin_addr.s_addr = inet_addr(proxyAddr.c_str());
		else
		{
			socketInfo.sin_addr.s_addr = Resolve(proxyAddr);
			if(socketInfo.sin_addr.s_addr == 0)
			{
				Disconnect();
				return -3;
			}
		}
		socketInfo.sin_port = htons(proxyPort);						//use serverPort

		if((err = connect_timeout(Server, (struct sockaddr*)&socketInfo, sizeof(struct sockaddr_in), connectTimeout)) != 0)
		{
			Disconnect();
			return -4;
		}

		if(IsIP(serverAddr))
		{
			//SOCKS4 - Assuming no userID is required. Could be modified if becomes relevant
			char reqField[9];
			reqField[0] = 0x04;
			reqField[1] = 0x01;
			uint16_t port = htons(serverPort);
			memcpy(&reqField[2], &port, 2);
			uint32_t serverAddrBytes = inet_addr(serverAddr.c_str());
			memcpy(&reqField[4], &serverAddrBytes, 4);
			reqField[8] = 0;
			send(Server, reqField, 9, 0);
		}
		else
		{
			//SOCKS4a - Assuming no userID is required. Could be modified if becomes relevant
			char* reqField = new char[9 + serverAddr.size() + 1];
			memset(reqField, 0, 9 + serverAddr.size() + 1);

			reqField[0] = 0x04;
			reqField[1] = 0x01;
			uint16_t port = htons(serverPort);
			memcpy(&reqField[2], &port, 2);
			reqField[7] = 0xFF;
			memcpy(&reqField[9], serverAddr.c_str(), serverAddr.size());
			send(Server, reqField, 9 + serverAddr.size() + 1, 0);
			delete[] reqField;
		}

		unsigned char recvField[8];
		int nbytes = recv(Server, recvField, 8, 0);
		if(nbytes <= 0)
		{
			Disconnect();
			return -5;
		}
		if(recvField[0] != 0)
		{
			Disconnect();
			return -6;
		}
		if(recvField[1] != 0x5a)
		{
			Disconnect();
			return -7;
		}
	}
	FD_SET(Server, &master);
	fdmax = Server;
	serverConnected = true;

	if(memcmp(servPublic, zeroArray, 32) == 0)
	{
		GetServersPublicKey();
	}

	return 0;
}

bool Client::DataWaiting()
{
	read_fds = master;
	timeval w = {wait.tv_sec, wait.tv_usec};
	if(select(fdmax+1, &read_fds, 0, 0, &w) == -1)
	{
		return false;
	}

	return FD_ISSET(Server, &read_fds);
}

bool Client::ReceiveData()
{
	uint8_t type = buffer[0];
	int bytes = recvr(Server, buffer, 1, 0);
	if(bytes != 1)
	{
		Disconnect();
		errStr = "Lost connection with server";
		return false;
	}
	if(buffer[0] == '\0')
	{
		recvr(Server, &buffer[1], 4, 0);
		uint32_t len = ntohl(*((uint32_t*)&buffer[1]));
		recvr(Server, &buffer[5], len, 0);
		errStr = &buffer[5];
		return false;
	}
	else if(buffer[0] == '\xFF')
	{
		recvr(Server, &buffer[1], 12, 0);
		uint32_t convID = ntohl(*((uint32_t*)&buffer[1]));
		int convIndex = GetConvIndex(convID);
		uint32_t senderID = ntohl(*((uint32_t*)&buffer[9]));
		uint32_t msgLen = ntohl(*((uint32_t*)&buffer[5]));
		if(recvr(Server, &buffer[13], 16 + msgLen, 0) < 16 + (int)msgLen)
		{
			errStr = "Disconnected from server";
			return false;
		}

		if(convIndex == -1)
		{
			errStr = "The received message belongs to a conversation that doesn't exist locally";
			return false;
		}

		char* msg = new char[msgLen];
		int decMsgLen = crypt.Decrypt(&buffer[29], msgLen, (const u8*)&buffer[13], (const u8*)conversations[convIndex].GetSymKey(), msg);
		if(decMsgLen == -1)
		{
			errStr = "Bad decrypt";
			return false;
		}

		conversations[convIndex].AddMsg(senderID, msg, decMsgLen);
		updateMessages = true;
        msgsConvIndex = convIndex;
		delete[] msg;

		//Send "case 13" saying we read this message successfully, and don't need to be pushed it again.
		buffer[0] = '\x0D';
		uint32_t sNet = htonl((uint32_t)(1 + 4 + 4 + 16 + msgLen));			//The extra constants are important to the server
		memcpy(&buffer[5], &sNet, 4);
		send(Server, buffer, 1 + 4 + 4, 0);
		return true;
	}
	else if(buffer[0] == '\x0D')
	{
		return true;
	}
	else if(buffer[0] == '\x06' || buffer[0] == '\x07')
	{
		recvr(Server, buffer, 76, 0);
		convID = ntohl(*((uint32_t*)buffer));
		uint32_t creatorID = ntohl(*((uint32_t*)&buffer[4]));
		char* key = new char[32];
		if(creatorID != userID)
		{
			const char* contactPubKey = contacts[GetContactIndex(creatorID)].GetPublicKey();
			curve25519_donna((u8*)key, userPrivate, (const u8*)contactPubKey);
		}
		else
		{
			memcpy(key, userPrivate, 32);
		}

		if(crypt.Decrypt(&buffer[24], 48, (const u8*)&buffer[8], (const u8*)key, &buffer[24]) != 32)
		{
			Disconnect();
			memset(key, 0, 32);
			delete[] key;
			errStr = "Bad decrypt, aborting connection";
			return false;
		}
		memset(key, 0, 32);
		delete[] key;

		uint32_t users_num = ntohl(*((uint32_t*)&buffer[72]));
		recvr(Server, &buffer[76], users_num * 4, 0);
		for(unsigned int j = 0; j < users_num; j++)
		{
			uint32_t user = ntohl(*((uint32_t*)&buffer[76 + (4 * j)]));
			memcpy(&buffer[76 + (4 * j)], &user, 4);
		}
		conversations.push_back(Conversation(convID, creatorID, (const unsigned char*)&buffer[24], (const unsigned int*)&buffer[76], users_num));
		updateConvs = true;
		return true;
	}
	else
	{
		bool success = true;
		switch(type)
		{
		case 0:
		{
			recvr(Server, &buffer[1], 32, 0);
			memcpy(servPublic, &buffer[1], 32);
			break;
		}
		case 1:
		{
			recvr(Server, &buffer[1], 4, 0);
			userID = ntohl(*((uint32_t*)&buffer[1]));
			break;
		}
		case 2:
		{
			unsigned int read = 1;
			errStr.clear();
			if(info & 8)
			{
				if(recvr(Server, &buffer[read], 4, 0) != 4)
				{
					success = false;
					errStr += "Bad rand recv, ";
				}
				else
				{
					rand = ntohl(*((uint32_t*)&buffer[read]));
				}
				read += 4;
			}
			if(info & 4)
			{
				if(recvr(Server, &buffer[read], 16, 0) != 16)
				{
					success = false;
					errStr += "Bad salt recv, ";
				}
				else
				{
					memcpy(userSalt, &buffer[read], 16);
				}
				read += 16;
			}
			if(info & 2)
			{
				if(recvr(Server, &buffer[read], 16, 0) != 16)
				{
					success = false;
					errStr += "Bad IV recv, ";
				}
				else
				{
					memcpy(userIV, &buffer[read], 16);
				}
				read += 16;
			}
			if(info & 1)						//Moved down because relies on updated IV
			{
				if(recvr(Server, &buffer[read], 48, 0) != 48)
				{
					success = false;
					errStr += "Bad private key recv, ";
				}
				else
				{
					memcpy(encPrivate, &buffer[read], 48);
					if(crypt.Decrypt((const char*)encPrivate, 48, userIV, userPasswordKey, (char*)userPrivate) == -1)
					{
						memset(userPrivate, 0, 32);
						success = false;
						errStr += "Incorrect password";
					}
				}
				read += 48;
			}
			break;
		}
		case 3:
			break;
		case 4:
		{
			recvr(Server, &buffer[1], 32, 0);
			contacts[GetContactIndex(contactID)].SetPublicKey(&buffer[1]);
			break;
		}
		case 5:
			break;		//Contact was added... Lets pat ourselves on the back
		case 6:
		{
			recvr(Server, &buffer[1], 4, 0);
			convID = ntohl(*((uint32_t*)&buffer[1]));
			break;
		}
		case 7:
			break;
		case 9:
		{
			errStr.clear();

			recvr(Server, &buffer[1], 4, 0);
			uint32_t size = ntohl(*((uint32_t*)&buffer[1]));
			unsigned int i = 0;
			while(i < size)
			{
				recvr(Server, &buffer[0], 37, 0);
				contactID = ntohl(*((uint32_t*)buffer));
				unsigned int len = (uint8_t)buffer[36];
				char* nickname = 0;
				unsigned int decLen = 0;
				if(len > 0)
				{
					recvr(Server, &buffer[37], len, 0);
					nickname = new char[len];
					decLen = crypt.Decrypt(&buffer[37], len, userIV, userPrivate, nickname);
					if(decLen <= 0)
					{
						decLen = 0;
						nickname = 0;
						success = false;
						errStr += "Couldn't decrypt nickname\n";
					}
				}
				contacts.push_back(Contact(contactID, nickname, decLen, &buffer[4]));
				if(nickname != 0)
					delete[] nickname;

				i += 37 + len;
			}
			break;
		}
		case 10:			
			break;
		case 11:
			break;
		case 12:
		{
			recvr(Server, &buffer[1], 4, 0);
			uint32_t size = ntohl(*((uint32_t*)&buffer[1]));
			unsigned int i = 0;
			while(i < size)
			{
				recvr(Server, buffer, 76, 0);
				convID = ntohl(*((uint32_t*)buffer));
				uint32_t creatorID = ntohl(*((uint32_t*)&buffer[4]));
				char* key = new char[32];
				if(creatorID != userID)
				{
					const char* contactPubKey = contacts[GetContactIndex(creatorID)].GetPublicKey();
					curve25519_donna((u8*)key, userPrivate, (const u8*)contactPubKey);
				}
				else
				{
					memcpy(key, userPrivate, 32);
				}

				if(crypt.Decrypt(&buffer[24], 48, (const u8*)&buffer[8], (const u8*)key, &buffer[24]) != 32)
				{
					Disconnect();
					memset(key, 0, 32);
					delete[] key;
					errStr = "Bad decrypt, aborting connection";
					success = false;
					break;
				}
				memset(key, 0, 32);
				delete[] key;

				uint32_t users_num = ntohl(*((uint32_t*)&buffer[72]));
				recvr(Server, &buffer[76], users_num * 4, 0);
				for(unsigned int j = 0; j < users_num; j++)
				{
					uint32_t user = ntohl(*((uint32_t*)&buffer[76 + (4 * j)]));
					memcpy(&buffer[76 + (4 * j)], &user, 4);
				}
				i += 76 + (users_num * 4);
				conversations.push_back(Conversation(convID, creatorID, (const unsigned char*)&buffer[24], (const unsigned int*)&buffer[76], users_num));
			}
			break;
		}
		case 13:
			break;
		case 14:
		{
			recvr(Server, &buffer[1], 4, 0);
			uint32_t size = ntohl(*((uint32_t*)&buffer[1]));
			if(size > 0)
			{
				unsigned int x = 0;
				while(x < size)
				{
					recvr(Server, buffer, 13, 0);
					if(buffer[0] == '\xFF')
					{
						uint32_t convID = ntohl(*((uint32_t*)&buffer[1]));
						int convIndex = GetConvIndex(convID);
						if(convIndex == -1)
						{
							errStr = "The received message belongs to a conversation that doesn't exist locally";
							success = false;
							break;
						}

						uint32_t senderID = ntohl(*((uint32_t*)&buffer[9]));
						uint32_t msgLen = ntohl(*((uint32_t*)&buffer[5]));
						recvr(Server, &buffer[13], 16 + msgLen, 0);
						char* msg = new char[msgLen];
						uint32_t decMsgLen = crypt.Decrypt(&buffer[29], msgLen, (const u8*)&buffer[13], (const u8*)conversations[convIndex].GetSymKey(), msg);
						conversations[convIndex].AddMsg(senderID, msg, decMsgLen);
						delete[] msg;
						x += 1 + 4 + 4 + 4 + 16 + msgLen;

						//Send "case 13" saying we read this message successfully, and don't need to be pushed it again.
						buffer[0] = '\x0D';
						uint32_t sNet = htonl((uint32_t)(1 + 4 + 4 + 16 + msgLen));			//The extra constants are important to the server
						memcpy(&buffer[5], &sNet, 4);
						send(Server, buffer, 1 + 4 + 4, 0);
					}
				}
			}
			break;
		}
		case 15:
			break;
		default:
			errStr = "Bad received data :/";
			success = false;
			break;
		}
		buffer[0] = '\xFF';		//Ensures that an attacker can't overwrite buffer[0] to fake a user request, followed by a malicious response
                                //since their is no server request number xFF and never will be (negatives reserved for conv data)
		return success;
	}
	errStr = "If you are reading this... something is terribly wrong...";
	return false;
}

void Client::Disconnect()
{
	closesocket(Server);
	serverConnected = false;
	signedIn = false;
}

bool Client::GetServersPublicKey()
{
	buffer[0] = 0;
	size = 1;
	send(Server, buffer, size, 0);
	return ReceiveData();
}

unsigned int Client::CreateUser(const char* password)
{
	userID = 0;
	buffer[0] = 1;

	//Start by passing hash test!
	send(Server, buffer, 1, 0);				//Let server know what we want to do
	if(recvr(Server, &buffer[1], 33, 0) != 33)
	{
		errStr = "Lost connection with server";
		return 0;
	}

	unsigned int len = ((unsigned char)buffer[1] / 8);
	bool foundIt = false;

	//Multiple thread support!!
	HashInfo* his = new HashInfo[threadNum];
	for(unsigned int i = 0; i < threadNum; i++)
	{
		his[i].found = &foundIt;
		his[i].inc = threadNum;
		his[i].match = &buffer[2];
		his[i].n = len;
		his[i].result = &buffer[1];
		his[i].salt = &buffer[18];
		his[i].start = (long long)i;
	}

	pthread_t* threads = new pthread_t[threadNum-1];
    for(unsigned int i = 0; i < (unsigned int)(threadNum-1); i++)
		pthread_create(&threads[i], 0, FindHash, &his[i]);

	FindHash(&his[threadNum-1]);
    for(unsigned int i = 0; i < (unsigned int)(threadNum-1); i++)
		pthread_join(threads[i], 0);

	//Generate Values To Store
	fprng.GenerateBlocks(userPrivate, 2);						//Create random private key
	fprng.GenerateBlocks(userIV, 1);							//		 random IV
	fprng.GenerateBlocks(userSalt, 1);							//		 random salt
	curve25519_donna(userPublic, userPrivate, Curve25519Base);

	//Encrypt Private Key Before Giving To Server!!! (Using password)
	libscrypt_scrypt((const uint8_t*)password, strlen(password), (unsigned char*)userSalt, 16, SCRYPT_WORK_VALUE, 8, 1, userPasswordKey, 32);	//Create AES-32 key from password
	crypt.Encrypt((const char*)userPrivate, 32, userIV, userPasswordKey, (char*)encPrivate);

	//Copy values to buffer and send off!!
	memcpy(&buffer[17], userPublic, 32);
	memcpy(&buffer[49], encPrivate, 48);
	memcpy(&buffer[97], userIV, 16);
	memcpy(&buffer[113], userSalt, 16);
	size = 1 + 16 + 32 + 48 + 16 + 16;
	send(Server, buffer, size, 0);
	if(ReceiveData())
		return userID;
	else
		return 0;
}

bool Client::GetInfo(uint8_t info_arg)
{
	if(userID > 0 && serverConnected)
	{
		info = info_arg;
		if(info == 0)
		{
			if(memcmp(userPrivate, zeroArray, 32) == 0)
				info += 1;
			if(memcmp(userIV, zeroArray, 16) == 0)
				info += 2;
			if(memcmp(userSalt, zeroArray, 16) == 0)
				info += 4;
			if(rand == 0)
				info += 8;
		}

		if(info > 0)
		{
			buffer[0] = 2;
			uint32_t u_net = htonl(userID);
			memcpy(&buffer[1], &u_net, 4);
			buffer[5] = info;
			size = 1 + 4 + 1;
			send(Server, buffer, size, 0);
			return ReceiveData();
		}
		else
			return true;
	}
	else
	{
		errStr = "User ID not set\n";
		return false;
	}
}

bool Client::Login(uint32_t userID, const char* password)
{
	if(serverConnected)
	{
		if(userID != this->userID)
		{
			this->userID = userID;
			signedIn = false;
			info = 0;
			memset(encPrivate, 0, 48);
			memset(userPrivate, 0, 32);
			memset(userPublic, 0, 32);
			memset(userPasswordKey, 0, 32);
			memset(userIV, 0, 16);
			memset(userSalt, 0, 16);

			rand = 0;
			conversations.clear();
			contacts.clear();
		}

		if(!GetInfo(4))					//Fetch the salt first
			return false;
		if(password != 0)
			libscrypt_scrypt((const uint8_t*)password, strlen(password), (unsigned char*)userSalt, 16, SCRYPT_WORK_VALUE, 8, 1, userPasswordKey, 32);

		//Get rest of the necessary information from server
		if(!GetInfo())
			return false;

		buffer[0] = 3;
		uint32_t u_net = htonl(userID);
		memcpy(&buffer[1], &u_net, 4);
		char* sharedKey = new char[32];
		curve25519_donna((uint8_t*)sharedKey, userPrivate, servPublic);
		char* hash = new char[32];
		libscrypt_scrypt((const uint8_t*)sharedKey, 32, (const uint8_t*)&rand, 4, 16384, 8, 1, (uint8_t*)hash, 32);
		memcpy(&buffer[5], hash, 32);
		delete[] sharedKey;
		delete[] hash;

		size = 1 + 4 + 32;
		send(Server, buffer, size, 0);
		if(ReceiveData())
		{
			curve25519_donna(userPublic, userPrivate, Curve25519Base);
			signedIn = true;
			rand++;
			return true;
		}
		else
		{
			userID = 0;
			memset(userPrivate, 0, 32);
			signedIn = false;
			return false;
		}
	}
	else
	{
		errStr = "Not connected to server";
		return false;
	}
}

bool Client::GetUsersPublicKey(uint32_t contactID)
{
	if(serverConnected)
	{
		if(GetContactIndex(contactID) == -1)
		{
			errStr = "Contact not added yet";
			return false;
		}

		this->contactID = contactID;
		buffer[0] = 4;
		uint32_t u_net = htonl(contactID);
		memcpy(&buffer[1], &u_net, 4);
		size = 1 + 4;
		send(Server, buffer, size, 0);
		return ReceiveData();
	}
	else
	{
		errStr = "Not connected to server";
		return false;
	}
}

bool Client::AddUserToContacts(uint32_t contactID, const char* nickName, unsigned int nickLen)
{
	if(signedIn)
	{
		unsigned int encNickLen = 0;
		char* encNickName = 0;
		if(nickName != 0 && nickLen != 0)
		{
			encNickLen = PaddedSize(nickLen);
			encNickName = new char[encNickLen];
			crypt.Encrypt(nickName, nickLen, userIV, userPrivate, encNickName);
		}

		this->contactID = contactID;
		buffer[0] = 5;
		uint32_t c_net = htonl(this->contactID);
		memcpy(&buffer[1], &c_net, 4);
		uint8_t l_net = (uint8_t)encNickLen;										//Can't be more than 31 bytes!!
		buffer[5] = l_net;

		if(encNickLen > 0)
		{
			memcpy(&buffer[6], encNickName, encNickLen);
			delete[] encNickName;
		}
		size = 1 + 4 + 1 + encNickLen;
		send(Server, buffer, size, 0);
		if(ReceiveData())
		{
			//Add user
			contacts.push_back(Contact(this->contactID, nickName, nickLen));

			//See if we can find his public key anyway...
			buffer[0] = 4;
			memcpy(&buffer[1], &c_net, 4);
			send(Server, buffer, 5, 0);
			ReceiveData();
			return true;
		}
		else
		{
			return false;
		}
	}
	else
	{
		errStr = "Not signed in";
		return false;
	}
}

unsigned int Client::CreateConversation(unsigned int contactID)
{
	convID = 0;
	if(signedIn)
	{
		int index = GetContactIndex(contactID);
		if(index != -1)
		{
			this->contactID = contactID;
			buffer[0] = 6;
			uint32_t c_net = htonl(this->contactID);
			memcpy(&buffer[1], &c_net, 4);
			char* symKey = new char[32];
			fprng.GenerateBlocks((unsigned char*)symKey, 2);

			char* iv = new char[16];
			fprng.GenerateBlocks((unsigned char*)iv, 1);
			char* myEncSymKey = new char[48];
			crypt.Encrypt(symKey, 32, (const u8*)iv, userPrivate, myEncSymKey);
			memcpy(&buffer[5], iv, 16);
			memcpy(&buffer[21], myEncSymKey, 48);
			delete[] myEncSymKey;

			fprng.GenerateBlocks((unsigned char*)iv, 1);
			char* sharedKey = new char[32];
			curve25519_donna((u8*)sharedKey, userPrivate, (const u8*)contacts[index].GetPublicKey());				//test_userPublic REFERS TO CONTACT'S PUBLIC KEY!
			char* conEncSymKey = new char[48];
			crypt.Encrypt(symKey, 32, (const u8*)iv, (const u8*)sharedKey, conEncSymKey);
			memcpy(&buffer[69], iv, 16);
			memcpy(&buffer[85], conEncSymKey, 48);
			delete[] iv;
			delete[] conEncSymKey;
			delete[] sharedKey;

			size = 1 + 4 + 16 + 48 + 16 + 48;
			send(Server, buffer, size, 0);
			if(ReceiveData())
			{
				conversations.push_back(Conversation(convID, userID, (unsigned char*)symKey, &userID, 1));
				conversations.back().AddUser(contactID);
				memset(symKey, 0, 32);
				delete[] symKey;
				return convID;
			}
			else
			{
				delete[] symKey;
				return 0;
			}
		}
		else
		{
			errStr = "This user ID is not a contact";
			return false;
		}
	}
	else
	{
		errStr = "Not signed in";
		return false;
	}
}

bool Client::AddToConversation(unsigned int contactID, unsigned int convID)
{
	if(signedIn)
	{
		int contactIndex = GetContactIndex(contactID);
		if(contactIndex != -1)
		{
			int convIndex = GetConvIndex(convID);
			if(convIndex != -1)
			{
				this->contactID = contactID;
				this->convID = convID;
				buffer[0] = 7;
				uint32_t c_net = htonl(this->convID);
				memcpy(&buffer[1], &c_net, 4);
				c_net = htonl(this->contactID);
				memcpy(&buffer[5], &c_net, 4);

				char* iv = new char[16];
				fprng.GenerateBlocks((unsigned char*)iv, 1);
				char* sharedKey = new char[32];
				curve25519_donna((u8*)sharedKey, (const u8*)userPrivate, (const u8*)contacts[contactIndex].GetPublicKey());
				char* encSymKey = new char[48];
				crypt.Encrypt(conversations[convIndex].GetSymKey(), 32, (const u8*)iv, (const u8*)sharedKey, encSymKey);
				memcpy(&buffer[9], iv, 16);
				memcpy(&buffer[25], encSymKey, 48);
				delete[] encSymKey;
				delete[] sharedKey;
				delete[] iv;

				size = 1 + 4 + 4 + 16 + 48;
				send(Server, buffer, size, 0);
				if(ReceiveData())
				{
					conversations[convIndex].AddUser(contactID);
					return true;
				}
				else
					return false;
			}
			else
			{
				errStr = "This conversation ID is not added";
				return false;
			}
		}
		else
		{
			errStr = "This user ID is not a contact";
			return false;
		}
	}
	else
	{
		errStr = "Not signed in";
		return false;
	}
}

bool Client::SendMessage(uint32_t convID, const char* msg, unsigned int msgLen)
{
	if(signedIn)
	{
		if(msgLen >= 4096)
		{
			errStr = "Server will not accept a message this long :/";
			return false;
		}
		this->convID = convID;
		int convIndex = GetConvIndex(convID);
		if(convIndex == -1)
		{
			errStr = "The received message belong to a conversation that doesn't exist locally";
			return false;
		}
		uint32_t encMsgLen = PaddedSize(msgLen);

		buffer[0] = 8;
		uint32_t c_net = htonl(convID);
		memcpy(&buffer[1], &c_net, 4);
		uint32_t l_net = htonl(encMsgLen);
		memcpy(&buffer[5], &l_net, 4);
		fprng.GenerateBlocks((unsigned char*)&buffer[9], 1);
		crypt.Encrypt(msg, msgLen, (const u8*)&buffer[9], (const u8*)conversations[convIndex].GetSymKey(), &buffer[25]);
		size = 1 + 4 + 4 + 16 + encMsgLen;
		send(Server, buffer, size, 0);
		return ReceiveData();
	}
	else
	{
		errStr = "Not signed in";
		return false;
	}
}

bool Client::FetchContacts()
{
	if(signedIn)
	{
		buffer[0] = 9;
		size = 1;
		send(Server, buffer, size, 0);
		return ReceiveData();
	}
	else
	{
		errStr = "Not signed in";
		return false;
	}
}

bool Client::RemoveContact(uint32_t contactID)
{
	if(signedIn)
	{
		buffer[0] = 10;
		uint32_t c_net = htonl(contactID);
		memcpy(&buffer[1], &c_net, 4);
		size = 1 + 4;
		send(Server, buffer, size, 0);
		if(ReceiveData())
		{
			contacts = DropIndex<Contact>(contacts, GetContactIndex(contactID));
			return true;
		}
		else
			return false;
	}
	else
	{
		errStr = "Not signed in";
		return false;
	}
}

bool Client::LeaveConversation(uint32_t convID)
{
	if(signedIn)
	{
		buffer[0] = 11;
		uint32_t c_net = htonl(convID);
		memcpy(&buffer[1], &c_net, 4);
		size = 1 + 4;
		send(Server, buffer, size, 0);
		if(ReceiveData())
		{
			conversations = DropIndex<Conversation>(conversations, GetConvIndex(convID));
			return true;
		}
		else
			return false;
	}
	else
	{
		errStr = "Not signed in";
		return false;
	}
}

bool Client::FetchConversations()
{
	if(signedIn)
	{
		buffer[0] = 12;
		size = 1;
		send(Server, buffer, size, 0);
		return ReceiveData();
	}
	else
	{
		errStr = "Not signed in";
		return false;
	}
}

bool Client::FetchMessages()
{
	if(signedIn)
	{
		buffer[0] = 14;
		size = 1;
		send(Server, buffer, size, 0);
		return ReceiveData();
	}
	else
	{
		errStr = "Not signed in";
		return false;
	}
}

bool Client::UpdateNickname(uint32_t contactID, const char* nickName, unsigned int nickLen)
{
	if(signedIn)
	{
		unsigned int encNickLen = 0;
		char* encNickName = 0;
		if(nickName != 0 && nickLen != 0)
		{
			encNickLen = PaddedSize(nickLen);
			encNickName = new char[encNickLen];
			crypt.Encrypt(nickName, nickLen, userIV, userPrivate, encNickName);
		}

		this->contactID = contactID;
		buffer[0] = 15;
		uint32_t c_net = htonl(this->contactID);
		memcpy(&buffer[1], &c_net, 4);
		uint8_t l_net = (uint8_t)encNickLen;										//Can't be more than 31 bytes!!
		buffer[5] = l_net;

		if(encNickLen > 0)
		{
			memcpy(&buffer[6], encNickName, encNickLen);
			delete[] encNickName;
		}
		size = 1 + 1 + 4 + encNickLen;
		send(Server, buffer, size, 0);
		if(ReceiveData())
		{
			contacts[GetContactIndex(this->contactID)].SetNickname(nickName, nickLen);
			return true;
		}
		else
			return false;
	}
	else
	{
		errStr = "Not signed in";
		return false;
	}
}

int Client::GetConvIndex(uint32_t convID)
{
	int convIndex = -1;
	for(int i = 0; i < (int)conversations.size(); i++)
	{
		if(conversations[i].GetConvID() == convID)
		{
			convIndex = i;
			break;
		}
	}
	return convIndex;
}

int Client::GetContactIndex(uint32_t contactID)
{
	int contactIndex = -1;
	for(int i = 0; i < (int)contacts.size(); i++)
	{
		if(contacts[i].GetContactID() == contactID)
		{
			contactIndex = i;
			break;
		}
	}
	return contactIndex;
}

Client::~Client()
{
	memset(userPrivate, 0, 32);
	memset(userPasswordKey, 0, 32);
	if(serverConnected)
		Disconnect();
}

//Static Helper Functions
//---------------------------------------------------------------------------------------------------
template <class T>
std::vector<T> DropIndex(std::vector<T> v, unsigned int i)
{
	unsigned int n = v.size();
	if(i >= n)
		return v;

	std::vector<T> r;
	for(unsigned int j = 0; j < n; j++)
	{
		if(j != i)
			r.push_back(v[j]);
	}
	return r;
}

bool IsIP(string& IP)	//	127.0.0.1		1.2.3.4		123.456.789.012
{
	if(IP.length() >= 7 && IP.length() <= 15)
	{
        unsigned char Periods = 0;
        char PerPos[5] = {0};							//PerPos[0] is -1, three periods, then PerPos[4] points one past the string
		PerPos[0] = -1;
		PerPos[4] = IP.length();

        for(unsigned char i = 0; i < IP.length(); i++)
		{
			if(IP[i] == '.')
			{
				Periods += 1;
				if(Periods <= 3 && i != 0 && i != IP.length()-1)
					PerPos[Periods] = i;
				else
					return false;
			}
			else if(IP[i] < 48 || IP[i] > 57)
				return false;
		}

		unsigned int iTemp = 0;
		for(int i = 0; i < 4; i++)
		{
			if((PerPos[i+1]-1) != PerPos[i])			//Check for two side by side periods
			{
				stringstream ss;
				ss << IP.substr(((signed char)PerPos[i]) + 1, PerPos[i+1] - (PerPos[i] + 1)).c_str();
				ss >> iTemp;
				if(iTemp > 255)
					return false;
			}
			else
				return false;
		}
	}
	else
		return false;

    return true;
}

u_long Resolve(string& addr)
{
	u_long IP;
	memset(&IP, 0, sizeof(u_long));

	//Resolve IPv4 address from hostname
	struct addrinfo hints;
	struct addrinfo *info, *p;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	int Info;
	if((Info = getaddrinfo(addr.c_str(), NULL, &hints, &info)) != 0)
	{
		return IP;
	}
	p = info;
	while(p->ai_family != AF_INET)												//Make sure address is IPv4
	{
		p = p->ai_next;
	}
	IP = (((sockaddr_in*)p->ai_addr)->sin_addr).s_addr;
	freeaddrinfo(info);

	return IP;
}

int recvr(int socket, char* buffer, int length, int flags)
{
	int i = 0;
	while(i < length)
	{
		int n = recv(socket, &buffer[i], length-i, flags);
		if(n <= 0)
			return n;
		i += n;
	}
	return i;
}

int connect_timeout(int fd, const sockaddr* addr, socklen_t len, timeval timeout)
{
	fd_set readset, writeset;
	FD_ZERO(&readset);
	FD_SET(fd, &readset);
	writeset = readset;

	#ifdef WINDOWS
		unsigned long iMode = 1;
		if(ioctlsocket(fd, FIONBIO, &iMode) != 0)
			return -1;						//Couldn't set socket to non-blocking
	#else
		int flags = fcntl(fd, F_GETFL, 0);
		if(flags < 0)
			return -1;						//Couldn't get socket flags

		if(fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0)
			return -1;						//Couldn't set socket flags
	#endif
	int r = connect(fd, addr, len);
	if(r < 0)
	{
		#ifdef WINDOWS
			if(WSAGetLastError() != WSAEINPROGRESS && WSAGetLastError() != WSAEWOULDBLOCK)
				return -2;
		#else
			if(errno != EINPROGRESS)
				return -2;						//Couldn't connect, and not currently trying to connect
		#endif
	}
	if(r != 0)
	{
		r = select(fd + 1, &readset, &writeset, 0, &timeout);
		if(r < 0)
			return -3;						//Select failed
		if(r == 0)
		{   //we had a timeout
			errno = ETIMEDOUT;
			return -4;						//Timeout occured
		}

		int error = 0;
		socklen_t errLen = sizeof(error);

		if(FD_ISSET(fd, &readset) || FD_ISSET(fd, &writeset))
		{
			if(getsockopt(fd, SOL_SOCKET, SO_ERROR, (void*)&error, &errLen) < 0)
				return -5;					//There was an error
		}
		else
			return -6;						//Select told us there were sockets ready, but it's not fd!

		if(error)
		{
			errno = error;
			return -7;
		}
	}
	#ifdef WINDOWS
		iMode = 0;
		if(ioctlsocket(fd, FIONBIO, &iMode) != 0)
			return -8;						//Couldn't set socket to blocking
	#else
		if(fcntl(fd, F_SETFL, flags) < 0)
			return -8;						//Couldn't set socket flags
	#endif

	return 0;								//SUCCESS!!
}

void* FindHash(void* v)
{
	HashInfo* hi = (HashInfo*)v;
	char* attempt = new char[16];
	char* hashOut = new char[16];
	memcpy(attempt, &(hi->start), sizeof(long long));
	while(*hi->found == false)
	{
		*((long long*)attempt) += hi->inc;
		libscrypt_scrypt(attempt, 16, hi->salt, 16, 128, 3, 1, hashOut, 16);
		if(memcmp(&hashOut[0], hi->match, hi->n) == 0)
		{
			*hi->found = true;
			memcpy(hi->result, attempt, 16);
		}
	}
	delete[] hashOut;
	delete[] attempt;
}
