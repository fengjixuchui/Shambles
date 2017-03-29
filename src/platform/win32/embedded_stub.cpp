#include "embedded_stub.hpp"
#include <Windows.h>
#include <iostream>

/* create embedded stub section */
#pragma section(SECT_EXE32, read, execute, write)

/*
__declspec(allocate(SECT_EXE32))
stub_configuration __stub_config = {
	0xDEADBEEF,
	0xDEADBEEF
};*/


/* stub main */
#pragma code_seg(SECT_EXE32)
extern "C" void __declspec(naked)  __stub_main(void)
{ 
	__asm {
		mov eax, 0xAAAAAAAA; //image base
		mov ecx, 0xBBBBBBBB; //oep
		mov ebx, 0xCCCCCCCC; //key
	}

	uint32_t image_base;
	uint32_t oep;
	__asm {
		mov image_base, eax;
		mov oep, ecx;
	}

	oep = oep + image_base;
	__asm {
		jmp oep;
	}
	//((int(*)(void))configs.oep)();
}


