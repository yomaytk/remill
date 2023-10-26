/*
 * Copyright (c) 2018 Trail of Bits, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "remill/BC/Lifter.h"
#include "Lift.h"

DEFINE_string(bc_out, "",
              "Name of the file in which to place the generated bitcode.");

DEFINE_string(os, REMILL_OS,
              "Operating system name of the code being "
              "translated. Valid OSes: linux, macos, windows, solaris.");
DEFINE_string(arch, REMILL_ARCH,
              "Architecture of the code being translated. "
              "Valid architectures: x86, amd64 (with or without "
              "`_avx` or `_avx512` appended), aarch64, aarch32");
DEFINE_string(target_elf, "DUMMY_ELF",
              "Name of the target ELF binary");

void AArch64TraceManager::SetLiftedTraceDefinition(uint64_t addr, llvm::Function *lifted_func) {
  traces[addr] = lifted_func;
}

llvm::Function *AArch64TraceManager::GetLiftedTraceDeclaration(uint64_t addr) {
  auto trace_it = traces.find(addr);
  if (trace_it != traces.end()) {
    return trace_it->second;
  } else {
    return nullptr;
  }
}

llvm::Function *AArch64TraceManager::GetLiftedTraceDefinition(uint64_t addr) {
  return GetLiftedTraceDeclaration(addr);
}

bool AArch64TraceManager::TryReadExecutableByte(uint64_t addr, uint8_t *byte) {
  
  auto byte_it = memory.find(addr);
  if (byte_it != memory.end()) {
    *byte = byte_it->second;
    return true;
  } else {
    return false;
  }

}

std::string AArch64TraceManager::Sub_FuncName(uint64_t addr) {
  std::stringstream ss;
  ss << "sub_" << std::hex << addr;
  return ss.str();
}

std::string AArch64TraceManager::TraceName(uint64_t addr) {
  // call super class method
  auto fun_name = Sub_FuncName(addr);
  prerefered_func_addrs[addr] = true;
  return fun_name;
}

std::string AArch64TraceManager::GetUniqueLiftedFuncName(std::string func_name) {
  return func_name + "_" + to_string(unique_i64++) + "__Lifted";
}

void AArch64TraceManager::SetELFData() {

  elf_obj.LoadELF();
  entry_point = elf_obj.entry;
  // set text section
  elf_obj.SetCodeSection();
  // set symbol table (WARNING: only when the ELF binary is not stripped)
  auto func_entrys = elf_obj.GetFuncEntry();
  std::sort(func_entrys.rbegin(), func_entrys.rend());
  // set instruction bytes of every code sections
  size_t i = 0;
  while (i < func_entrys.size()) {
    uint64_t fun_bytes_size;
    uintptr_t fun_end_addr;
    uintptr_t sec_addr;
    uintptr_t n_fun_end_addr;
    uint8_t *bytes;
    int sec_included_cnt = 0;
    // specify included section
    for (auto &[_, code_sec] : elf_obj.code_sections) {
      if (code_sec.vma <= func_entrys[i].entry && func_entrys[i].entry < code_sec.vma + code_sec.size) {
        sec_addr = code_sec.vma;
        fun_end_addr = code_sec.vma + code_sec.size;
        fun_bytes_size = (code_sec.vma + code_sec.size) - func_entrys[i].entry;
        bytes = code_sec.bytes;
        sec_included_cnt++;
      }
    }
    if (sec_included_cnt != 1) {
      printf("[ERROR] \"%s\" is not included in one code section.\n", func_entrys[i].func_name.c_str());
      exit(EXIT_FAILURE);
    }
    n_fun_end_addr = UINTPTR_MAX;
    while (sec_addr < n_fun_end_addr) {
      // assign every insn to the manager
      auto lifted_func_name = GetUniqueLiftedFuncName(func_entrys[i].func_name);
      auto dasm_func = DisasmFunc(lifted_func_name, func_entrys[i].entry, fun_bytes_size);
      // program entry point
      if (entry_point == func_entrys[i].entry) {
        if (!entry_func_lifted_name.empty()) {
          printf("[ERROR] multiple entrypoints are found.\n");
          exit(EXIT_FAILURE);
        }
        entry_func_lifted_name = dasm_func.func_name;
      }
      for (uintptr_t addr = func_entrys[i].entry;addr < fun_end_addr; addr++) {
        memory[addr] = bytes[addr - sec_addr];
      }
      disasm_funcs[func_entrys[i].entry] = dasm_func;
      n_fun_end_addr = func_entrys[i].entry;
      i++;
    }
  }

}

extern "C" int main(int argc, char *argv[]) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);

  AArch64TraceManager manager(FLAGS_target_elf);
  manager.SetELFData();

  llvm::LLVMContext context;
  auto os_name = remill::GetOSName(REMILL_OS);
  auto arch_name = remill::GetArchName(FLAGS_arch);
  auto arch = remill::Arch::Build(&context, os_name, arch_name);
  auto module = remill::LoadArchSemantics(arch.get());

  remill::IntrinsicTable intrinsics(module.get());
  remill::TraceLifter trace_lifter(arch.get(), manager);

  // lift every disassembled function
  for (const auto &[_, asm_func] : manager.disasm_funcs) {
    if (!trace_lifter.Lift(asm_func.vma)) {
      printf("[ERROR] Failed to Lift \"%s\"\n", asm_func.func_name.c_str());
      exit(EXIT_FAILURE);
    }
    // set function name
    auto lifted_trace = manager.GetLiftedTraceDefinition(asm_func.vma);
    lifted_trace->setName(asm_func.func_name.c_str());
  }
  // set entry point
  if (manager.entry_func_lifted_name.empty()) {
    printf("[ERROR] We couldn't find entry function.\n");
    exit(EXIT_FAILURE);
  } else {
    trace_lifter.SetEntryPoint(manager.entry_func_lifted_name);
  }
  // set entry pc
  trace_lifter.SetEntryPC(manager.entry_point);
  // set data section
  trace_lifter.SetDataSections(manager.elf_obj.sections);
  // define prerefered functions
  for (auto [addr, pre_refered] : manager.prerefered_func_addrs) {
    if (!pre_refered) {
      continue;
    }
    auto lifted_func_name = manager.disasm_funcs[addr].func_name;
    if (lifted_func_name.empty()) {
      auto &plt_code_sec = manager.elf_obj.code_sections[".plt"];
      // panic if move to .plt section
      if (plt_code_sec.vma <= addr && addr < plt_code_sec.vma + plt_code_sec.size) {
        trace_lifter.DefinePreReferedFunction(
          manager.Sub_FuncName(addr),
          manager.panic_plt_jmp_fun_name,
          remill::LLVMFunTypeIdent::VOID_VOID
        );
      } else {
        printf("[ERROR] addr \"0x%08lx\" is not function entry point.\n", addr);
        exit(EXIT_FAILURE);
      }
    } else {
      trace_lifter.DefinePreReferedFunction(
        manager.Sub_FuncName(addr), 
        lifted_func_name, 
        remill::LLVMFunTypeIdent::NULL_FUN_TY
      );
    }
  }
  
  auto host_arch =
      remill::Arch::Build(&context, os_name, remill::GetArchName(REMILL_ARCH));
  host_arch->PrepareModule(module.get());
  remill::StoreModuleToFile(module.get(), FLAGS_bc_out);

  printf("Done\n");
  return 0;
}
