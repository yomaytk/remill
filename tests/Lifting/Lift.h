#include <gflags/gflags.h>
#include <glog/logging.h>
#include <gtest/gtest.h>
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

#include "remill/BC/IntrinsicTable.h"
#include "remill/BC/Lifter.h"
#include "remill/BC/Util.h"
#include "remill/Arch/Arch.h"
#include "remill/Arch/Instruction.h"
#include "remill/Arch/Runtime/Intrinsics.h"
#include "remill/Arch/Name.h"
#include "remill/OS/OS.h"
#include "remill/Arch/AArch64/Runtime/State.h"
#include "remill/Arch/Runtime/Runtime.h"
#include "binary/loader.h"

#ifdef __APPLE__
#  define SYMBOL_PREFIX "_"
#else
#  define SYMBOL_PREFIX ""
#endif

#define ARY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#define AARCH_OP_SIZE 4

class DisasmFunc;

class AArch64TraceManager : public remill::TraceManager {
  public:
  
    virtual ~AArch64TraceManager(void) = default;
    AArch64TraceManager(std::string target_elf_file_name): elf_obj(BinaryLoader::ELFObject(target_elf_file_name)) {}

    void SetLiftedTraceDefinition(uint64_t addr, llvm::Function *lifted_func);
    llvm::Function *GetLiftedTraceDeclaration(uint64_t addr);
    llvm::Function *GetLiftedTraceDefinition(uint64_t addr);
    bool TryReadExecutableByte(uint64_t addr, uint8_t *byte);

    void SetELFData();

    std::unordered_map<uintptr_t, uint8_t> memory;
    std::unordered_map<uint64_t, llvm::Function *> traces;
    std::vector<DisasmFunc> disasm_funcs;
    std::string entry_func_lifted_name;

  private:
    BinaryLoader::ELFObject elf_obj;
    uintptr_t entry_point;
    uint8_t *text_bytes;
    uint64_t text_size;
    uintptr_t text_vma;
    
};

class DisasmFunc {
  public:
    DisasmFunc(std::string func_name_, uintptr_t begin_, uint64_t insn_size_): func_name(func_name_), begin(begin_), insns_size(insn_size_) {}

    std::string func_name;
    uintptr_t begin;
    uint64_t insns_size;
};
