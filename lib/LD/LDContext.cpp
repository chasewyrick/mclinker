//===- LDContext.cpp ------------------------------------------------------===//
//
//                     The MCLinker Project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#include <mcld/LD/LDContext.h>
#include <mcld/LD/LDSection.h>
#include <mcld/LD/LDSymbol.h>
#include <llvm/ADT/StringRef.h>

using namespace mcld;

//==========================
// LDReader
LDContext::LDContext()
{
}

LDContext::~LDContext()
{
}

LDSection* LDContext::getSection(unsigned int pIdx)
{
  if (pIdx >= m_SectionTable.size())
    return NULL;
  return m_SectionTable[pIdx];
}

const LDSection* LDContext::getSection(unsigned int pIdx) const
{
  if (pIdx >= m_SectionTable.size())
    return NULL;
  return m_SectionTable[pIdx];
}

LDSection* LDContext::getSection(const std::string& pName)
{
  sect_iterator sect_iter, sect_end = sectEnd();
  for (sect_iter = sectBegin(); sect_iter != sect_end; ++sect_iter) {
    if((*sect_iter)->name() == pName)
      return *sect_iter;
  }
  return NULL;
}

const LDSection* LDContext::getSection(const std::string& pName) const
{
  const_sect_iterator sect_iter, sect_end = sectEnd();
  for (sect_iter = sectBegin(); sect_iter != sect_end; ++sect_iter) {
    if((*sect_iter)->name() == pName)
      return *sect_iter;
  }
  return NULL;
}

size_t LDContext::getSymbolIdx(const llvm::StringRef& pName) const
{
  size_t result = 1;
  size_t size = m_SymTab.size();
  for (; result < size; ++result)
    if (m_SymTab[result]->name() == pName)
      return result;
  return 0;
}

size_t LDContext::getSectionIdx(const std::string& pName) const
{
  size_t result = 1;
  size_t size = m_SectionTable.size();
  for (; result != size; ++result)
    if (m_SectionTable[result]->name() == pName)
      return result;
  return 0;
}

LDSymbol* LDContext::getSymbol(unsigned int pIdx)
{
  if (pIdx >= m_SymTab.size())
    return NULL;
  return m_SymTab[pIdx];
}

const LDSymbol* LDContext::getSymbol(unsigned int pIdx) const
{
  if (pIdx >= m_SymTab.size())
    return NULL;
  return m_SymTab[pIdx];
}


LDSymbol* LDContext::getSymbol(const llvm::StringRef& pName)
{
  size_t sym = 1;
  size_t size = m_SymTab.size();
  for (; sym < size; ++sym)
    if (m_SymTab[sym]->name() == pName)
      return m_SymTab[sym];
  return NULL;
}

const LDSymbol* LDContext::getSymbol(const llvm::StringRef& pName) const
{
  size_t sym = 1;
  size_t size = m_SymTab.size();
  for (; sym < size; ++sym)
    if (m_SymTab[sym]->name() == pName)
      return m_SymTab[sym];
  return NULL;
}

