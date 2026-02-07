#pragma once

#include "Utils.h"

namespace RTTI {;

class CompleteObjectLocator;

////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TypeInfo
{
public:
	TypeInfo() {}
	TypeInfo(const char *modName, duint modBase);

	static TypeInfo FromObjectThisAddr(duint addr, bool log = false);
	static TypeInfo FromCompleteObjectLocatorAddr(const char *modName, duint modBase, duint addr, bool log = false);
	static void ScanSection(const char *modName, duint modBase, duint secBase, size_t size);

	bool IsValid() const { return m_isValid; }
	duint GetPVFTable() const { return m_pvftable; }
	const std::string &GetTypeName() const;
	const std::string &GetClassHierarchyString() const { return m_classHierarchyStr; }

private:

	bool InitFromThisAddr(duint addr, bool log);
	bool InitFromCOLAddr(duint addr, bool log);
	bool Init(bool log);
	bool GetVFTableFromThis(bool log);
	bool GetCOLFromVFTable(bool log);	
	
	std::string	m_moduleName;
	std::string m_classHierarchyStr;
	duint		m_this = 0;
	duint		m_completeThis = 0;
	duint		m_ppvftable = 0;
	duint		m_pvftable = 0;
	duint		m_pcol = 0;
	duint		m_moduleBase = 0;
	bool		m_isValid = false;

	CompleteObjectLocator	*m_completeObjectLocator = nullptr;
};

};