//===- ARMLDBackend.h -----------------------------------------------------===//
//
//                     The MCLinker Project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#ifndef MCLD_ARM_LDBACKEND_H
#define MCLD_ARM_LDBACKEND_H

#include "ARMELFDynamic.h"
#include "ARMGOT.h"
#include "ARMPLT.h"
#include <mcld/LD/LDSection.h>
#include <mcld/Target/GNULDBackend.h>
#include <mcld/Target/OutputRelocSection.h>

namespace mcld {

class LinkerConfig;
class FragmentLinker;
class SectionMap;

//===----------------------------------------------------------------------===//
/// ARMGNULDBackend - linker backend of ARM target of GNU ELF format
///
class ARMGNULDBackend : public GNULDBackend
{
public:
  // max branch offsets for ARM, THUMB, and THUMB2
  // @ref gold/arm.cc:99
  static const int32_t ARM_MAX_FWD_BRANCH_OFFSET = ((((1 << 23) - 1) << 2) + 8);
  static const int32_t ARM_MAX_BWD_BRANCH_OFFSET = ((-((1 << 23) << 2)) + 8);
  static const int32_t THM_MAX_FWD_BRANCH_OFFSET = ((1 << 22) -2 + 4);
  static const int32_t THM_MAX_BWD_BRANCH_OFFSET = (-(1 << 22) + 4);
  static const int32_t THM2_MAX_FWD_BRANCH_OFFSET = (((1 << 24) - 2) + 4);
  static const int32_t THM2_MAX_BWD_BRANCH_OFFSET = (-(1 << 24) + 4);

public:
  ARMGNULDBackend(const LinkerConfig& pConfig);
  ~ARMGNULDBackend();

public:
  typedef std::vector<llvm::ELF::Elf32_Dyn*> ELF32DynList;

  /** \enum ReservedEntryType
   *  \brief The reserved entry type of reserved space in ResolveInfo.
   *
   *  This is used for sacnRelocation to record what kinds of entries are
   *  reserved for this resolved symbol
   *
   *  In ARM, there are three kinds of entries, GOT, PLT, and dynamic reloction.
   *  GOT may needs a corresponding relocation to relocate itself, so we
   *  separate GOT to two situations: GOT and GOTRel. Besides, for the same
   *  symbol, there might be two kinds of entries reserved for different location.
   *  For example, reference to the same symbol, one may use GOT and the other may
   *  use dynamic relocation.
   *
   *  bit:  3       2      1     0
   *   | PLT | GOTRel | GOT | Rel |
   *
   *  value    Name         - Description
   *
   *  0000     None         - no reserved entry
   *  0001     ReserveRel   - reserve an dynamic relocation entry
   *  0010     ReserveGOT   - reserve an GOT entry
   *  0011     GOTandRel    - For different relocation, we've reserved GOT and
   *                          Rel for different location.
   *  0100     GOTRel       - reserve an GOT entry and the corresponding Dyncamic
   *                          relocation entry which relocate this GOT entry
   *  0101     GOTRelandRel - For different relocation, we've reserved GOTRel
   *                          and relocation entry for different location.
   *  1000     ReservePLT   - reserve an PLT entry and the corresponding GOT,
   *                          Dynamic relocation entries
   *  1001     PLTandRel    - For different relocation, we've reserved PLT and
   *                          Rel for different location.
   */
  enum ReservedEntryType {
    None         = 0,
    ReserveRel   = 1,
    ReserveGOT   = 2,
    GOTandRel    = 3,
    GOTRel       = 4,
    GOTRelandRel = 5,
    ReservePLT   = 8,
    PLTandRel    = 9
  };

public:
  /// initTargetSections - initialize target dependent sections in output.
  void initTargetSections(Module& pModule, ObjectBuilder& pBuilder);

  /// initTargetSymbols - initialize target dependent symbols in output.
  void initTargetSymbols(FragmentLinker& pLinker);

  /// initRelocFactory - create and initialize RelocationFactory
  bool initRelocFactory(const FragmentLinker& pLinker);

  /// getRelocFactory
  RelocationFactory* getRelocFactory();

  /// scanRelocation - determine the empty entries are needed or not and create
  /// the empty entries if needed.
  /// For ARM, following entries are check to create:
  /// - GOT entry (for .got section)
  /// - PLT entry (for .plt section)
  /// - dynamin relocation entries (for .rel.plt and .rel.dyn sections)
  void scanRelocation(Relocation& pReloc,
                      const LDSymbol& pInputSym,
                      FragmentLinker& pLinker,
                      Module& pModule,
                      const LDSection& pSection);

  uint32_t machine() const
  { return llvm::ELF::EM_ARM; }

  /// OSABI - the value of e_ident[EI_OSABI]
  virtual uint8_t OSABI() const
  { return llvm::ELF::ELFOSABI_NONE; }

  /// ABIVersion - the value of e_ident[EI_ABIVRESION]
  virtual uint8_t ABIVersion() const
  { return 0x0; }

  /// flags - the value of ElfXX_Ehdr::e_flags
  virtual uint64_t flags() const
  { return (llvm::ELF::EF_ARM_EABIMASK & 0x05000000); }

  bool isLittleEndian() const
  { return true; }

  unsigned int bitclass() const
  { return 32; }

  uint64_t defaultTextSegmentAddr() const
  { return 0x8000; }

  /// doPreLayout - Backend can do any needed modification before layout
  void doPreLayout(FragmentLinker& pLinker);

  /// doPostLayout -Backend can do any needed modification after layout
  void doPostLayout(Module& pModule, FragmentLinker& pLinker);

  /// dynamic - the dynamic section of the target machine.
  /// Use co-variant return type to return its own dynamic section.
  ARMELFDynamic& dynamic();

  /// dynamic - the dynamic section of the target machine.
  /// Use co-variant return type to return its own dynamic section.
  const ARMELFDynamic& dynamic() const;


  /// emitSectionData - write out the section data into the memory region.
  /// When writers get a LDSection whose kind is LDFileFormat::Target, writers
  /// call back target backend to emit the data.
  ///
  /// Backends handle the target-special tables (plt, gp,...) by themselves.
  /// Backend can put the data of the tables in SectionData directly
  ///  - LDSection.getSectionData can get the section data.
  /// Or, backend can put the data into special data structure
  ///  - backend can maintain its own map<LDSection, table> to get the table
  /// from given LDSection.
  ///
  /// @param pSection - the given LDSection
  /// @param pConfig - all options in the command line.
  /// @param pLayout - for comouting the size of fragment
  /// @param pRegion - the region to write out data
  /// @return the size of the table in the file.
  uint64_t emitSectionData(const LDSection& pSection,
                           const Layout& pLayout,
                           MemoryRegion& pRegion) const;

  ARMGOT& getGOT();

  const ARMGOT& getGOT() const;

  ARMPLT& getPLT();

  const ARMPLT& getPLT() const;

  OutputRelocSection& getRelDyn();

  const OutputRelocSection& getRelDyn() const;

  OutputRelocSection& getRelPLT();

  const OutputRelocSection& getRelPLT() const;

  /// getTargetSectionOrder - compute the layout order of ARM target sections
  unsigned int getTargetSectionOrder(const LDSection& pSectHdr) const;

  /// finalizeTargetSymbols - finalize the symbol value
  bool finalizeTargetSymbols(FragmentLinker& pLinker);

  /// readSection - read target dependent sections
  bool readSection(Input& pInput, SectionData& pSD);

private:
  void scanLocalReloc(Relocation& pReloc, FragmentLinker& pLinker);

  void scanGlobalReloc(Relocation& pReloc, FragmentLinker& pLinker);

  void checkValidReloc(Relocation& pReloc,
                       const FragmentLinker& pLinker) const;

  /// addCopyReloc - add a copy relocation into .rel.dyn for pSym
  /// @param pSym - A resolved copy symbol that defined in BSS section
  void addCopyReloc(ResolveInfo& pSym);

  /// defineSymbolforCopyReloc - allocate a space in BSS section and
  /// and force define the copy of pSym to BSS section
  /// @return the output LDSymbol of the copy symbol
  LDSymbol& defineSymbolforCopyReloc(FragmentLinker& pLinker,
                                     const ResolveInfo& pSym);

  /// updateAddend - update addend value of the relocation if the
  /// the target symbol is a section symbol. Addend is the offset
  /// in the section. This value should be updated after section
  /// merged.
  void updateAddend(Relocation& pReloc,
                    const LDSymbol& pInputSym,
                    const Layout& pLayout) const;

  void defineGOTSymbol(FragmentLinker& pLinker);

  /// maxBranchOffset
  /// FIXME: if we can handle arm attributes, we may refine this!
  uint64_t maxBranchOffset() { return THM_MAX_FWD_BRANCH_OFFSET; }

  /// mayRelax - Backends should override this function if they need relaxation
  bool mayRelax() { return true; }

  /// doRelax - Backend can orevride this function to add its relaxation
  /// implementation. Return true if the output (e.g., .text) is "relaxed"
  /// (i.e. layout is changed), and set pFinished to true if everything is fit,
  /// otherwise set it to false.
  bool doRelax(FragmentLinker& pLinker, bool& pFinished);

  /// initTargetStubs
  bool initTargetStubs(FragmentLinker& pLinker);

  /// getRelEntrySize - the size in BYTE of rel type relocation
  size_t getRelEntrySize()
  { return 8; }

  /// getRelEntrySize - the size in BYTE of rela type relocation
  size_t getRelaEntrySize()
  { assert(0 && "ARM backend with Rela type relocation\n"); return 12; }

  /// doCreateProgramHdrs - backend can implement this function to create the
  /// target-dependent segments
  virtual void doCreateProgramHdrs(Module& pModule,
                                   const FragmentLinker& pLinker);

private:
  RelocationFactory* m_pRelocFactory;
  ARMGOT* m_pGOT;
  ARMPLT* m_pPLT;
  /// m_RelDyn - dynamic relocation table of .rel.dyn
  OutputRelocSection* m_pRelDyn;
  /// m_RelPLT - dynamic relocation table of .rel.plt
  OutputRelocSection* m_pRelPLT;

  ARMELFDynamic* m_pDynamic;
  LDSymbol* m_pGOTSymbol;
  LDSymbol* m_pEXIDXStart;
  LDSymbol* m_pEXIDXEnd;

  //     variable name           :  ELF
  LDSection* m_pEXIDX;           // .ARM.exidx
  LDSection* m_pEXTAB;           // .ARM.extab
  LDSection* m_pAttributes;      // .ARM.attributes
//  LDSection* m_pPreemptMap;      // .ARM.preemptmap
//  LDSection* m_pDebugOverlay;    // .ARM.debug_overlay
//  LDSection* m_pOverlayTable;    // .ARM.overlay_table
};

//===----------------------------------------------------------------------===//
/// ARMMachOLDBackend - linker backend of ARM target of MachO format
///
/**
class ARMMachOLDBackend : public DarwinARMLDBackend
{
public:
  ARMMachOLDBackend();
  ~ARMMachOLDBackend();

private:
  MCMachOTargetArchiveReader *createTargetArchiveReader() const;
  MCMachOTargetObjectReader *createTargetObjectReader() const;
  MCMachOTargetObjectWriter *createTargetObjectWriter() const;

};
**/
} // namespace of mcld

#endif

