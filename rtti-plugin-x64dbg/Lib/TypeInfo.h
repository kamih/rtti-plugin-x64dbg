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

	static TypeInfo fromObjectThisAddr(duint addr, bool log = false);
	static TypeInfo fromCompleteObjectLocatorAddr(const char *modName, duint modBase, duint addr, bool log = false);
	static void scanSection(const char *modName, duint modBase, duint secBase, size_t size);

	bool valid() const { return mValid; }
	duint getVFTablep() const { return mVFTablep; }
	const std::string &getTypeName() const;
	const std::string &getClassHierarchyString() const { return mClassHierarchyStr; }

private:

	bool initFromThisAddr(duint addr, bool log);
	bool initFromCOLAddr(duint addr, bool log);
	bool initFinish(bool log);
	bool getVFTableFromThis(bool log);
	bool getCOLFromVFTable(bool log);	
	
	CompleteObjectLocator	*mCOL = nullptr;
	std::string				mModuleName;
	std::string				mClassHierarchyStr;
	duint					mThis = 0;
	duint					mCompleteThis = 0;
	duint					mVFtablepp = 0;
	duint					mVFTablep = 0;
	duint					mCOLp = 0;
	duint					mModuleBase = 0;
	bool					mValid = false;	
};

};