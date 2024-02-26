#pragma once

void logInfo(const char* fmt, ...);

//S_PRESENT_OCCLUDED

#ifdef _DEBUG
#define DX_CHECK_RETURN(F, L) do {\
		HRESULT res = (F);\
		if(res != S_OK){\
			logInfo("DX error %lx / %u at line %d\n", (unsigned int)(res), ( unsigned int )( res & 0xFFFF ), (int)(L));\
			__debugbreak();\
		}\
	} while(0);

#else

#define DX_CHECK_RETURN(F, L) (F);

#endif

#define DX_RET(F) DX_CHECK_RETURN((F), __LINE__)