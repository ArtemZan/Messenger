#pragma once
#include "dbghelp.h"

struct Errors
{
	/*Errors& operator>>(std::string& message)
	{
		message = messages.pop_front();
		return *this;
	}

	Errors& operator<<(const std::string& message)
	{
		messages.push_back(message);
		return *this;
	}

	Errors& operator<<(const char* message)
	{
		messages.emplace_back((const char*&&)message, strlen(message));
		return *this;
	}*/

	static Errors& Error(const char* message)
	{
		if (useConsole)
		{
			std::cout << message;
		}
		else
		{
			messages.emplace_back((const char*&&)message, strlen(message));
		}

		return errorsContainer;
	}

	static Errors& Error(const std::string& message)
	{
		if (useConsole)
		{
			std::cout << message;
		}
		else
		{
			messages.push_back(message);
		}

		return errorsContainer;
	}

	template <typename DataT>
	static Errors& Error(const DataT& data)
	{
		if (useConsole)
		{
			std::cout << data;
			return errorsContainer;
		}
		std::stringstream ss;
		ss << data;
		messages.push_back(ss.str());
		return errorsContainer;
	}

	template <typename DataT>
	Errors& Add(const DataT& data)
	{
		if (useConsole)
		{
			std::cout << data;
			return *this;
		}
		std::stringstream ss;
		ss << messages.back();
		ss << data;
		return *this;
	}

	//static inline Errors* GetErrorsContainer() { return &errorsContainer; }

	static inline bool UseConsole() { return useConsole; }
private:
	Errors()
	{
		useConsole = (ImageNtHeader(PVOID(GetModuleHandle(NULL)))->OptionalHeader.Subsystem == IMAGE_SUBSYSTEM_WINDOWS_CUI);
	}

	static tsque<std::string> messages;

	static bool useConsole;

	static Errors errorsContainer;
};

tsque<std::string>	Errors::messages;
Errors				Errors::errorsContainer;
bool				Errors::useConsole;