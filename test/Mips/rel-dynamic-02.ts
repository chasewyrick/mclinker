# Conditions:
#   a) Linking a shared library.
#   b) Relocations' targets are undefined symbols.
# Check:
#   a) Emitting R_MIPS_REL32 relocations for both relocations.
#   b) There should be no PLT entries.
#
# RUN: yaml2obj -format=elf %s > %t.o
# RUN: %MCLinker -mtriple=mipsel-linux-gnu -shared -o %t.so %t.o
# RUN: llvm-readobj -dt -r -s %t.so | FileCheck %s

# CHECK:     Sections [
# CHECK:       Section {
# CHECK-NOT:     Name: .plt
#
# CHECK:      Relocations [
# CHECK-NEXT:   Section (4) .rel.dyn {
# CHECK-NEXT:     0x[[ADDR1:[0-9A-Z]+]] R_MIPS_REL32 T1 0x0
# CHECK-NEXT:     0x[[ADDR2:[0-9A-Z]+]] R_MIPS_REL32 T1 0x0
# CHECK-NEXT:   }
# CHECK-NEXT: ]
#
# CHECK:      Symbol {
# CHECK:        Name: D0@
# CHECK-NEXT:   Value: 0x[[ADDR2]]
# CHECK-NEXT:   Size: 8
# CHECK-NEXT:   Binding: Global
# CHECK-NEXT:   Type: Object
# CHECK-NEXT:   Other: 0
# CHECK-NEXT:   Section: .data
# CHECK-NEXT: }
# CHECK:      Symbol {
# CHECK:        Name: T0@
# CHECK-NEXT:   Value: 0x[[ADDR1]]
# CHECK-NEXT:   Size: 8
# CHECK-NEXT:   Binding: Global
# CHECK-NEXT:   Type: Function
# CHECK-NEXT:   Other: 0
# CHECK-NEXT:   Section: .text
# CHECK-NEXT: }
# CHECK:      Symbol {
# CHECK:        Name: T1@
# CHECK-NEXT:   Value: 0x0
# CHECK-NEXT:   Size: 0
# CHECK-NEXT:   Binding: Global
# CHECK-NEXT:   Type: None
# CHECK-NEXT:   Other: 0
# CHECK-NEXT:   Section: Undefined
# CHECK-NEXT: }

---
FileHeader:
  Class:   ELFCLASS32
  Data:    ELFDATA2LSB
  Type:    ET_REL
  Machine: EM_MIPS
  Flags:   [EF_MIPS_PIC, EF_MIPS_CPIC, EF_MIPS_ABI_O32, EF_MIPS_ARCH_32]

Sections:
- Name:         .text
  Type:         SHT_PROGBITS
  Size:         8
  AddressAlign: 16
  Flags:        [SHF_EXECINSTR, SHF_ALLOC]

- Name:         .rel.text
  Type:         SHT_REL
  Info:         .text
  AddressAlign: 4
  Relocations:
    - Offset: 0
      Symbol: T1
      Type:   R_MIPS_32

- Name:         .data
  Type:         SHT_PROGBITS
  Size:         8
  AddressAlign: 16
  Flags:        [SHF_WRITE, SHF_ALLOC]

- Name:         .rel.data
  Type:         SHT_REL
  Info:         .data
  AddressAlign: 4
  Relocations:
    - Offset: 0
      Symbol: T1
      Type:   R_MIPS_32

Symbols:
  Global:
    - Name:    T0
      Section: .text
      Type:    STT_FUNC
      Value:   0
      Size:    8
    - Name:    D0
      Section: .data
      Type:    STT_OBJECT
      Value:   0
      Size:    8
    - Name:    T1
...
