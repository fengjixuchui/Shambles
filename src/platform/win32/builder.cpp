#include "builder.hpp"
#include <string>
#include <iostream>
#include <iomanip>
#include <Windows.h>
#include <fstream>
#include <pe_bliss.h>
#include "schemes.hpp"
#include "tealib.hpp"
#include "embedded_stub.hpp"

using namespace pe_bliss;
using namespace std;

/* check that the given address is within the address limits of a given section */
bool IsAddressWithinSection(uintptr_t address, section sec)
{
	uintptr_t start_addr = sec.get_virtual_address() + (uintptr_t)GetModuleHandle(0);
	uintptr_t end_addr = sec.get_virtual_address() + sec.get_virtual_size() + (uintptr_t)GetModuleHandle(0);

	if (address >= start_addr && address <= end_addr) return true;
	else return false;
}

uintptr_t FindSectionByName( pe_base *image, const char *section_name, section &found_section ) 
{	
	/* iterate through all sections until we find section by name */
	section_list sections = image->get_image_sections();
	for each (section sec in sections)
	{
		if (std::strcmp(sec.get_name().c_str(), section_name) == 0)
		{
			found_section = sec;

			/* get absolute address of the section */
			HMODULE hModule = GetModuleHandle(NULL);
			uintptr_t pimage_dos_header = (uintptr_t)hModule;
			uint32_t abs_addr = pimage_dos_header + sec.get_virtual_address();

			return abs_addr;
		}
	}
	return NULL;
}

/* find embedded stub section */
uintptr_t FindEmbeddedStub( pe_base *image, section &stub_result )
{
	uint16_t num_sections = image->get_number_of_sections();
	PIMAGE_SECTION_HEADER stub_section = nullptr;

	/* determine embedded stub's section name */
	const char* stub_name = SECT_EXE32;

	/* iterate through sections until we find the embedded stub */
	section stub;
	uintptr_t psection = FindSectionByName(image, stub_name, stub);
	if (psection == NULL) {
		wcout << endl << "[ERROR] Could not find an embedded stub.\n";
		return NULL;
	}
	
	wcout << "------Found Embedded Stub------\n";
	wcout << "SizeOfRawData: "			<< stub.get_size_of_raw_data() << endl;
	wcout << "VirtualSize: "			<< stub.get_virtual_size() << endl;
	wcout << "VirtualAddress: "			<< "0x" << hex << stub.get_virtual_address() << endl;
	wcout << "Absolute Address: "		<< "0x" << hex << psection << endl;
	stub_result = stub;
	return psection;
}

int Build(const char *filepath, BuildConfigurations build_configs)
{
	/* get target path info */
	string filepath_str	= string(filepath);
	string ext				= string(filepath_str).substr(filepath_str.find_last_of(".") + 1);
	string output_path		= string(filepath_str).substr(0, filepath_str.find_last_of(".")) + string(".") + ext;

	/* get host path info */
	TCHAR host_path[MAX_PATH];
	GetModuleFileName(NULL, (LPWSTR)&host_path, MAX_PATH);

	/* load target image */
	ifstream pe_file(filepath, ios::in | ios::binary);
	pe_base target_image(pe_factory::create_pe(pe_file));

	/* validate image type (native executable for now only) */
	if (!target_image.is_exe() || target_image.is_dotnet()) {
		wcout << endl << "[ERROR] Unsupported image type." << endl;
		pe_file.close();
		return UNSUPPORTED_IMAGETYPE;
	}
	/* validate image architecture (32-bit for now only) */
	if (target_image.get_pe_type() != pe_type_32) {
		wcout << endl << "[ERROR] Unsupported target architecture. " << endl;
		pe_file.close();
		return UNSUPPORTED_ARCH;
	}

	/* search for an embedded stub in our own host binary */
	ifstream host_pe(host_path, ios::in | ios::binary);
	pe_base host_image(pe_factory::create_pe(host_pe));
	section stub_section;
	uintptr_t pstub_code = FindEmbeddedStub(&host_image, stub_section);
	if (pstub_code == NULL) {
		return ERROR_STUB;
	}

	/* check that the address of the stub's entry point points directly to the real method, 
	   as opposed to a jump table.*/
	if (!IsAddressWithinSection((uintptr_t)__stub_main, stub_section)) {
		// host binary was compiled with 'Incremental Linking' so the stub's main startup function cannot be found
		// solution: recompile the binary in 'Release' mode or set the /INCREMENTAL:NO flag in msvc
		wcout << endl	<< "[FAILURE] Unable to locate stub's startup address." << endl;
		return ERROR_BAD_HOST_BINARY;
	}

	/* perform encryption */
	char key[256];
	for (int i = 0; i < sizeof(key); i++) { key[i] = rand() % 256; }
	{
		//encrypt image sections
		section_list &image_sections = target_image.get_image_sections();
		for (section &s : image_sections)
		{
			//ignore tls for now
			if (std::strcmp(s.get_name().c_str(), ".tls") == 0) {
				wcout << "[WARNING] TLS section is ignored." << endl;
			}
			else 
			{
				string& section_raw_data = s.get_raw_data();
				for (int ptr = 0; ptr < s.get_size_of_raw_data(); ptr += 2) {
					tea_encrypt((uint32_t*)&section_raw_data[6], (uint32_t*)key);
				}
				break;
			}
		}
	}
	target_image.remove_directory(IMAGE_DIRECTORY_ENTRY_DEBUG);

	/* prepare embedded stub for injection into the target image */
	{
		wcout << endl << "--------Preparing Stub----------" << endl;
		
		//let stub be readable and executable
		stub_section.readable(true).executable(true).writeable(true);

		//set stub configuration
		//stub_configuration stub_config = { 0 };
		//stub_config.num_sections = target_image.get_number_of_sections();
		//stub_config.oep = target_image.get_ep();

		//write stub's raw data
		string& stub_data = stub_section.get_raw_data();
		//memcpy(&stub_data[0], &stub_config, sizeof(stub_config));
		
		//overwrite addresses / temporary
		*((uintptr_t*)&stub_data[1]) = target_image.get_image_base_32();
		*((uintptr_t*)&stub_data[1+5]) = target_image.get_ep();
		*((uintptr_t*)&stub_data[1 + 10]) = (uintptr_t)key;

		//print out log
		wcout << "[CONFIG] Image Base Offset: " << target_image.get_image_base_32() << endl;
		wcout << "Size of raw data: " << stub_section.get_size_of_raw_data() << endl;
		wcout << "Raw data:" << endl;
		for (int i = 0; i < stub_section.get_size_of_raw_data(); i++) {
			wcout << hex << setfill(L'0') << setw(2) << (uint8_t)(stub_data.at(i)) << " ";
		}
	}

	/* inject stub */
	wcout << endl  << endl << "--------Injecting Stub----------" << endl;
	section& injected_stub = target_image.add_section(stub_section);
	

	/* set new image entry point to the stub's startup function */
	{
		uint32_t old_ep = target_image.get_ep();
		uint32_t new_ep = ((uintptr_t)(void*)__stub_main - stub_section.get_virtual_address() - (uintptr_t)GetModuleHandle(NULL)) + injected_stub.get_virtual_address();
		target_image.set_ep(new_ep);
		wcout << "New Entry Point: " << "0x" << hex << new_ep << endl;
	}

	/* iat */
	/*
	const imported_functions_list imports = get_imported_functions(target_image);

	for (imported_functions_list::const_iterator it = imports.begin(); it != imports.end(); ++it)
	{
		const import_library& lib = *it;
		wcout << "Library [" << lib.get_name().c_str() << "]" << std::endl 
			<< "Timestamp: " << lib.get_timestamp() << std::endl 
			<< "RVA to IAT: " << lib.get_rva_to_iat() << std::endl 
			<< "========" << std::endl;

		const import_library::imported_list& functions = lib.get_imported_functions();
		for (import_library::imported_list::const_iterator func_it = functions.begin(); func_it != functions.end(); ++func_it)
		{
			const imported_function& func = *func_it; 
			wcout << "[+] ";
			if (func.has_name()) 
				wcout << func.get_name().c_str();
			else
				wcout << "#" << func.get_ordinal(); 

			wcout << " hint: " << func.get_hint() << std::endl;
		}

		wcout << std::endl;
	}
	*/


	/* save patched, target image */
	ofstream new_file(output_path.c_str(), ios::out | ios::binary | ios::trunc);
	if (!new_file) {
		cout << endl << "[ERROR] Failed to create: " << output_path << endl;
		pe_file.close();
		return ERROR_CREATE_NEW_FILE;
	}
	rebuild_pe(target_image, new_file);

	return BUILD_SUCCESS;
}