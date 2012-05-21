//===- MsgHandler.h -------------------------------------------------------===//
//
//                     The MCLinker Project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#ifndef MCLD_MESSAGE_HANDLER_H
#define MCLD_MESSAGE_HANDLER_H
#ifdef ENABLE_UNITTEST
#include <gtest.h>
#endif
#include <string>
#include <llvm/ADT/StringRef.h>
#include <llvm/ADT/Twine.h>
#include <mcld/Support/Path.h>
#include <mcld/LD/DiagnosticEngine.h>

namespace mcld
{

/** \class MsgHandler
 *  \brief MsgHandler controls the timing to output message.
 */
class MsgHandler
{
public:
  MsgHandler(DiagnosticEngine& pEngine);
  ~MsgHandler();

  bool emit();

  void addString(llvm::StringRef pStr) const;

  void addString(const std::string& pStr) const;

  void addTaggedVal(intptr_t pValue, DiagnosticEngine::ArgumentKind pKind) const;

private:
  void flushCounts()
  { m_Engine.state().numArgs = m_NumArgs; }

private:
  DiagnosticEngine& m_Engine;
  mutable unsigned int m_NumArgs;
};

inline const MsgHandler &
operator<<(const MsgHandler& pHandler, llvm::StringRef pStr)
{
  pHandler.addString(pStr);
  return pHandler;
}

inline const MsgHandler &
operator<<(const MsgHandler& pHandler, const std::string& pStr)
{
  pHandler.addString(pStr);
  return pHandler;
}

inline const MsgHandler &
operator<<(const MsgHandler& pHandler, const sys::fs::Path& pPath)
{
  pHandler.addString(pPath.native());
  return pHandler;
}

inline const MsgHandler &
operator<<(const MsgHandler& pHandler, const char* pStr)
{
  pHandler.addTaggedVal(reinterpret_cast<intptr_t>(pStr),
                        DiagnosticEngine::ak_c_string);
  return pHandler;
}

inline const MsgHandler &
operator<<(const MsgHandler& pHandler, int pValue)
{
  pHandler.addTaggedVal(pValue, DiagnosticEngine::ak_sint);
  return pHandler;
}

inline const MsgHandler &
operator<<(const MsgHandler& pHandler, unsigned int pValue)
{
  pHandler.addTaggedVal(pValue, DiagnosticEngine::ak_uint);
  return pHandler;
}

inline const MsgHandler &
operator<<(const MsgHandler& pHandler, size_t pValue)
{
  pHandler.addTaggedVal(pValue, DiagnosticEngine::ak_uint);
  return pHandler;
}

inline const MsgHandler &
operator<<(const MsgHandler& pHandler, bool pValue)
{
  pHandler.addTaggedVal(pValue, DiagnosticEngine::ak_bool);
  return pHandler;
}

//===----------------------------------------------------------------------===//
// Inline member functions
inline MsgHandler
DiagnosticEngine::report(uint16_t pID, DiagnosticEngine::Severity pSeverity)
{
  m_State.ID = pID;
  m_State.severity = pSeverity;

  MsgHandler result(*this);
  return result;
}

} // namespace of mcld

#endif

