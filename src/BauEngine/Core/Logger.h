#pragma once
#include <iostream>

class Logger
{
public:
	inline void Info(std::string m)
	{
		std::cout << "\x1b[1;36m" << "[INFO]: " << "\x1b[0m" << m << std::endl;
	}

	inline void Warn(std::string m)
	{
		std::cout << "\x1b[1;33m" << "[WARN]: " << "\x1b[0m" << m << std::endl;
	}

	inline void Error(std::string m)
	{
		std::cout << "\x1b[1;31m" << "[ERROR/NOT CRIT]: " << "\x1b[0m" << m << std::endl;
	}

	inline void CritErr(std::string m)
	{
		throw new std::runtime_error(std::string("\x1b[1;31m") + "[ERROR/CRIT]: " + "\x1b[0m" + m );
	}
};