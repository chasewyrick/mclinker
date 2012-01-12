//===- RelocationFactory.cpp ----------------------------------------------===//
//
//                     The MCLinker Project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#include <mcld/LD/RelocationFactory.h>
#include <mcld/Target/GOT.h>
#include <mcld/Target/TargetLDBackend.h>
#include <llvm/Support/Host.h>
#include <cstring>
#include <cassert>

using namespace mcld;

//==========================
// RelocationFactory
RelocationFactory::RelocationFactory(size_t pNum)
  : GCFactory<Relocation, 0>(pNum),
    m_pTargetDataFactory(new TargetDataFactory(pNum)),
    m_pLayout(0) {
}

RelocationFactory::~RelocationFactory()
{
  if (0 != m_pTargetDataFactory)
    delete m_pTargetDataFactory;
}

Relocation* RelocationFactory::produce(RelocationFactory::Type pType,
                                       MCFragmentRef& pFragRef,
                                       Address pAddend)
{
  // FIXME: To prevent relocations from double free by both iplist and
  // GCFactory, currently we new relocations directly and let iplist
  // delete them.

  // target_data is the place where the relocation applys to.
  // Use TargetDataFactory to generate temporary data, and copy the
  // content of the fragment into this data.
  DWord* target_data = NULL;
  target_data = m_pTargetDataFactory->allocate();
  *target_data = 0;

  // byte swapping if the host and target have different endian
  if(llvm::sys::isLittleEndianHost() != getTarget().isLittleEndian()) {
     uint32_t tmp_data;

     switch(getTarget().bitclass()) {
      case 32u:
        pFragRef.memcpy(&tmp_data, 4);
        tmp_data = bswap32(tmp_data);
        *target_data = tmp_data;
        break;

      case 64u:
        pFragRef.memcpy(target_data, 8);
        *target_data = bswap64(*target_data);
        break;

      default:
        break;
    }
  }
  else {
    pFragRef.memcpy(target_data, (getTarget().bitclass()/8));
  }

  return new Relocation(pType, &pFragRef, pAddend, target_data, *this);
}

Relocation* RelocationFactory::produceEmptyEntry()
{
  // FIXME: To prevent relocations from double free by both iplist and
  // GCFactory, currently we new relocations directly and let iplist
  // delete them.

  return new Relocation(0, 0, 0, 0, *this);
}

void RelocationFactory::destroy(Relocation* pRelocation)
{
   /** GCFactory will recycle the relocation **/
}

void RelocationFactory::setLayout(const Layout& pLayout)
{
  m_pLayout = &pLayout;
}

const Layout& RelocationFactory::getLayout() const
{
  assert(0 != m_pLayout);
  return *m_pLayout;
}

