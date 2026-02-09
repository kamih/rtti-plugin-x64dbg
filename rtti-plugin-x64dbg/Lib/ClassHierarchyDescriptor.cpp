#include "ClassHierarchyDescriptor.h"
#include "CompleteObjectLocator.h"
#include <map>

namespace RTTI {;

static std::map<duint, ClassHierarchyDescriptor*> gCHDMap; // debugged process mem addr -> our CHD

////////////////////////////////////////////////////////////////////////////////////////////////////////////
ClassHierarchyDescriptor::ClassHierarchyDescriptor(duint addr,
												   const _ClassHierarchyDescriptor &chd,
												   std::vector<BaseClassDescriptor*> &&bcdVec,
												   std::vector<int> &&offsets,
												   bool completeOffsets)
: mAddr(addr)
, mAttributes(chd.attributes)
, mBaseClasses(bcdVec)
, mBaseClassOffsets(offsets)
, mCompleteOffsets(completeOffsets)
{
	assert(mBaseClasses.size() == mBaseClassOffsets.size());
	// Create the class hierarchy tree
	mTree = std::make_unique<CHTreeNode>(0);
	buildClassHierarchyTree(1, (unsigned int)mBaseClasses.size(), mTree);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ClassHierarchyDescriptor::addCOLRef(CompleteObjectLocator *col)
{
	mCOLs.insert(col);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ClassHierarchyDescriptor::buildClassHierarchyTree(unsigned int startIdx,
													   unsigned int endIdx,
													   const CHTreeNodePtr &node)
{
	for (unsigned int i = startIdx; i < endIdx; ++i) {
		auto baseClass = mBaseClasses[i];
		auto baseClassNode = std::make_unique<CHTreeNode>(i);
		const auto numBases = baseClass->getNumContainedBases();
		if (numBases > 0) {
			buildClassHierarchyTree(i + 1, i + 1 + numBases, baseClassNode);
			i += numBases;
		}
		node->mBaseClassNodes.push_back(std::move(baseClassNode));
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Calculates all base class object offsets
void ClassHierarchyDescriptor::updateOffsetsFromCompleteObject(duint objAddr)
 {
	if (mCompleteOffsets)
		return;
	mCompleteOffsets = true;
	for (size_t i = 0; i < mBaseClasses.size(); ++i) {
		mBaseClassOffsets[i] =  mBaseClasses[i]->calcOffsetFromCompleteObj(objAddr);
		if (mBaseClassOffsets[i] == -1)
			mCompleteOffsets = false;
	}
	// Now that all offsets are known,
	// we can find the base type which corresponds to each COL that uses this hierarchy
	if (mCompleteOffsets) {
		for (auto col : mCOLs)
			col->updateBaseType();
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
// returns the type of the first base class that matches the passed offset
TypeDescriptor *ClassHierarchyDescriptor::findBaseTypeForOffset(DWORD offset) const
 {
	for (size_t i = 0; i < mBaseClasses.size(); ++i) {
		if (offset == mBaseClassOffsets[i])
			return mBaseClasses[i]->getTypeDesc();
	}
	return nullptr;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ClassHierarchyDescriptor::print() const
{
	std::string attribStr = "";
	if (mAttributes & CHD_MULTINH)
		attribStr += " Multiple";
	if (mAttributes & CHD_VIRTINH)
		attribStr += " Virtual";
	if (mAttributes & CHD_AMBIGUOUS)
		attribStr += " Ambiguous";
	dprintf("ClassHierarchyDescriptor:\n");
	dprintf("    addr:                  0x%p\n", (void*)mAddr);
	dprintf("    attributes:            0x%X%s\n", mAttributes, attribStr.c_str());
	dprintf("    numBaseClasses:        %d\n", mBaseClasses.size());
	for (int i = 0; i < (int)mBaseClasses.size(); ++i)
		mBaseClasses[i]->print(i, (int)mBaseClasses.size(), mBaseClassOffsets[i]);
}
//// static ////////////////////////////////////////////////////////////////////////////////////////////////
ClassHierarchyDescriptor *ClassHierarchyDescriptor::loadFromAddr(duint modBase, duint addr, bool log)
{
	// First check if we have this in our map
	auto it = gCHDMap.find(addr);
	if (it != gCHDMap.end())
		return it->second;
	_ClassHierarchyDescriptor chd;
	if (!DbgMemRead(addr, (void*)&chd, sizeof(_ClassHierarchyDescriptor))) {
		if (log) dprintf("Couldn't read the _ClassHierarchyDescriptor from: 0x%p\n", addr);
		return nullptr;
	}
	if (chd.signature != 0) {
		if (log) dprintf("Unexpected _ClassHierarchyDescriptor.signature: %d\n", chd.signature);
		return nullptr;
	}
	if (chd.numBaseClasses == 0) {
		if (log) dprintf("Unexpected _ClassHierarchyDescriptor.numBaseClasses: 0\n");
		return nullptr;
	}
	// Read the base class array
	const unsigned int numBCD = chd.numBaseClasses;
	std::vector<DWORD> pBaseClass(numBCD, 0);
	const duint pBaseClassArray = modBase + chd.pBaseClassArray;
	if (!DbgMemRead(pBaseClassArray, (void*)pBaseClass.data(), sizeof(DWORD)*numBCD)) {
		if (log) dprintf("Couldn't read pBaseClassArray: 0x%p\n", (void*)pBaseClassArray);
		return nullptr;
	}
	std::vector<BaseClassDescriptor*> bcdVec;
	std::vector<int> offsets;
	bool completeOffsets = true;
	for (unsigned int i = 0; i < numBCD; ++i) {
		const duint bcAddr = modBase + pBaseClass[i];
		auto *bcd = BaseClassDescriptor::loadFromAddr(modBase, bcAddr, log);
		if (!bcd)
			return nullptr;
		bcdVec.push_back(bcd);
		int offset = bcd->calcOffsetFromCompleteObj(0);
		if (offset == -1)
			completeOffsets = false;
		offsets.push_back(offset);
	}
	// Create new CHD and add it to our map
	auto *c = new ClassHierarchyDescriptor(addr, chd, std::move(bcdVec), std::move(offsets), completeOffsets);
	gCHDMap[addr] = c;
	return c;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
std::string ClassHierarchyDescriptor::toString() const
{
	std::ostringstream sstr;
	treeToString(sstr, mTree);
	return sstr.str();
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ClassHierarchyDescriptor::treeToString(std::ostringstream &sstr, const CHTreeNodePtr &node) const
{
	if (!node->mBaseClassNodes.empty())
		sstr << ": ";
	for (const auto &child : node->mBaseClassNodes) {
		if (child != *node->mBaseClassNodes.begin())
			sstr << ", ";
		const int index = child->mIndex;
		const auto classDesc = mBaseClasses[index];
		if (classDesc->hasAttribute(BCD_VBOFCONTOBJ))
			sstr << "virtual ";
		sstr << classDesc->getTypeDesc()->getName();
		const int offset = mBaseClassOffsets[index];
		if (offset) {
			// -1: unknown vbtable offsets
			if (offset == -1) {
				sstr << "(+?)";
			} else {
				sstr << "(+";
				sstr << std::uppercase << std::hex << offset;
				sstr << ")";
			}
		}
		if (!child->mBaseClassNodes.empty()) {
			sstr << "[";
			treeToString(sstr, child);
			sstr << "]";
		}
	}
}
};