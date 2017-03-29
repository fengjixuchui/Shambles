#ifndef __BUILDER_H_
#define __BUILDER_H_

	typedef struct __BuildConfigurations 
	{
	} BuildConfigurations;

	enum BuilderResult 
	{
		BUILD_SUCCESS = 0,
		UNSUPPORTED_IMAGETYPE,
		UNSUPPORTED_ARCH,
		ERROR_STUB,
		ERROR_CREATE_NEW_FILE,
		ERROR_BAD_HOST_BINARY
	};

	/* build target with embedded stub, for now only */
	int Build(const char *filepath, BuildConfigurations build_configs);

#endif // __BUILDER_H_