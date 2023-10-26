#pragma once

#include <string>
#include <cstring>
#include <unistd.h>
#include <vector>
#include "remill/Arch/AArch64/Runtime/State.h"
#include "remill/Arch/Runtime/Types.h"

// SIGSTKSZ is no longer constant in glibc 2.34+
const size_t STACK_SIZE = 4 * 1024 * 1024; /* 4 MiB */
const size_t MAPPED_SIZE = 1024 * 1024; /* 1 MiB */
const addr_t DUMMY_VMA = UINT64_MAX;

// class EmulatedMemory;

typedef uint32_t _ecv_reg_t;
class EmulatedMemory;
class MemoryManager;

extern void __svc_call();
extern void *_ecv_translate_ptr(addr_t vma_addr);
extern void debug_g_memorys();

extern "C" {
  extern const EntryFunc __g_entry_func;
  extern const addr_t __g_entry_pc;
  extern State g_state;
  extern const uint64_t __g_entry_pc;
  extern const uint8_t *__g_data_sec_name_ptr_array[];
  extern const uint64_t __g_data_sec_vma_array[];
  extern uint64_t __g_data_sec_size_array[];
  extern uint8_t *__g_data_sec_bytes_ptr_array[];
  extern const uint64_t __g_data_sec_num;
}

extern _ecv_reg_t _ecv_at_phent;
extern _ecv_reg_t _ecv_at_phnum;
extern MemoryManager *g_memorys;

enum class MemoryAreaType : uint8_t {
  STACK,
  HEAP,
  DATA,
  RODATA,
  OTHER,
};

struct MemoryManager {
  std::vector<EmulatedMemory*> emulated_memorys;
  MemoryManager(std::vector<EmulatedMemory*> __emulated_memorys) : emulated_memorys(__emulated_memorys) {}
  ~MemoryManager() {
    for (auto memory : emulated_memorys)  delete(memory);
  }
};

class alignas(128) EmulatedMemory {

  public:
    EmulatedMemory(MemoryAreaType __memory_area_type, std::string __name, addr_t __vma, uint64_t __len, uint8_t *__bytes, uint8_t* __upper_bytes, bool __bytes_on_heap, bool __to_higher)
      : memory_area_type(__memory_area_type), name(__name), vma(__vma), len(__len), bytes(__bytes), bytes_on_heap(__bytes_on_heap), upper_bytes(__upper_bytes), to_higher(__to_higher) {}
    ~EmulatedMemory() {
      if (bytes_on_heap)  free(bytes);
    }

    static EmulatedMemory *VmaStackEntryInit(int argc, char *argv[]) {
      _ecv_reg_t argc_dummy = 1; /* FIXME: proper argc */
      _ecv_reg_t sp;
      addr_t vma = 0x7ffffffff000; /* 128 TiB */ /* FIXME */
      uint64_t len = STACK_SIZE; /* 4 MiB */
      auto bytes = reinterpret_cast<uint8_t*>(malloc(len));
      memset(bytes, 0, len);

      /* Initialize the stack */
      sp = vma + len;
      g_state.gpr.x29.dword = sp;

      /* Initialize AT_RANDOM */
      /* FIXME: this shouldn't be on the stack? */
      sp -= 16;
      // getentropy(bytes + (sp - vma), 16);
      memset(bytes + (sp - vma), 1, 16);
      _ecv_reg_t randomp = sp;

      /* Initialize AT_PHDR */
      /* FIXME: this shouldn't be on the stack? */
      sp -= _ecv_at_phent * _ecv_at_phnum;
      auto _ecv_at_ph = (uint8_t*)malloc(_ecv_at_phent * _ecv_at_phnum);
      memcpy(bytes + (sp - vma), _ecv_at_ph, _ecv_at_phent * _ecv_at_phnum);
      _ecv_reg_t phdr = sp;

      /* Initialize auxv */
      struct {
        _ecv_reg_t k;
        _ecv_reg_t v;
      } auxv[] = {
        {3 /* AT_PHDR */, phdr},
        {4 /* AT_PHENT */, _ecv_at_phent},
        {5 /* AT_PHNUM */, _ecv_at_phnum},
        {6 /* AT_PAGESZ */, 4096},
        {9 /* AT_ENTRY */, (uint32_t)__g_entry_pc},
        {11 /* AT_UID */, getuid()},
        {12 /* AT_EUID */, geteuid()},
        {13 /* AT_GID */, getgid()},
        {14 /* AT_EGID */, getegid()},
        {23 /* AT_SECURE */, 0},
        {25 /* AT_RANDOM */, randomp},
        {0 /* AT_NULL */, 0},
      };
      sp -= sizeof(auxv);
      memcpy(bytes + (sp - vma), auxv, sizeof(auxv));

      /* TODO: envp */
      sp -= sizeof(_ecv_reg_t);

      /* TODO: argv */
      sp -= sizeof(_ecv_reg_t) * (argc_dummy + 1);

      /* TODO: argc */
      sp -= sizeof(_ecv_reg_t);
      memcpy(bytes + (sp - vma), &argc_dummy, sizeof(_ecv_reg_t));

      g_state.gpr.sp.dword = sp;

      return new EmulatedMemory(MemoryAreaType::STACK, "Stack", vma, len, bytes, bytes + len, true, false);
    }

    static EmulatedMemory *VmaHeapEntryInit() {
      auto bytes = reinterpret_cast<uint8_t*>(malloc(MAPPED_SIZE));
      auto upper_bytes = bytes + MAPPED_SIZE;
      return new EmulatedMemory(MemoryAreaType::HEAP, "Heap", DUMMY_VMA, MAPPED_SIZE, bytes, upper_bytes, true, false);
    }

    MemoryAreaType memory_area_type;
    std::string name;
    addr_t vma;
    uint64_t len;
    uint8_t *bytes;
    uint8_t *upper_bytes;
    bool bytes_on_heap; // whether or not bytes is allocated on the heap memory
    bool to_higher; // whether or not vma is low-leveld address
  
};

