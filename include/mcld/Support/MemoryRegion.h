/*****************************************************************************
 *   The MCLinker Project, Copyright (C), 2011 -                             *
 *   Embedded and Web Computing Lab, National Taiwan University              *
 *   MediaTek, Inc.                                                          *
 *                                                                           *
 *   Luba Tang <lubatang@mediatek.com>                                       *
 ****************************************************************************/
#ifndef LD_MEMORY_REGION_H
#define LD_MEMORY_REGION_H
#ifdef ENABLE_UNITTEST
#include <gtest.h>
#endif

#include <mcld/ADT/Uncopyable.h>
#include <llvm/ADT/ilist.h>
#include <llvm/ADT/StringRef.h>
#include <mcld/Support/FileSystem.h>

namespace mcld
{

/** \class MemoryRegion
 *  \brief MemoryRegion provides a customized memory map IO for linking purposes.
 *
 *  \see
 */
class MemoryRegion : private Uncopyable
{
friend class RegionFactory;
typedef sys::fs::detail::Address Address;
private:
  MemoryRegion(const Address pVMAStart,
               size_t pSize);
public:
  ~MemoryRegion();

  const Address start() const
  { return m_VMAStart; }

  // end is the next address of the last byte 
  const Address end() const
  { return m_VMAStart+m_Length; }

  size_t size() const
  { return m_Length; }

  const Address getBuffer() const
  { return m_VMAStart; }

private:
  const Address m_VMAStart;
  size_t m_Length;
};

} // namespace of mcld

#endif

