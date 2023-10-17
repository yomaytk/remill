#pragma once

#include "remill/Arch/AArch64/Runtime/State.h"
#include "remill/Arch/Runtime/Types.h"

// SIGSTKSZ is no longer constant in glibc 2.34+
const size_t STACK_SIZE = 4 * 1024 * 1024; /* 4 MiB */
const size_t MAPPED_SIZE = 1024 * 1024; /* 1 MiB */
const addr_t DUMMY_VMA = UINT64_MAX;

extern void __svc_call();

enum class MemoryAreaType : uint8_t {
  STACK,
  HEAP,
  BSS,
  RODATA,
  OTHER,
};

class alignas(128) EmulatedMemory {

  public:
    EmulatedMemory(MemoryAreaType __memory_area_type, addr_t __vma, uint64_t __len, uint8_t *__bytes)
      : memory_area_type(__memory_area_type), vma(__vma), len(__len), bytes(__bytes) {}
    ~EmulatedMemory() {
      free(bytes);
    }

    MemoryAreaType memory_area_type;
    addr_t vma;
    uint64_t len;
    uint8_t *bytes;
  
};

extern "C" {
  extern EntryFunc __g_entry_func;
  extern State g_state;
  extern addr_t g_pc;
}

extern EmulatedMemory **g_emulated_memorys;
