//===- GNULDBackend.h -----------------------------------------------------===//
//
//                     The MCLinker Project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#ifndef MCLD_TARGET_GNU_LDBACKEND_H
#define MCLD_TARGET_GNU_LDBACKEND_H
#ifdef ENABLE_UNITTEST
#include <gtest.h>
#endif
#include <mcld/Target/TargetLDBackend.h>

#include <llvm/Support/ELF.h>
#include <mcld/ADT/HashTable.h>
#include <mcld/ADT/HashEntry.h>
#include <mcld/LD/ELFDynObjFileFormat.h>
#include <mcld/LD/ELFExecFileFormat.h>
#include <mcld/LD/GNUArchiveReader.h>
#include <mcld/LD/ELFObjectReader.h>
#include <mcld/LD/ELFDynObjReader.h>
#include <mcld/LD/ELFDynObjWriter.h>
#include <mcld/LD/ELFExecWriter.h>
#include <mcld/LD/ELFObjectWriter.h>
#include <mcld/LD/ELFSegment.h>
#include <mcld/LD/ELFSegmentFactory.h>
#include <mcld/Target/ELFDynamic.h>

#include <mcld/Support/GCFactory.h>
#include <mcld/Module.h>

namespace mcld {

struct SymCompare
{
  bool operator()(const LDSymbol* X, const LDSymbol* Y) const
  { return (X==Y); }
};

struct PtrHash
{
  size_t operator()(const LDSymbol* pKey) const
  {
    return (unsigned((uintptr_t)pKey) >> 4) ^
           (unsigned((uintptr_t)pKey) >> 9);
  }
};

class Module;
class LinkerConfig;
class Layout;
class EhFrameHdr;

/** \class GNULDBackend
 *  \brief GNULDBackend provides a common interface for all GNU Unix-OS
 *  LDBackend.
 */
class GNULDBackend : public TargetLDBackend
{
protected:
  GNULDBackend(const LinkerConfig& pConfig);

public:
  virtual ~GNULDBackend();

  // -----  readers/writers  ----- //
  GNUArchiveReader* createArchiveReader(Module& pModule);
  ELFObjectReader* createObjectReader(FragmentLinker& pLinker);
  ELFDynObjReader* createDynObjReader(FragmentLinker& pLinker);
  ELFObjectWriter* createObjectWriter(FragmentLinker& pLinker);
  ELFDynObjWriter* createDynObjWriter(FragmentLinker& pLinker);
  ELFExecWriter*   createExecWriter(FragmentLinker& pLinker);

  // -----  output sections  ----- //
  /// initExecSections - initialize sections of the output executable file.
  bool initExecSections(FragmentLinker& pLinker);

  /// initDynObjSections - initialize sections of the output shared object.
  bool initDynObjSections(FragmentLinker& pLinker);

  /// getOutputFormat - get the sections of the output file.
  const ELFFileFormat* getOutputFormat() const;
  ELFFileFormat*       getOutputFormat();

  ELFDynObjFileFormat* getDynObjFileFormat();
  const ELFDynObjFileFormat* getDynObjFileFormat() const;

  ELFExecFileFormat* getExecFileFormat();
  const ELFExecFileFormat* getExecFileFormat() const;

  // -----  target symbols ----- //
  /// initStandardSymbols - initialize standard symbols.
  /// Some section symbols is undefined in input object, and linkers must set
  /// up its value. Take __init_array_begin for example. This symbol is an
  /// undefined symbol in input objects. FragmentLinker must finalize its value
  /// to the begin of the .init_array section, then relocation enties to
  /// __init_array_begin can be applied without emission of "undefined
  /// reference to `__init_array_begin'".
  bool initStandardSymbols(FragmentLinker& pLinker);

  /// finalizeSymbol - Linker checks pSymbol.reserved() if it's not zero,
  /// then it will ask backend to finalize the symbol value.
  /// @return ture - if backend set the symbol value sucessfully
  /// @return false - if backend do not recognize the symbol
  bool finalizeSymbols(FragmentLinker& pLinker) {
    return (finalizeStandardSymbols(pLinker) &&
            finalizeTargetSymbols(pLinker));
  }

  /// finalizeStandardSymbols - set the value of standard symbols
  virtual bool finalizeStandardSymbols(FragmentLinker& pLinker);

  /// finalizeTargetSymbols - set the value of target symbols
  virtual bool finalizeTargetSymbols(FragmentLinker& pLinker) = 0;

  /// finalizeTLSSymbol - set the value of a TLS symbol
  virtual bool finalizeTLSSymbol(FragmentLinker& pLinker, LDSymbol& pSymbol);

  size_t sectionStartOffset() const;

  /// The return value of machine() it the same as e_machine in the ELF header*/
  virtual uint32_t machine() const = 0;

  /// ELFVersion - the value of e_ident[EI_VERSION]
  virtual uint8_t ELFVersion() const
  { return llvm::ELF::EV_CURRENT; }

  /// OSABI - the value of e_ident[EI_OSABI]
  virtual uint8_t OSABI() const = 0;

  /// ABIVersion - the value of e_ident[EI_ABIVRESION]
  virtual uint8_t ABIVersion() const = 0;

  /// flags - the value of ElfXX_Ehdr::e_flags
  virtual uint64_t flags() const = 0;

  /// entry - the symbol name of the entry point
  virtual const char* entry() const
  { return "_start"; }

  /// dyld - the name of the default dynamic linker
  /// target may override this function if needed.
  /// @ref gnu ld, bfd/elf32-i386.c:521
  virtual const char* dyld() const
  { return "/usr/lib/libc.so.1"; }

  /// defaultTextSegmentAddr - target should specify its own default start address
  /// of the text segment. esp. for exec.
  virtual uint64_t defaultTextSegmentAddr() const
  { return 0x0; }

  bool hasTextRel() const
  { return m_bHasTextRel; }

  bool hasStaticTLS() const
  { return m_bHasStaticTLS; }

  /// segmentStartAddr - this function returns the start address of the segment
  uint64_t segmentStartAddr(const FragmentLinker& pLinker) const;

  /// sizeNamePools - compute the size of regular name pools
  /// In ELF executable files, regular name pools are .symtab, .strtab.,
  /// .dynsym, .dynstr, and .hash
  virtual void sizeNamePools(const Module& pModule);

  /// emitSectionData - emit target-dependent section data
  virtual uint64_t emitSectionData(const LDSection& pSection,
                                   const Layout& pLayout,
                                   MemoryRegion& pRegion) const = 0;

  /// emitRegNamePools - emit regular name pools - .symtab, .strtab
  virtual void emitRegNamePools(const Module& pModule,
                                const Layout& pLayout,
                                MemoryArea& pOutput);

  /// emitNamePools - emit dynamic name pools - .dyntab, .dynstr, .hash
  virtual void emitDynNamePools(const Module& pModule,
                                const Layout& pLayout,
                                MemoryArea& pOutput);

  /// sizeInterp - compute the size of program interpreter's name
  /// In ELF executables, this is the length of dynamic linker's path name
  virtual void sizeInterp();

  /// emitInterp - emit the .interp
  virtual void emitInterp(MemoryArea& pOutput);

  /// getSectionOrder - compute the layout order of the section
  /// Layout calls this function to get the default order of the pSectHdr.
  /// If the pSectHdr.type() is LDFileFormat::Target, then getSectionOrder()
  /// will call getTargetSectionOrder().
  ///
  /// If targets favors certain order for general sections, please override
  /// this function.
  ///
  /// @see getTargetSectionOrder
  virtual unsigned int getSectionOrder(const LDSection& pSectHdr) const;

  /// getTargetSectionOrder - compute the layout order of target section
  /// If the target favors certain order for the given gSectHdr, please
  /// override this function.
  ///
  /// By default, this function returns the maximun order, and pSectHdr
  /// will be the last section to be laid out.
  virtual unsigned int getTargetSectionOrder(const LDSection& pSectHdr) const
  { return (unsigned int)-1; }

  /// numOfSegments - return the number of segments
  /// if the target favors other ways to emit program header, please override
  /// this function
  virtual unsigned int numOfSegments() const
  { return m_ELFSegmentTable.size(); }

  /// elfSegmentTable - return the reference of the elf segment table
  ELFSegmentFactory& elfSegmentTable()
  { return m_ELFSegmentTable; }

  /// elfSegmentTable - return the reference of the elf segment table
  const ELFSegmentFactory& elfSegmentTable() const
  { return m_ELFSegmentTable; }

  /// commonPageSize - the common page size of the target machine, and we set it
  /// to 4K here. If target favors the different size, please override this
  /// function
  virtual uint64_t commonPageSize() const;

  /// abiPageSize - the abi page size of the target machine, and we set it to 4K
  /// here. If target favors the different size, please override this function
  virtual uint64_t abiPageSize() const;

  /// getSymbolIdx - get the symbol index of ouput symbol table
  size_t getSymbolIdx(LDSymbol* pSymbol) const;

  /// isDefaultExecStack - target should specify whether the stack is default
  /// executable. If target favors another choice, please override this function
  virtual bool isDefaultExecStack() const
  { return true; }

  /// allocateCommonSymbols - allocate common symbols in the corresponding
  /// sections.
  /// Different concrete target backend may overlap this function.
  virtual bool allocateCommonSymbols(Module& pModule, FragmentLinker& pLinker);

  /// isSymbolPreemtible - whether the symbol can be preemted by other
  /// link unit
  /// @ref Google gold linker, symtab.h:551
  bool isSymbolPreemptible(const ResolveInfo& pSym) const;

  /// symbolNeedsDynRel - return whether the symbol needs a dynamic relocation
  /// @ref Google gold linker, symtab.h:645
  bool symbolNeedsDynRel(const FragmentLinker& pLinker,
                         const ResolveInfo& pSym,
                         bool pSymHasPLT,
                         bool isAbsReloc) const;

  // getTDATASymbol - get section symbol of .tdata
  LDSymbol& getTDATASymbol();
  const LDSymbol& getTDATASymbol() const;

  /// getTBSSSymbol - get section symbol of .tbss
  LDSymbol& getTBSSSymbol();
  const LDSymbol& getTBSSSymbol() const;

protected:
  uint64_t getSymbolSize(const LDSymbol& pSymbol) const;

  uint64_t getSymbolInfo(const LDSymbol& pSymbol) const;

  uint64_t getSymbolValue(const LDSymbol& pSymbol) const;

  uint64_t getSymbolShndx(const LDSymbol& pSymbol, const Layout& pLayout) const;

  /// getHashBucketCount - calculate hash bucket count.
  /// @ref Google gold linker, dynobj.cc:791
  static unsigned getHashBucketCount(unsigned pNumOfSymbols, bool pIsGNUStyle);

  /// isDynamicSymbol
  /// @ref Google gold linker: symtab.cc:311
  bool isDynamicSymbol(const LDSymbol& pSymbol);

  /// symbolNeedsPLT - return whether the symbol needs a PLT entry
  /// @ref Google gold linker, symtab.h:596
  bool symbolNeedsPLT(const FragmentLinker& pLinker,
                      const ResolveInfo& pSym) const;

  /// symbolNeedsCopyReloc - return whether the symbol needs a copy relocation
  bool symbolNeedsCopyReloc(const FragmentLinker& pLinker,
                            const Relocation& pReloc,
                            const ResolveInfo& pSym) const;

  /// emitSymbol32 - emit an ELF32 symbol
  void emitSymbol32(llvm::ELF::Elf32_Sym& pSym32,
                    LDSymbol& pSymbol,
                    const Layout& pLayout,
                    char* pStrtab,
                    size_t pStrtabsize,
                    size_t pSymtabIdx);

  /// emitSymbol64 - emit an ELF64 symbol
  void emitSymbol64(llvm::ELF::Elf64_Sym& pSym64,
                    LDSymbol& pSymbol,
                    const Layout& pLayout,
                    char* pStrtab,
                    size_t pStrtabsize,
                    size_t pSymtabIdx);

  /// defineTDATASymbol - define section symbol for .tdata
  bool defineTDATASymbol(FragmentLinker& pLinker);

  /// defineTBSSSymbol - define section symbol for .tbss
  bool defineTBSSSymbol(FragmentLinker& pLinker);

  /// checkAndSetHasTextRel - check pSection flag to set HasTextRel
  void checkAndSetHasTextRel(const LDSection& pSection);

  void setHasStaticTLS(bool pVal = true)
  { m_bHasStaticTLS = pVal; }

private:
  /// createProgramHdrs - base on output sections to create the program headers
  void createProgramHdrs(Module& pModule, const FragmentLinker& pLinker);

  /// setupProgramHdrs - set up the attributes of segments
  ///  (i.e., offset, addresses, file/mem size, flag,  and alignment)
  void setupProgramHdrs(const FragmentLinker& pLinker);

  /// getSegmentFlag - give a section flag and return the corresponding segment
  /// flag
  inline uint32_t getSegmentFlag(const uint32_t pSectionFlag)
  {
    uint32_t flag = llvm::ELF::PF_R;
    if (0 != (pSectionFlag & llvm::ELF::SHF_WRITE))
      flag |= llvm::ELF::PF_W;
    if (0 != (pSectionFlag & llvm::ELF::SHF_EXECINSTR))
      flag |= llvm::ELF::PF_X;
    return flag;
  }

  /// createGNUStackInfo - create an output GNU stack section or segment if needed
  void createGNUStackInfo(const Module& pModule, FragmentLinker& pLinker);

  /// setOutputSectionOffset - helper function to set output sections' offset.
  void setOutputSectionOffset(Module& pModule,
                              Module::iterator pSectBegin,
                              Module::iterator pSectEnd);

  /// setOutputSectionOffset - helper function to set output sections' address.
  void setOutputSectionAddress(FragmentLinker& pLinker,
                               Module& pModule,
                               Module::iterator pSectBegin,
                               Module::iterator pSectEnd);

  /// preLayout - Backend can do any needed modification before layout
  void preLayout(Module& pModule, FragmentLinker& pLinker);

  /// postLayout -Backend can do any needed modification after layout
  void postLayout(Module& pModule, FragmentLinker& pLinker);

  /// preLayout - Backend can do any needed modification before layout
  virtual void doPreLayout(FragmentLinker& pLinker) = 0;

  /// postLayout -Backend can do any needed modification after layout
  virtual void doPostLayout(Module& pModule, FragmentLinker& pLinker) = 0;

  /// postProcessing - Backend can do any needed modification in the final stage
  void postProcessing(FragmentLinker& pLinker, MemoryArea& pOutput);

  /// dynamic - the dynamic section of the target machine.
  virtual ELFDynamic& dynamic() = 0;

  /// dynamic - the dynamic section of the target machine.
  virtual const ELFDynamic& dynamic() const = 0;

protected:
  // Based on Kind in LDFileFormat to define basic section orders for ELF, and
  // refer gold linker to add more enumerations to handle Regular and BSS kind
  enum SectionOrder {
    SHO_INTERP = 1,          // .interp
    SHO_RO_NOTE,             // .note.ABI-tag, .note.gnu.build-id
    SHO_NAMEPOOL,            // *.hash, .dynsym, .dynstr
    SHO_RELOCATION,          // .rel.*, .rela.*
    SHO_REL_PLT,             // .rel.plt should come after other .rel.*
    SHO_INIT,                // .init
    SHO_PLT,                 // .plt
    SHO_TEXT,                // .text
    SHO_FINI,                // .fini
    SHO_RO,                  // .rodata
    SHO_EXCEPTION,           // .eh_frame_hdr, .eh_frame, .gcc_except_table
    SHO_TLS_DATA,            // .tdata
    SHO_TLS_BSS,             // .tbss
    SHO_RELRO_LOCAL,         // .data.rel.ro.local
    SHO_RELRO,               // .data.rel.ro,
    SHO_RELRO_LAST,          // for x86 to adjust .got if needed
    SHO_NON_RELRO_FIRST,     // for x86 to adjust .got.plt if needed
    SHO_DATA,                // .data
    SHO_LARGE_DATA,          // .ldata
    SHO_RW_NOTE,             //
    SHO_SMALL_DATA,          // .sdata
    SHO_SMALL_BSS,           // .sbss
    SHO_BSS,                 // .bss
    SHO_LARGE_BSS,           // .lbss
    SHO_UNDEFINED = ~(0U)    // default order
  };

  typedef HashEntry<LDSymbol*, size_t, SymCompare> HashEntryType;
  typedef HashTable<HashEntryType, PtrHash, EntryFactory<HashEntryType> > HashTableType;

protected:
  ELFObjectReader* m_pObjectReader;

  // -----  file formats  ----- //
  ELFDynObjFileFormat* m_pDynObjFileFormat;
  ELFExecFileFormat* m_pExecFileFormat;

  // ELF segment factory
  ELFSegmentFactory m_ELFSegmentTable;

  // map the LDSymbol to its index in the output symbol table
  HashTableType* m_pSymIndexMap;

  // section .eh_frame_hdr
  EhFrameHdr* m_pEhFrameHdr;

  // ----- dynamic flags ----- //
  // DF_TEXTREL of DT_FLAGS
  bool m_bHasTextRel;

  // DF_STATIC_TLS of DT_FLAGS
  bool m_bHasStaticTLS;

  // -----  standard symbols  ----- //
  // section symbols
  LDSymbol* f_pPreInitArrayStart;
  LDSymbol* f_pPreInitArrayEnd;
  LDSymbol* f_pInitArrayStart;
  LDSymbol* f_pInitArrayEnd;
  LDSymbol* f_pFiniArrayStart;
  LDSymbol* f_pFiniArrayEnd;
  LDSymbol* f_pStack;

  // section symbols for .tdata and .tbss
  LDSymbol* f_pTDATA;
  LDSymbol* f_pTBSS;

  // segment symbols
  LDSymbol* f_pExecutableStart;
  LDSymbol* f_pEText;
  LDSymbol* f_p_EText;
  LDSymbol* f_p__EText;
  LDSymbol* f_pEData;
  LDSymbol* f_p_EData;
  LDSymbol* f_pBSSStart;
  LDSymbol* f_pEnd;
  LDSymbol* f_p_End;
};

} // namespace of mcld

#endif

