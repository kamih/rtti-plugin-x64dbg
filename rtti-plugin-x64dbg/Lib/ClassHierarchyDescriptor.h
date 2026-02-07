#pragma once
#include "RTINFO.h"
#include "BaseClassDescriptor.h"
#include <vector>
#include <sstream>
#include <set>

namespace RTTI {;

class CompleteObjectLocator;

////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct CHTreeNode;
using CHTreeNodePtr = std::unique_ptr<CHTreeNode>;
struct CHTreeNode
{
	CHTreeNode(int i) : mIndex(i) {}
	int mIndex = 0;
    std::vector<CHTreeNodePtr> mBaseClassNodes;
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ClassHierarchyDescriptor
{
public:
	static ClassHierarchyDescriptor *loadFromAddr(duint modBase, duint addr, bool log);
	std::string toString() const;
	void print() const;	
	TypeDescriptor *findBaseTypeForOffset(DWORD offset) const;
	void updateOffsetsFromCompleteObject(duint objAddr);
	void addCOLRef(CompleteObjectLocator *col);

private:
	ClassHierarchyDescriptor(duint addr, const _ClassHierarchyDescriptor &chd, std::vector<BaseClassDescriptor*> &&bcdVec, std::vector<int> &&offsets, bool completeOffsets);
	void buildClassHierarchyTree(unsigned int startIdx, unsigned int endIdx, const CHTreeNodePtr &node);
	void treeToString(std::ostringstream &sstr, const CHTreeNodePtr &node) const;

	duint								mAddr;				// virtual address in process mem of this _ClassHierarchyDescriptor
	DWORD								mAttributes;		// bit flags
	CHTreeNodePtr						mTree;				// class hierarchy in tree format
	std::set<CompleteObjectLocator*>	mCOLs;				// COLs that use this hierarchy
	std::vector<BaseClassDescriptor*>	mBaseClasses;		// Depth-first linear array of Base classes in this hierarchy
	std::vector<int>					mBaseClassOffsets;	// offsets from complete object to this subobject
	bool								mCompleteOffsets;	// true if all base class offsets from complete object are available
															// false if there are virtual bases and we need an obj instance to find the vbtables to compute their offsets
};

};