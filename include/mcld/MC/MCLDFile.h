//===- MCLDFile.h ---------------------------------------------------------===//
//
//                     The MCLinker Project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// MCLDFile represents a file, the content of the file is stored in LDContext.
//
//===----------------------------------------------------------------------===//
#ifndef MCLD_LD_FILE_H
#define MCLD_LD_FILE_H
#ifdef ENABLE_UNITTEST
#include <gtest.h>
#endif

#include <mcld/ADT/Uncopyable.h>
#include <mcld/Support/Path.h>
#include <llvm/ADT/StringRef.h>
#include <string>

namespace mcld {

class LDContext;

/** \class MCLDFile
 *  \brief MCLDFile represents the file being linked or produced.
 *
 *  MCLDFile is the storage of name, path and type
 *  A MCLDFile just refers to LDContext, not owns it.
 *
 *  @see mcld::sys::fs::Path LDContext
 */
class MCLDFile : private Uncopyable
{
public:
  enum Type {
    Unknown,
    Object,
    Exec,
    DynObj,
    CoreFile,
    Script,
    Archive,
    External
  };

public:
  MCLDFile();
  MCLDFile(llvm::StringRef pName);
  MCLDFile(llvm::StringRef pName,
           const sys::fs::Path& pPath,
           unsigned int pType = Unknown);

  virtual ~MCLDFile();

  // -----  modifiers  ----- //
  void setType(unsigned int pType)
  { m_Type = pType; }

  void setContext(LDContext* pContext)
  { m_pContext = pContext; }

  void setPath(const sys::fs::Path& pPath)
  { m_Path = pPath; }

  /// setSOName - set the name of the shared object.
  /// In ELF, this will be written in DT_SONAME
  void setSOName(const std::string& pName);

  // -----  observers  ----- //
  unsigned int type() const
  { return m_Type; }

  const std::string& name() const
  { return m_Name; }

  const sys::fs::Path& path() const
  { return m_Path; }

  bool hasContext() const
  { return (0 != m_pContext); }

  LDContext* context()
  { return m_pContext; }

  const LDContext* context() const
  { return m_pContext; }

protected:
  unsigned int m_Type;
  LDContext *m_pContext;
  sys::fs::Path m_Path;
  std::string m_Name;
};

} // namespace of mcld

#endif

