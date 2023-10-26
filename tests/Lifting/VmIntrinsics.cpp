#include <iostream>
#include <cstdarg>
#include "remill/Arch/Runtime/Intrinsics.h"
#include "remill/Arch/AArch64/Runtime/State.h"
#include "memory.h"

template <typename T>
NEVER_INLINE static T *getMemoryAddr(addr_t vma_addr) {
  return reinterpret_cast<T*>(_ecv_translate_ptr(vma_addr));
}

#define UNDEFINED_INTRINSICS(intrinsics) printf("[ERROR] undefined intrinsics: %s\n", intrinsics); \
                                          fflush(stdout); \
                                          abort();
#define PRINT_GPREGISTERS(index) printf("w" #index ": %u, x" #index ": %llu, ", g_state.gpr.x##index.dword, g_state.gpr.x##index.qword)

extern "C" void debug_state_machine() {
  printf("[Debug]\n");
  printf("State.GPR: ");
  PRINT_GPREGISTERS(0);
  PRINT_GPREGISTERS(1);
  PRINT_GPREGISTERS(2);
  PRINT_GPREGISTERS(3);
  PRINT_GPREGISTERS(4);
  PRINT_GPREGISTERS(5);
  PRINT_GPREGISTERS(6);
  PRINT_GPREGISTERS(7);
  PRINT_GPREGISTERS(8);
  PRINT_GPREGISTERS(9);
  PRINT_GPREGISTERS(10);
  PRINT_GPREGISTERS(11);
  PRINT_GPREGISTERS(12);
  PRINT_GPREGISTERS(13);
  PRINT_GPREGISTERS(14);
  PRINT_GPREGISTERS(15);
  PRINT_GPREGISTERS(16);
  PRINT_GPREGISTERS(17);
  PRINT_GPREGISTERS(18);
  PRINT_GPREGISTERS(19);
  PRINT_GPREGISTERS(20);
  PRINT_GPREGISTERS(21);
  PRINT_GPREGISTERS(22);
  PRINT_GPREGISTERS(23);
  PRINT_GPREGISTERS(24);
  PRINT_GPREGISTERS(25);
  PRINT_GPREGISTERS(26);
  PRINT_GPREGISTERS(27);
  PRINT_GPREGISTERS(28);
  PRINT_GPREGISTERS(29);
  PRINT_GPREGISTERS(30);
  printf("sp: %llu, pc: 0x%08llx\n", g_state.gpr.sp.qword, g_state.gpr.pc.qword);
  auto nzcv = g_state.nzcv;
  printf("State.NZCV:\nn: %hhu, z: %hhu, c: %hhu, v: %hhu\n", nzcv.n, nzcv.z, nzcv.c, nzcv.v);
  auto sr = g_state.sr;
  printf("State.SR:\ntpidr_el0: %llu, tpidrro_el0: %llu, n: %hhu, z: %hhu, c: %hhu, v: %hhu, ixc: %hhu, ofc: %hhu, ufc: %hhu, idc: %hhu, ioc: %hhu\n", 
    sr.tpidr_el0.qword, sr.tpidrro_el0.qword, sr.n, sr.z, sr.c, sr.v, sr.ixc, sr.ofc, sr.ufc, sr.idc, sr.ioc);
}

extern void debug_g_memorys() {
  for (auto &memory : g_memorys->emulated_memorys) {
    printf("memory_area_type: ");
    switch (memory->memory_area_type)
    {
    case MemoryAreaType::STACK:
      printf("STACK, ");
      break;
    case MemoryAreaType::HEAP:
      printf("HEAP, ");
      break;
    case MemoryAreaType::DATA:
      printf("DATA, ");
      break;
    case MemoryAreaType::RODATA:
      printf("RODATA, ");
      break;
    case MemoryAreaType::OTHER:
      printf("OTHER, ");
      break;
    default:
      printf("[ERROR] unknown memory area type, ");
      abort();
      break;
    }
    printf("name: %s, vma: 0x%016llx, len: %llu, bytes: 0x%016llx, upper_bytes: 0x%016llx, bytes_on_heap: %s, to_higher: %s\n",
      memory->name.c_str(), memory->vma, memory->len, (addr_t)memory->bytes, (addr_t)memory->upper_bytes, memory->bytes_on_heap ? "true" : "false", memory->to_higher ? "true" : "false");
  }
}

uint8_t __remill_read_memory_8(Memory *, addr_t addr){
  return *getMemoryAddr<uint8_t>(addr);
}

uint16_t __remill_read_memory_16(Memory *, addr_t addr){
  return *getMemoryAddr<uint16_t>(addr);
}

uint32_t __remill_read_memory_32(Memory *, addr_t addr) {
  return *getMemoryAddr<uint32_t>(addr);
}

uint64_t __remill_read_memory_64(Memory *, addr_t addr) {
  return *getMemoryAddr<uint64_t>(addr);
}

float32_t __remill_read_memory_f32(Memory *, addr_t addr){
  return *getMemoryAddr<float32_t>(addr);
}

float64_t __remill_read_memory_f64(Memory *, addr_t addr){
  return *getMemoryAddr<float64_t>(addr);
}

float128_t __remill_read_memory_f128(Memory *, addr_t addr){
  return *getMemoryAddr<float128_t>(addr);
}

Memory * __remill_write_memory_8(Memory *memory, addr_t addr, uint8_t src){
  auto dst = getMemoryAddr<uint8_t>(addr);
  *dst = src;
  return memory;
}

Memory * __remill_write_memory_16(Memory *memory, addr_t addr, uint16_t src){
  auto dst = getMemoryAddr<uint16_t>(addr);
  *dst = src;
  return memory;
}

Memory *__remill_write_memory_32(Memory *memory, addr_t addr, uint32_t src) {
  auto dst = getMemoryAddr<uint32_t>(addr);
  *dst = src;
  return memory;
}

Memory *__remill_write_memory_64(Memory *memory, addr_t addr, uint64_t src) {
  auto dst = getMemoryAddr<uint64_t>(addr);
  *dst = src;
  return memory;
}

Memory * __remill_write_memory_f32(Memory *memory, addr_t addr, float32_t src){
  auto dst = getMemoryAddr<float32_t>(addr);
  *dst = src;
  return memory;
}

Memory * __remill_write_memory_f64(Memory *memory, addr_t addr, float64_t src){
  auto dst = getMemoryAddr<float64_t>(addr);
  *dst = src;
  return memory;
}

Memory *__remill_write_memory_f128(Memory *, addr_t, float128_t){ return nullptr; }

Memory *__remill_syscall_tranpoline_call(State &state, Memory *memory) {
  /* TODO: We should select one syscall emulate process (own implementation, WASI, LKL, etc...) */
  __svc_call();
  return memory;
}

// Marks `mem` as being used. This is used for making sure certain symbols are
// kept around through optimization, and makes sure that optimization doesn't
// perform dead-argument elimination on any of the intrinsics.
extern "C" void __remill_mark_as_used(void *mem) {
  asm("" ::"m"(mem));
}

/*
  empty runtime helper function
*/

Memory *__remill_function_return(State &, addr_t, Memory *memory) {
  return memory;
}

Memory *__remill_missing_block(State &, addr_t, Memory *memory) {
  printf("[WARNING] reached \"__remill_missing_block\"\n");
  return memory;
}

Memory *__remill_async_hyper_call(State &, addr_t ret_addr, Memory *memory){ 
  return memory; 
}
// panic function for jumping to .plt section
Memory *__remill_panic_plt_jmp(State &, addr_t, Memory *) {
  printf("[ERROR] detect jumping to .plt section\n");
  fflush(stdout);
  abort();
}

bool __remill_flag_computation_sign(bool result, ...){ return result; }
bool __remill_flag_computation_zero(bool result, ...){ return result; }
bool __remill_flag_computation_overflow(bool result, ...){ return result; }
bool __remill_flag_computation_carry(bool result, ...){ return result; }

bool __remill_compare_sle(bool result){ return result; }
bool __remill_compare_slt(bool result){ return result; }
bool __remill_compare_sge(bool result){ return result; }
bool __remill_compare_sgt(bool result){ return result; }
bool __remill_compare_ule(bool result){ return result; }
bool __remill_compare_ult(bool result){ return result; }
bool __remill_compare_ugt(bool result){ return result; }
bool __remill_compare_uge(bool result){ return result; }
bool __remill_compare_eq(bool result){ return result; }
bool __remill_compare_neq(bool result){ return result; }

Memory *__remill_read_memory_f80(Memory *, addr_t, native_float80_t &){ UNDEFINED_INTRINSICS("__remill_read_memory_f80"); return nullptr; }
Memory * __remill_write_memory_f80(Memory *, addr_t, const native_float80_t &){ UNDEFINED_INTRINSICS("__remill_") return nullptr; }

uint8_t __remill_undefined_8(void){ UNDEFINED_INTRINSICS("__remill_undefined_8"); return 0; }
uint16_t __remill_undefined_16(void){ UNDEFINED_INTRINSICS("__remill_undefined_16"); return 0; }
uint32_t __remill_undefined_32(void){ UNDEFINED_INTRINSICS("__remill_undefined_32"); return 0; }
uint64_t __remill_undefined_64(void){ UNDEFINED_INTRINSICS("__remill_undefied_64"); return 0; }
float32_t __remill_undefined_f32(void){ UNDEFINED_INTRINSICS("__remill_undefined_f32"); return 0; }
float64_t __remill_undefined_f64(void){ UNDEFINED_INTRINSICS("__remill_undefined_f64"); return 0; }
float80_t __remill_undefined_f80(void){ UNDEFINED_INTRINSICS("__remill_undefined_f80"); return 0; }
float128_t __remill_undefined_f128(void){ UNDEFINED_INTRINSICS("__remill_undefined_f128"); return 0; }

Memory * __remill_error(State &, addr_t addr, Memory *){ UNDEFINED_INTRINSICS("__remill_error"); return 0; }
Memory * __remill_function_call(State &, addr_t addr, Memory *){ UNDEFINED_INTRINSICS("__remill_function_call"); return 0; }
// Memory *__remill_function_return(State &, addr_t addr, Memory *){ return 0; }
Memory * __remill_jump(State &, addr_t addr, Memory *){ UNDEFINED_INTRINSICS("__remill_jump"); return 0; }
// Memory *__remill_missing_block(State &, addr_t addr, Memory *){ return 0; }
// Memory * __remill_sync_hyper_call(State &, Memory *, SyncHyperCall::Name){ return 0; }
Memory * __remill_barrier_load_load(Memory *){ UNDEFINED_INTRINSICS("__remill_barrier_load_load"); return 0; }
Memory * __remill_barrier_load_store(Memory *){ UNDEFINED_INTRINSICS("__remill_barrier_load_store"); return 0; }
Memory * __remill_barrier_store_load(Memory *){ UNDEFINED_INTRINSICS("__remill_barrier_store_load"); return 0; }
Memory * __remill_barrier_store_store(Memory *){ UNDEFINED_INTRINSICS("__remill_store_store"); return 0; }
Memory * __remill_atomic_begin(Memory *) { UNDEFINED_INTRINSICS("__remill_atomic_begin"); return 0; }
Memory *__remill_atomic_end(Memory *){ UNDEFINED_INTRINSICS("__remill_atomic_end"); return 0; }
Memory *__remill_delay_slot_begin(Memory *){ UNDEFINED_INTRINSICS("__remill_delay_slot_begin"); return 0; }
Memory *__remill_delay_slot_end(Memory *){ UNDEFINED_INTRINSICS("__remill_delay_slot_end"); return 0; }
Memory *__remill_compare_exchange_memory_8(Memory *, addr_t addr, uint8_t &expected, uint8_t desired){ UNDEFINED_INTRINSICS("__remill_compare_exchange_memory_8"); return 0; }
Memory *__remill_compare_exchange_memory_16(Memory *, addr_t addr, uint16_t &expected, uint16_t desired){ UNDEFINED_INTRINSICS("__remill_compare_exchange_memory_16"); return 0; }
Memory *__remill_compare_exchange_memory_32(Memory *, addr_t addr, uint32_t &expected, uint32_t desired){ UNDEFINED_INTRINSICS("__remill_compare_exchange_memory_32"); return 0; }
Memory *__remill_compare_exchange_memory_64(Memory *, addr_t addr, uint64_t &expected,uint64_t desired){ UNDEFINED_INTRINSICS("__remill_compare_exchange_memory_64"); return 0; }
#if !defined(REMILL_DISABLE_INT128)
Memory *__remill_compare_exchange_memory_128(Memory *, addr_t addr, uint128_t &expected, uint128_t &desired){ UNDEFINED_INTRINSICS("__remill_compare_exchange_memory_128"); return 0; }
#endif
Memory *__remill_fetch_and_add_8(Memory *, addr_t addr, uint8_t &value){ UNDEFINED_INTRINSICS("__remill_fetch_and_add_8"); return 0; }
Memory *__remill_fetch_and_add_16(Memory *, addr_t addr, uint16_t &value){ UNDEFINED_INTRINSICS("__remill_fetch_and_add_16"); return 0; }
Memory *__remill_fetch_and_add_32(Memory *, addr_t addr, uint32_t &value){ UNDEFINED_INTRINSICS("__remill_fetch_and_add_32"); return 0; }
Memory *__remill_fetch_and_add_64(Memory *, addr_t addr, uint64_t &value){ UNDEFINED_INTRINSICS("__remill_fetch_and_add_64"); return 0; }
Memory *__remill_fetch_and_sub_8(Memory *, addr_t addr, uint8_t &value){ UNDEFINED_INTRINSICS("__remill_fetch_and_sub_8"); return 0; }
Memory *__remill_fetch_and_sub_16(Memory *, addr_t addr, uint16_t &value){ UNDEFINED_INTRINSICS("__remill_fetch_and_sub_16"); return 0; }
Memory *__remill_fetch_and_sub_32(Memory *, addr_t addr, uint32_t &value){ UNDEFINED_INTRINSICS("__remill_fetch_and_sub_32"); return 0; }
Memory *__remill_fetch_and_sub_64(Memory *, addr_t addr, uint64_t &value){ UNDEFINED_INTRINSICS("__remill_fetch_and_sub_64"); return 0; }
Memory *__remill_fetch_and_and_8(Memory *, addr_t addr, uint8_t &value){ UNDEFINED_INTRINSICS("__remill_fetch_and_and_8"); return 0; }
Memory *__remill_fetch_and_and_16(Memory *, addr_t addr, uint16_t &value){ UNDEFINED_INTRINSICS("__remill_fetch_and_and_16"); return 0; }
Memory *__remill_fetch_and_and_32(Memory *, addr_t addr, uint32_t &value){ UNDEFINED_INTRINSICS("__remill_fetch_and_and_32"); return 0; }
Memory *__remill_fetch_and_and_64(Memory *, addr_t addr, uint64_t &value){ UNDEFINED_INTRINSICS("__remill_fetch_and_and_64"); return 0; }
Memory *__remill_fetch_and_or_8(Memory *, addr_t addr, uint8_t &value){ UNDEFINED_INTRINSICS("__remill_fetch_and_or_8"); return 0; }
Memory *__remill_fetch_and_or_16(Memory *, addr_t addr, uint16_t &value){ UNDEFINED_INTRINSICS("__remill_fetch_and_or_16"); return 0; }
Memory *__remill_fetch_and_or_32(Memory *, addr_t addr, uint32_t &value){ UNDEFINED_INTRINSICS("__remill_fetch_and_or_32"); return 0; }
Memory *__remill_fetch_and_or_64(Memory *, addr_t addr, uint64_t &value){ UNDEFINED_INTRINSICS("__remill_fetch_and_or_64"); return 0; }
Memory *__remill_fetch_and_xor_8(Memory *, addr_t addr, uint8_t &value){ UNDEFINED_INTRINSICS("__remill_fetch_and_xor_8"); return 0; }
Memory *__remill_fetch_and_xor_16(Memory *, addr_t addr, uint16_t &value){ UNDEFINED_INTRINSICS("__remill_fetch_and_xor_16"); return 0; }
Memory *__remill_fetch_and_xor_32(Memory *, addr_t addr, uint32_t &value){ UNDEFINED_INTRINSICS("__remill_fetch_and_xor_32"); return 0; }
Memory *__remill_fetch_and_xor_64(Memory *, addr_t addr, uint64_t &value){ UNDEFINED_INTRINSICS("__remill_fetch_and_xor_64"); return 0; }
Memory *__remill_fetch_and_nand_8(Memory *, addr_t addr, uint8_t &value){ UNDEFINED_INTRINSICS("__remill_fetch_and_nand_8"); return 0; }
Memory *__remill_fetch_and_nand_16(Memory *, addr_t addr, uint16_t &value){ UNDEFINED_INTRINSICS("__remill_fetch_and_nand_16"); return 0; }
Memory *__remill_fetch_and_nand_32(Memory *, addr_t addr, uint32_t &value){ UNDEFINED_INTRINSICS("__remill_fetch_and_nand_32"); return 0; }
Memory *__remill_fetch_and_nand_64(Memory *, addr_t addr, uint64_t &value){ UNDEFINED_INTRINSICS("__remill_fetch_and_nand_64"); return 0; }
int __remill_fpu_exception_test_and_clear(int read_mask, int clear_mask){ UNDEFINED_INTRINSICS("__remill_fpu_exception_test_and_clear"); return 0; }
uint8_t __remill_read_io_port_8(Memory *, addr_t){ UNDEFINED_INTRINSICS("__remill_read_io_port_8"); return 0; }
uint16_t __remill_read_io_port_16(Memory *, addr_t){ UNDEFINED_INTRINSICS("__remill_read_io_port_16"); return 0; }
uint32_t __remill_read_io_port_32(Memory *, addr_t){ UNDEFINED_INTRINSICS("__remill_read_io_port_32"); return 0; }
Memory *__remill_write_io_port_8(Memory *, addr_t, uint8_t){ UNDEFINED_INTRINSICS("__remill_write_io_port_8"); return 0; }
Memory *__remill_write_io_port_16(Memory *, addr_t, uint16_t){ UNDEFINED_INTRINSICS("__remill_write_io_port_16"); return 0; }
Memory *__remill_write_io_port_32(Memory *, addr_t, uint32_t){ UNDEFINED_INTRINSICS("__remill_write_io_port_32"); return 0; }
Memory *__remill_x86_set_segment_es(Memory *){ UNDEFINED_INTRINSICS("__remill_x86_set_segment_es"); return 0; }
Memory *__remill_x86_set_segment_ss(Memory *){ UNDEFINED_INTRINSICS("__remill_x86_set_segment_ss"); return 0; }
Memory *__remill_x86_set_segment_ds(Memory *){ UNDEFINED_INTRINSICS("__remill_x86_set_segment_ds"); return 0; }
Memory *__remill_x86_set_segment_fs(Memory *){ UNDEFINED_INTRINSICS("__remill_x86_set_segment_fs"); return 0; }
Memory *__remill_x86_set_segment_gs(Memory *){ UNDEFINED_INTRINSICS("__remill_x86_set_segment_gs"); return 0; }
Memory *__remill_x86_set_debug_reg(Memory *){ UNDEFINED_INTRINSICS("__remill_x86_set_debug_reg"); return 0; }
Memory *__remill_x86_set_control_reg_0(Memory *){ UNDEFINED_INTRINSICS("__remill_x86_set_control_reg_0"); return 0; }
Memory *__remill_x86_set_control_reg_1(Memory *){ UNDEFINED_INTRINSICS("__remill_x86_set_control_reg_1"); return 0; }
Memory *__remill_x86_set_control_reg_2(Memory *){ UNDEFINED_INTRINSICS("__remill_x86_set_control_reg_2"); return 0; }
Memory *__remill_x86_set_control_reg_3(Memory *){ UNDEFINED_INTRINSICS("__remill_x86_set_control_reg_3"); return 0; }
Memory *__remill_x86_set_control_reg_4(Memory *){ UNDEFINED_INTRINSICS("__remill_x86_set_control_reg_4"); return 0; }
Memory *__remill_amd64_set_debug_reg(Memory *){ UNDEFINED_INTRINSICS("__remill_amd64_set_debug_reg"); return 0; }
Memory *__remill_amd64_set_control_reg_0(Memory *){ UNDEFINED_INTRINSICS("__remill_amd64_set_control_reg_0"); return 0; }
Memory *__remill_amd64_set_control_reg_1(Memory *){ UNDEFINED_INTRINSICS("__remill_amd64_set_control_reg_1"); return 0; }
Memory *__remill_amd64_set_control_reg_2(Memory *){ UNDEFINED_INTRINSICS("__remill_amd64_set_control_reg_2"); return 0; }
Memory *__remill_amd64_set_control_reg_3(Memory *){ UNDEFINED_INTRINSICS("__remill_amd64_set_control_reg_3"); return 0; }
Memory *__remill_amd64_set_control_reg_4(Memory *){ UNDEFINED_INTRINSICS("__remill_amd64_set_control_reg_4"); return 0; }
Memory *__remill_amd64_set_control_reg_8(Memory *){ UNDEFINED_INTRINSICS("__remill_amd64_set_control_reg_8"); return 0; }
Memory *__remill_aarch64_emulate_instruction(Memory *){ UNDEFINED_INTRINSICS("__remill_aarch64_emulate_instruction"); return 0; }
Memory *__remill_aarch32_emulate_instruction(Memory *){UNDEFINED_INTRINSICS("__remill_aarch32_emulate_instruction");  return 0; }
Memory *__remill_aarch32_check_not_el2(Memory *){ UNDEFINED_INTRINSICS("__remill_aarch32_check_not_el2"); return 0; }
Memory *__remill_sparc_set_asi_register(Memory *){ UNDEFINED_INTRINSICS("__remill_sparc_set_asi_register"); return 0; }
Memory *__remill_sparc_unimplemented_instruction(Memory *){ UNDEFINED_INTRINSICS("__remill_sparc_unimplemented_instruction"); return 0; }
Memory *__remill_sparc_unhandled_dcti(Memory *){ UNDEFINED_INTRINSICS("__remill_sparc_unhandled_dcti"); return 0; }
Memory *__remill_sparc_window_underflow(Memory *){ UNDEFINED_INTRINSICS("__remill_sparc_window_underflow"); return 0; }
Memory *__remill_sparc_trap_cond_a(Memory *){ UNDEFINED_INTRINSICS("__remill_sparc_trap_cond_a"); return 0; }
Memory *__remill_sparc_trap_cond_n(Memory *){ UNDEFINED_INTRINSICS("__remill_sparc_trap_cond_n"); return 0; }
Memory *__remill_sparc_trap_cond_ne(Memory *){ UNDEFINED_INTRINSICS("__remill_sparc_trap_cond_ne"); return 0; }
Memory *__remill_sparc_trap_cond_e(Memory *){ UNDEFINED_INTRINSICS("__remill_sparc_trap_cond_e"); return 0; }
Memory *__remill_sparc_trap_cond_g(Memory *){ UNDEFINED_INTRINSICS("__remill_sparc_trap_cond_g"); return 0; }
Memory *__remill_sparc_trap_cond_le(Memory *){ UNDEFINED_INTRINSICS("__remill_sparc_trap_cond_le"); return 0; }
Memory *__remill_sparc_trap_cond_ge(Memory *){ UNDEFINED_INTRINSICS("__remill_sparc_trap_cond_ge"); return 0; }
Memory *__remill_sparc_trap_cond_l(Memory *){ UNDEFINED_INTRINSICS("__remill_sparc_trap_cond_l"); return 0; }
Memory *__remill_sparc_trap_cond_gu(Memory *){ UNDEFINED_INTRINSICS("__remill_sparc_trap_cond_gu"); return 0; }
Memory *__remill_sparc_trap_cond_leu(Memory *){ UNDEFINED_INTRINSICS("__remill_sparc_trap_cond_leu"); return 0; }
Memory *__remill_sparc_trap_cond_cc(Memory *){ UNDEFINED_INTRINSICS("__remill_sparc_trap_cond_cc"); return 0; }
Memory *__remill_sparc_trap_cond_cs(Memory *){ UNDEFINED_INTRINSICS("__remill_sparc_trap_cond_cs"); return 0; }
Memory *__remill_sparc_trap_cond_pos(Memory *){ UNDEFINED_INTRINSICS("__remill_sparc_trap_cond_pos"); return 0; }
Memory *__remill_sparc_trap_cond_neg(Memory *){ UNDEFINED_INTRINSICS("__remill_sparc_trap_cond_neg"); return 0; }
Memory *__remill_sparc_trap_cond_vc(Memory *){ UNDEFINED_INTRINSICS("__remill_sparc_trap_cond_vc"); return 0; }
Memory *__remill_sparc_trap_cond_vs(Memory *){ UNDEFINED_INTRINSICS("__remill_sparc_trap_cond_vs"); return 0; }
Memory *__remill_sparc32_emulate_instruction(Memory *){ UNDEFINED_INTRINSICS("__remill_sparc32_emulate_instruction"); return 0; }
Memory *__remill_sparc64_emulate_instruction(Memory *){ UNDEFINED_INTRINSICS("__remill_sparc64_emulate_instruction"); return 0; }
Memory *__remill_ppc_emulate_instruction(Memory *){ UNDEFINED_INTRINSICS("__remill_ppc_emulate_instruction"); return 0; }
Memory *__remill_ppc_syscall(Memory *){ UNDEFINED_INTRINSICS("__remill_ppc_syscall"); return 0; }
