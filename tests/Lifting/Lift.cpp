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

void AArch64TraceManager::SetELFData() {

  elf_obj.LoadELF();
  entry_point = elf_obj.entry;
  // set text section
  auto [__text_bytes, __text_size, __text_vma] = elf_obj.GetTextSection();
  text_bytes = __text_bytes;
  text_size = __text_size;
  text_vma = __text_vma;
  // set symbol table (WARNING: only when the ELF binary is not stripped)
  auto func_entrys = elf_obj.GetFuncEntry();
  // set instructions to the buf of manager
  uintptr_t func_limit_addr = reinterpret_cast<uintptr_t>(text_vma + text_size);
  for (auto i = 0;i < func_entrys.size();i++) {
    uint64_t func_bytes_size = func_limit_addr - func_entrys[i].entry;
    auto disasm_func = DisasmFunc(func_entrys[i].func_name + "__Lifted", func_entrys[i].entry, func_bytes_size);
    // find the function of entry point
    if (entry_point == func_entrys[i].entry) {
      if (!entry_func_lifted_name.empty()) {
        printf("multiple entrypoints are found.\n");
        abort();
      }
      entry_func_lifted_name = disasm_func.func_name;
    }
    // assign every insn to the manager
    for (uintptr_t addr = func_entrys[i].entry;addr < func_limit_addr; addr++) {
      memory[addr] = text_bytes[addr - text_vma];
    }
    disasm_funcs.emplace_back(disasm_func);
    func_limit_addr = func_entrys[i].entry;
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

  for (auto &asm_func : manager.disasm_funcs) {
    // lifting every disasm function
    if (trace_lifter.Lift(asm_func.begin)) {
      printf("[INFO] Lifted func: %s\n", asm_func.func_name.c_str());
    } else {
      printf("[ERROR] Failed to Lift \"%s\"\n", asm_func.func_name.c_str());
      abort();
    }
    // set function name
    auto lifted_trace = manager.GetLiftedTraceDefinition(asm_func.begin);
    lifted_trace->setName(asm_func.func_name.c_str());
  }
  
  // set entry point
  if (manager.entry_func_lifted_name.empty()) {
    printf("[ERROR] We couldn't find entry function.\n");
    abort();
  } else {
    trace_lifter.SetEntryPoint(manager.entry_func_lifted_name);
  }
  
  auto host_arch =
      remill::Arch::Build(&context, os_name, remill::GetArchName(REMILL_ARCH));
  host_arch->PrepareModule(module.get());
  remill::StoreModuleToFile(module.get(), FLAGS_bc_out);

  printf("Done\n");
  return 0;
}
