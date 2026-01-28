#pragma once

#include "RTINFO.h"
#include <vector>
#include <memory>
#include <sstream>

////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct CHTreeNode;
using CHTreeNodePtr = std::unique_ptr<CHTreeNode>;
struct CHTreeNode
{
	//~CHTreeNode() { printf("~CHTreeNode(%d)\n", index); }
	CHTreeNode() : index(0) {}
	CHTreeNode(unsigned int i) : index(i) {}
    unsigned int index = 0; // index to linear array of RTTIBaseClassDescriptor
    std::vector<CHTreeNodePtr> baseClasses;
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////
class RTTI
{
public:
	RTTI(duint addr, bool log = false);

	bool IsValid() const { return m_isValid; }
	duint GetPVFTable() const { return m_pvftable; }
	const std::string &GetTypeName() const { return m_typeNames[0]; }
	const std::string &GetClassHierarchyString() const { return m_classHierarchyStr; }

private:

	bool GetRTTIFromThis(bool log);
	bool GetVFTableFromThis(bool log);
	bool GetCOLFromVFTable(bool log);
	bool GetCHDFromCOL(bool log);
	bool GetBaseClassesFromCHD(bool log);
	void InitClassHierarchyTree();
	void BuildClassHierarchyTree(unsigned int startIdx, unsigned int endIdx, const CHTreeNodePtr &rootNode);
	void CHTreeToString(std::ostringstream &sstr, const CHTreeNodePtr &pnode) const;

	std::string	m_moduleName;
	std::string m_classHierarchyStr;
	duint		m_this = 0;
	duint		m_completeThis = 0;
	duint		m_ppvftable = 0;
	duint		m_pvftable = 0;
	duint		m_pcol = 0;
	duint		m_moduleBase = 0;
	int			m_thisTypeIndex = 0;
	bool		m_isValid = false;

	CHTreeNodePtr							m_classHierarchyTree;
	RTTICompleteObjectLocator				m_completeObjectLocator;
	TypeDescriptor							m_typeDescriptor;
	RTTIClassHierarchyDescriptor			m_classHierarchyDescriptor;
	std::vector<RTTIBaseClassDescriptor>	m_baseClassDescriptors;
	std::vector<TypeDescriptor>				m_baseClassTypeDescriptors;
	std::vector<std::string>				m_typeNames;
	std::vector<int>						m_baseClassOffsets;
};