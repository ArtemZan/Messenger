#pragma once
#include "dbghelp.h"

//Prints errors to console or saves them for later
struct Debug
{
	static inline std::string GetNextMessage() 
	{ 
		if(!messages.empty())
			return messages.pop_front(); 
	}

	static inline bool GetNextMessage(std::string& buffer) 
	{ 
		if (messages.empty())
			return false;
		buffer = messages.pop_front(); 
		return true;
	}

	//If console is used prints message else adds it to the queue
	template <typename DataT>
	static Debug& Message(const DataT& data)
	{
		if (useConsole)
		{
			std::cout << '\n' << data;
			return errorsContainer;
		}
		std::stringstream ss;
		ss << data;
		messages.push_back(ss.str());
		return errorsContainer;
	}

	//If console is used prints message else adds it to the last message
	template <typename DataT>
	Debug& Add(const DataT& data)
	{
		if (useConsole)
		{
			std::cout << data;
			return *this;
		}
		std::stringstream ss;
		ss << messages.pop_back();
		ss << data;
		messages.push_back(ss.str());
		return *this;
	}

	//Does the application use console?
	static inline bool UseConsole() { return useConsole; }
private:
	Debug()
	{
		useConsole = (ImageNtHeader(PVOID(GetModuleHandleA(NULL)))->OptionalHeader.Subsystem == IMAGE_SUBSYSTEM_WINDOWS_CUI);
	}

	static tsque<std::string> messages;

	static bool useConsole;

	static Debug errorsContainer;
};

tsque<std::string>	Debug::messages;
Debug				Debug::errorsContainer;
bool				Debug::useConsole;