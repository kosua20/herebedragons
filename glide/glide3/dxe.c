#include <sys/dxe.h>

extern_asm(___dj_stderr);
extern_asm(___dj_stdout);
extern_asm(___djgpp_base_address);
extern_asm(___djgpp_nearptr_disable);
extern_asm(___djgpp_nearptr_enable);
extern_asm(___dpmi_free_physical_address_mapping);
extern_asm(___dpmi_physical_address_mapping);
extern_asm(__crt0_startup_flags);
extern_asm(_atof);
extern_asm(_atoi);
extern_asm(_exit);
extern_asm(_fclose);
extern_asm(_fflush);
extern_asm(_fgetc);
extern_asm(_fgets);
extern_asm(_fopen);
extern_asm(_fprintf);
extern_asm(_fread);
extern_asm(_getc);
extern_asm(_getenv);
extern_asm(_malloc);
extern_asm(_memcpy);
extern_asm(_pow);
extern_asm(_printf);
extern_asm(_puts);
extern_asm(_sprintf);
extern_asm(_sscanf);
extern_asm(_strcat);
extern_asm(_strcmp);
extern_asm(_strcpy);
extern_asm(_strlen);
extern_asm(_strtok);
extern_asm(_vfprintf);

DXE_EXPORT_TABLE_AUTO (___dxe_eta___glide3x)
	DXE_EXPORT_ASM (___dj_stderr)
	DXE_EXPORT_ASM (___dj_stdout)
	DXE_EXPORT_ASM (___djgpp_base_address)
	DXE_EXPORT_ASM (___djgpp_nearptr_disable)
	DXE_EXPORT_ASM (___djgpp_nearptr_enable)
	DXE_EXPORT_ASM (___dpmi_free_physical_address_mapping)
	DXE_EXPORT_ASM (___dpmi_physical_address_mapping)
	DXE_EXPORT_ASM (__crt0_startup_flags)
	DXE_EXPORT_ASM (_atof)
	DXE_EXPORT_ASM (_atoi)
	DXE_EXPORT_ASM (_exit)
	DXE_EXPORT_ASM (_fclose)
	DXE_EXPORT_ASM (_fflush)
	DXE_EXPORT_ASM (_fgetc)
	DXE_EXPORT_ASM (_fgets)
	DXE_EXPORT_ASM (_fopen)
	DXE_EXPORT_ASM (_fprintf)
	DXE_EXPORT_ASM (_fread)
	DXE_EXPORT_ASM (_getc)
	DXE_EXPORT_ASM (_getenv)
	DXE_EXPORT_ASM (_malloc)
	DXE_EXPORT_ASM (_memcpy)
	DXE_EXPORT_ASM (_pow)
	DXE_EXPORT_ASM (_printf)
	DXE_EXPORT_ASM (_puts)
	DXE_EXPORT_ASM (_sprintf)
	DXE_EXPORT_ASM (_sscanf)
	DXE_EXPORT_ASM (_strcat)
	DXE_EXPORT_ASM (_strcmp)
	DXE_EXPORT_ASM (_strcpy)
	DXE_EXPORT_ASM (_strlen)
	DXE_EXPORT_ASM (_strtok)
	DXE_EXPORT_ASM (_vfprintf)
DXE_EXPORT_END
