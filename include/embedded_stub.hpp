#ifndef __EMBEDDED_STUB_H_
#define __EMBEDDED_STUB_H_
#include <stdint.h>

/* internal section names for embedded stub */
#define SECT_EXE32 ".exe32"

/* stub info */
struct stub_configuration {
	uint8_t num_sections;	//number of sections
	uintptr_t oep;			//original target entry point
};

//extern stub_configuration __stub_config;
extern "C"  void __stub_main(void);

#endif