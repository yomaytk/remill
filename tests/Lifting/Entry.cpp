#include <stdio.h>
#include <cstring>
#include <cstdint>
#include "remill/Arch/Runtime/Intrinsics.h"
#include "remill/Arch/AArch64/Runtime/State.h"
#include "memory.h"

addr_t g_pc;
State g_state = State();

EmulatedMemory **g_emulated_memorys;

int main(int argc, char** argv) {

  // allocate emulated stack and heap memory
  g_emulated_memorys[0] = new EmulatedMemory(MemoryAreaType::STACK, DUMMY_VMA, STACK_SIZE, reinterpret_cast<uint8_t*>(malloc(STACK_SIZE)));
  g_emulated_memorys[1] = new EmulatedMemory(MemoryAreaType::HEAP, DUMMY_VMA, MAPPED_SIZE, reinterpret_cast<uint8_t*>(malloc(MAPPED_SIZE)));
  // initalize State and PC
  g_pc = reinterpret_cast<addr_t>(__g_entry_func);
  g_state.gpr.pc = { .qword = reinterpret_cast<addr_t>(__g_entry_func) };
  g_state.gpr.sp = { .qword = reinterpret_cast<addr_t>(g_emulated_memorys[0]->bytes)};
  // go to the entry function (entry point is injected by lifted LLVM IR)
  __g_entry_func(&g_state, g_pc, reinterpret_cast<Memory*>(g_emulated_memorys));
  return 0;

}
