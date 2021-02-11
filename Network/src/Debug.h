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
	template <typename... Args_>
	static void Message(const Args_... data)
	{
		if (useConsole)
		{
			Print(data...);
			return;
		}

		tempSs.str(std::string());

		Add(data...);

		messages.push_back(tempSs.str());
	}

	//If console is used prints message else adds it to the last message
	/*template <typename DataT>
	Debug& Add(const DataT& data)
	{
		if (messages.empty())
			return *this;
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
	}*/

	//Does the application use console?
	static inline bool UseConsole() { return useConsole; }
private:
	Debug()
	{
		useConsole = (ImageNtHeader(PVOID(GetModuleHandleA(NULL)))->OptionalHeader.Subsystem == IMAGE_SUBSYSTEM_WINDOWS_CUI);
	}

	template <typename First_, typename... Args_>
	static void Add(const First_& first, const Args_&... args)
	{
		tempSs << first;
		Add(args...);
	}

	template <typename First_>
	static void Add(const First_& first)
	{
		tempSs << first;
	}

	template <typename First_, typename... Args_>
	static void Print(const First_& first, const Args_&... args)
	{
		std::cout << first;
		Print(args...);
	}

	template <typename First_>
	static void Print(const First_& arg)
	{
		std::cout << arg << '\n';
	}

	static tsque<std::string> messages;

	static bool useConsole;

	static Debug errorsContainer;

	static std::stringstream tempSs;
};

tsque<std::string>	Debug::messages;
Debug				Debug::errorsContainer;
bool				Debug::useConsole;
std::stringstream	Debug::tempSs;