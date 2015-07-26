#ifndef CONTACT_CPP
#define CONTACT_CPP

#include <string>
#include "string.h"

static char zeroByte = '\0';

class Contact
{
private:
	unsigned int contactID;
	std::string nickname;
	char publicKey[32];

public:
	
	Contact(unsigned int id, const char* nick, unsigned int len, const char* publicKey = 0)
	{
		contactID = id;
		if(len > 0 && nick != 0)
		{
			nickname = std::string(nick, len);
		}
		else
		{
			nickname.clear();
		}

		if(publicKey != 0)
			memcpy(this->publicKey, publicKey, 32);
	}

	bool HasNickname()
	{
		return (!nickname.empty());
	}

	unsigned int GetContactID()
	{
		return contactID;
	}

	const char* GetNickname()
	{
		if(HasNickname())
			return nickname.c_str();
		else
			return &zeroByte;
	}

	const char* GetPublicKey()
	{
		return (const char*)publicKey;
	}

	void SetNickname(const char* nick, unsigned int len)
	{
		nickname = std::string(nick, len);
	}

	void SetPublicKey(const char* publicKey)
	{
		memcpy(this->publicKey, publicKey, 32);
	}
	
	~Contact()
	{
		memset(publicKey, 0, 32);
		nickname.clear();
	}
};
#endif