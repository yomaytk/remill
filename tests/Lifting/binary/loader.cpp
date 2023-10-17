#define PACKAGE
#include "loader.h"
#include <stdlib.h>

#define ERROR_LEN 1000

using namespace BinaryLoader;

void ELFObject::OpenELF() {

	// init binary file descriptor
	if (bfd_inited) {
		bfd_init();
		bfd_inited = true;
	}

	bfd_h = bfd_openr(file_name.c_str(), nullptr);
	// confirm file_name is opened
	if (!bfd_h) {
		printf("failed to open binary file: %s, ERROR: %s\n", file_name.c_str(), bfd_errmsg(bfd_get_error()));
		abort();
	}
	// confirm file_name is an object file
	if (!bfd_check_format(bfd_h, bfd_object)) {
		printf("file \"%s\" does not look like an executable file.\n", file_name.c_str());
		abort();
	}

	bfd_set_error(bfd_error_no_error);
	// confirm file_name is an ELF binary
	if (!(bfd_get_flavour(bfd_h) == bfd_target_elf_flavour)) {
		printf("file \"%s\" is not an ELF binary.\n", file_name.c_str());
		abort();
	}
    
}

void ELFObject::LoadELF() {
	LoadELFBFD(); 
}

void ELFObject::LoadELFBFD() {
	// get binary handler
	OpenELF();
	// get entry point
	entry = bfd_get_start_address(bfd_h);
	// get binary format
	bin_type_str = std::string(bfd_h->xvec->name);
	switch(bfd_h->xvec->flavour) {
		case bfd_target_elf_flavour:
			bin_type = BIN_TYPE_ELF;
			break;
		case bfd_target_unknown_flavour:
		default:
			printf("file \"%s\" is not an ELF binary.\n", file_name.c_str());
			abort();
	}
	// get architecture
	bfd_arch_info = bfd_get_arch_info(bfd_h);
	bin_arch_str = std::string(bfd_arch_info->arch_name);
	switch (bfd_arch_info->mach){
		case bfd_mach_aarch64:
			bin_arch = BinaryArch::ARCH_AARCH64;
			bits = 64;
			break;
		case bfd_mach_x86_64:
			printf("x86_64 is not supported now.\n");
			abort();
		default:
			printf("unknown architecture\n");
			abort();
			break;
	}
	// get every static symbol table
	LoadStaticSymbolsBFD();
	// get every section
	LoadSectionsBFD();
}

void ELFObject::LoadStaticSymbolsBFD() {
    
	asymbol **bfd_symtab = nullptr;
	// get symbol table space
	long table_size = bfd_get_symtab_upper_bound(bfd_h);
	if (table_size < 0) {
		printf("failed to read symtab.\n");
		abort();
	} else if (table_size > 0) {
		bfd_symtab = reinterpret_cast<asymbol**>(malloc(table_size));
		if (!bfd_symtab) {
			printf("failed to allocate symtab memory.\n");
			abort();
		}
		// read symbol table
		long sym_num = bfd_canonicalize_symtab(bfd_h, bfd_symtab);
		if (sym_num < 0) {
			printf("failed to read symtab.\n");
			abort();
		}
		for (int i = 0;i < sym_num;i++) {
			ELFSymbol::SymbolType sym_type;
			if (bfd_symtab[i]->flags & BSF_FUNCTION || std::memcmp(bfd_symtab[i]->name, "_start", sizeof("_start")) == 0) {
					sym_type = ELFSymbol::SymbolType::SYM_TYPE_FUNC;
			} else if (bfd_symtab[i]->flags & BSF_LOCAL) {
					sym_type = ELFSymbol::SymbolType::SYM_TYPE_LVAR;
					continue;
			} else if (bfd_symtab[i]->flags & BSF_GLOBAL) {
					sym_type = ELFSymbol::SymbolType::SYM_TYPE_GVAR;
					continue;
			} else {
					continue;
			}
			symbols.emplace_back(ELFSymbol(sym_type, std::string(bfd_symtab[i]->name), bfd_asymbol_value(bfd_symtab[i])));
		}
	} else {
			printf("[INFO] static symbol table is not found.\n");
	}

}

std::tuple<uint8_t*, uint64_t, uintptr_t> ELFObject::GetTextSection() {
    
	uint8_t *text_bytes;
	uint64_t text_size;
	uintptr_t text_vma;
	bool text_found = false;

    if (sections.empty()) {
			printf("[BUG] GetTextSection is called but sections is empty\n");
			abort();
    } else {
			for (auto& section : sections) {
				if (section.sec_name.compare(".text") == 0) {
					if (text_found) {
						printf("found multiple .text section.\n");
						abort();
					}
					text_found = true;
					text_bytes = section.bytes;
					text_size = section.size;
					text_vma = section.vma;
				}
			}
    }
    return {text_bytes, text_size, text_vma};
}

std::vector<ELFObject::FuncEntry> ELFObject::GetFuncEntry() {
    
	std::vector<ELFObject::FuncEntry> func_entrys;

	if (symbols.empty()) {
		printf("[BUG] GetFuncEntry is called but symbols is empty\n");
		abort();
	} else {
		for (auto& symbol : symbols) {
			if (symbol.sym_type == ELFSymbol::SymbolType::SYM_TYPE_FUNC) {
				func_entrys.emplace_back(FuncEntry(symbol.addr, symbol.sym_name, entry == symbol.addr));
			}
		}
	}
	// descending order
	std::sort(func_entrys.rbegin(), func_entrys.rend());
	return func_entrys;

}

void ELFObject::LoadSectionsBFD() {

	asection *bfd_sec;
	
	for (bfd_sec = bfd_h->sections; bfd_sec; bfd_sec = bfd_sec->next) {
			
		ELFSection::SectionType sec_type;
		flagword bfd_flags;
		bfd_vma vma;
		bfd_size_type size;
		std::string sec_name;
		uint8_t* sec_bytes;
		
		// get bfd flags
		bfd_flags = bfd_section_flags(bfd_sec);
		if (bfd_flags & SEC_CODE) {
			sec_type = ELFSection::SEC_TYPE_CODE;
		} else if (bfd_flags & (SEC_DATA | SEC_ALLOC)) {
			sec_type = ELFSection::SEC_TYPE_DATA;
		} else if (bfd_flags & SEC_READONLY){
			sec_type = ELFSection::SEC_TYPE_READONLY;
		} else {
			sec_type = ELFSection::SEC_TYPE_UNKNOWN;
		}
		// get vma, section size, section name, section contents
		vma = bfd_section_vma(bfd_sec);
		size = bfd_section_size(bfd_sec);
		sec_name = std::string(bfd_section_name(bfd_sec));
		if (sec_name.empty()) {
			sec_name = std::string("<unnamed>");
		}
		sec_bytes = reinterpret_cast<uint8_t*>(malloc(size));
		if (!sec_bytes) {
			printf("failed to allocate section bytes.\n");
			abort();
		}
		if (!bfd_get_section_contents(bfd_h, bfd_sec, sec_bytes, 0, size)) {
			printf("failed to read and copy section bytes.\n");
			abort();
		}

		sections.emplace_back(ELFSection(this, sec_type, sec_name, vma, size, sec_bytes));
	}

}

void ELFObject::DebugBinary() {
    
	printf("[DEBUG]\n");
	printf("File Name: %s\n", file_name.c_str());
	printf("Binary Format Type: %s\n", bin_type_str.c_str());
	printf("CPU Architecture: %s\n", bin_arch_str.c_str());
	printf("Bits: %d\n", bits);
	printf("Entry Address: 0x%08lX\n", entry);
	// debug sections
	DebugSections();
	// debug static symbols
	DebugStaticSymbols();

}

void ELFObject::DebugSections() {
    
	for(auto& section : sections) {
		char sec_type[100];
		if (section.sec_type == ELFSection::SectionType::SEC_TYPE_CODE) {
			std::memcpy(sec_type, "CODE", sizeof("CODE"));
		} else if (section.sec_type == ELFSection::SectionType::SEC_TYPE_DATA) {
			std::memcpy(sec_type, "DATA", sizeof("DATA"));
		} else if (section.sec_type == ELFSection::SectionType::SEC_TYPE_READONLY) {
			std::memcpy(sec_type, "READONLY", sizeof("READONLY"));
		} else {
			std::memcpy(sec_type, "UNKNOWN", sizeof("UNKNOWN"));
		}
		printf("Section 0x%08lX\t%s\t\t%lu\t%s\n", 
			section.vma, section.sec_name.c_str(), section.size, sec_type);
	}

}

void ELFObject::DebugStaticSymbols() {

	for (auto& symbol : symbols) {
		printf("Symbol 0x%08lX\t%s\t\t%s\n", 
			symbol.addr, symbol.sym_name.c_str(), symbol.sym_type == ELFSymbol::SymbolType::SYM_TYPE_FUNC ? "FUNC" : "OTHER");
	}
}
