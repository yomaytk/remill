#include <stdio.h>
#include <cstring>
#include <cstdint>
#include "remill/Arch/Runtime/Intrinsics.h"
#include "remill/Arch/AArch64/Runtime/State.h"
#include "memory.h"

State g_state = State();
MemoryManager *g_memorys;

int main(int argc, char* argv[]) {

  std::vector<EmulatedMemory*> g_emulated_memorys;
  // allocate Stack
  g_emulated_memorys.emplace_back(EmulatedMemory::VmaStackEntryInit(argc, argv));
  // allocate one Heap
  g_emulated_memorys.emplace_back(EmulatedMemory::VmaHeapEntryInit());
  // allocate every sections
  for (int i = 0; i < __g_data_sec_num; i++) {
    g_emulated_memorys.emplace_back( new EmulatedMemory(
      MemoryAreaType::DATA,
      reinterpret_cast<const char*>(__g_data_sec_name_ptr_array[i]),
      __g_data_sec_vma_array[i],
      static_cast<size_t>(__g_data_sec_size_array[i]),
      __g_data_sec_bytes_ptr_array[i],
      __g_data_sec_bytes_ptr_array[i] + __g_data_sec_size_array[i],
      false,
      true
      )
    );
  }
  g_memorys = new MemoryManager (g_emulated_memorys);
  // initalize State 
  g_state.gpr.pc = { .qword = __g_entry_pc };
  g_state.gpr.sp = { .qword = g_emulated_memorys[0]->vma };
  // go to the entry function (entry point is injected by lifted LLVM IR)
  __g_entry_func(&g_state, __g_entry_pc, reinterpret_cast<Memory*>(g_memorys));

  delete(g_memorys);

  return 0;

}
