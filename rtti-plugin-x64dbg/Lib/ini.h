/// Borrowed with care MIT License
/// https://github.com/ThunderCls/xAnalyzer

#pragma once
#include <string>

namespace RTTI {;

class IniManager
{
public:
	IniManager(std::string szFileName);

	int readInteger(char *szSection, char *szKey, int iDefaultValue);
	double readDouble(char *szSection, char *szKey, float fltDefaultValue);
	bool readBoolean(char *szSection, char *szKey, bool bolDefaultValue);
	std::string readString(char *szSection, char *szKey, const char *szDefaultValue);

	void writeInteger(char *szSection, char *szKey, int iValue);
	void writeDouble(char *szSection, char *szKey, double fltValue);
	void writeBoolean(char *szSection, char *szKey, bool bolValue);
	void writeString(char *szSection, char *szKey, char *szValue);

private:
	std::string m_szFileName;
};

};