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

#include <gflags/gflags.h>
#include <glog/logging.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/GlobalValue.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <map>
#include <memory>
#include <sstream>
#include <string>

#include "remill/Arch/Arch.h"
#include "remill/Arch/Instruction.h"
#include "remill/Arch/Name.h"
#include "remill/BC/IntrinsicTable.h"
#include "remill/BC/Lifter.h"
#include "remill/BC/Util.h"
#include "remill/OS/OS.h"

#ifdef __APPLE__
#  define SYMBOL_PREFIX "_"
#else
#  define SYMBOL_PREFIX ""
#endif

#define ARY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

DEFINE_string(bc_out, "",
              "Name of the file in which to place the generated bitcode.");

DEFINE_string(os, REMILL_OS,
              "Operating system name of the code being "
              "translated. Valid OSes: linux, macos, windows, solaris.");
DEFINE_string(arch, REMILL_ARCH,
              "Architecture of the code being translated. "
              "Valid architectures: x86, amd64 (with or without "
              "`_avx` or `_avx512` appended), aarch64, aarch32");

#include "TARGET/asm1.c"

struct State;
struct Memory;

namespace {

class AArch64TraceManager : public remill::TraceManager {
 public:
  virtual ~AArch64TraceManager(void) = default;

  void SetLiftedTraceDefinition(uint64_t addr,
                                llvm::Function *lifted_func) override {
    traces[addr] = lifted_func;
  }

  llvm::Function *GetLiftedTraceDeclaration(uint64_t addr) override {
    auto trace_it = traces.find(addr);
    if (trace_it != traces.end()) {
      return trace_it->second;
    } else {
      return nullptr;
    }
  }

  llvm::Function *GetLiftedTraceDefinition(uint64_t addr) override {
    return GetLiftedTraceDeclaration(addr);
  }

  bool TryReadExecutableByte(uint64_t addr, uint8_t *byte) override {
    auto byte_it = memory.find(addr);
    if (byte_it != memory.end()) {
      *byte = byte_it->second;
      return true;
    } else {
      return false;
    }
  }

 public:
  std::unordered_map<uintptr_t, uint8_t> memory;
  std::unordered_map<uint64_t, llvm::Function *> traces;
};

class AssembleFunction {
  public:
    AssembleFunction(std::string func_name_, uintptr_t begin_, uint64_t insn_size_): func_name(func_name_), begin(begin_), insns_size(insn_size_) {}
    std::string GetLiftedName() { return func_name + "_lifted"; }

    std::string func_name;
    uintptr_t begin;
    uint64_t insns_size;
};

extern "C" {
  extern uint8_t* __entry0;
  extern uint8_t* __mhex;
}

}  // namespace

extern "C" int main(int argc, char *argv[]) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);

  AArch64TraceManager manager;

  // Add all code byts from the test cases to the memory.
  std::vector<AssembleFunction*> asm_func_list = {};
  auto asm_func = AssembleFunction(std::string("BB1"), reinterpret_cast<uintptr_t>(BB1), ARY_SIZE(BB1));
  asm_func_list.emplace_back(&asm_func);

  for (auto &asm_func : asm_func_list) {
    for (auto addr = asm_func->begin; addr < asm_func->begin + asm_func->insns_size ;addr++) {
      manager.memory[addr] = *reinterpret_cast<uint8_t*>(addr);
    }
  }

  llvm::LLVMContext context;
  auto os_name = remill::GetOSName(REMILL_OS);
  auto arch_name = remill::GetArchName(FLAGS_arch);
  auto arch = remill::Arch::Build(&context, os_name, arch_name);
  auto module = remill::LoadArchSemantics(arch.get());

  remill::IntrinsicTable intrinsics(module.get());
  remill::TraceLifter trace_lifter(arch.get(), manager);

  // execute lifting
  for (auto &asm_func : asm_func_list) {
    if (!trace_lifter.Lift(asm_func->begin)) {
      printf("Failed to Lift \"%s\"\n", asm_func->func_name);
      exit(EXIT_FAILURE);
    }

    printf("Lifted func: %s\n", asm_func->GetLiftedName().c_str());

    auto lifted_trace = manager.GetLiftedTraceDefinition(asm_func->begin);
    lifted_trace->setName(asm_func->GetLiftedName().c_str());
  }
  
  auto host_arch =
      remill::Arch::Build(&context, os_name, remill::GetArchName(REMILL_ARCH));
  host_arch->PrepareModule(module.get());
  remill::StoreModuleToFile(module.get(), FLAGS_bc_out);

  printf("Done\n");
  return 0;
}
