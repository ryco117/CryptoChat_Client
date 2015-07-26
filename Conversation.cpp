#ifndef CONV_CPP
#define CONV_CPP

#include <string.h>
#include <vector>

struct Msg
{
	unsigned int senderID;
	char* msg;
	unsigned int msgLen;
};

class Conversation
{
private:
	unsigned int convID;
	unsigned int creatorID;
	unsigned char symKey[32];
	std::vector<uint32_t> users;
	std::vector<Msg> msgs;

public:
	
	Conversation(unsigned int conv, unsigned int creator, const unsigned char key[32], const unsigned int* u, unsigned int n)
	{
		convID = conv;
		creatorID = creator;
		memcpy(symKey, key, 32);
		if(n > 0)
		{
			for(unsigned int i = 0; i < n; i++)
				users.push_back(u[i]);
		}
	}

	unsigned int GetConvID()
	{
		return convID;
	}

	unsigned int GetCreatorID()
	{
		return creatorID;
	}

	const char* GetSymKey()
	{
		return (const char*)symKey;
	}

	unsigned int GetUsersNum()
	{
		return users.size();
	}

	std::vector<uint32_t> GetUsers()
	{
		return users;
	}

	std::vector<Msg> GetMsgs()
	{
		return msgs;
	}

	void AddUser(unsigned int userID)
	{
		users.push_back(userID);
	}

	void AddMsg(unsigned int senderID, const char* msg, unsigned int msgLen)
	{
		if(strlen(msg) >= msgLen)
		{
			char* cpyMsg = new char[msgLen + 1];
			memcpy(cpyMsg, msg, msgLen);
			cpyMsg[msgLen] = 0;
			Msg m = {senderID, cpyMsg, msgLen};
			msgs.push_back(m);
		}
	}
	
	~Conversation()
	{
		memset(symKey, 0, 32);
		users.clear();

		unsigned int n = msgs.size();
		for(unsigned int i = 0; i < n; i++)
		{
			memset(msgs.back().msg, 0, msgs.back().msgLen);
			delete[] msgs.back().msg;
			msgs.pop_back();
		}
	}
};
#endif