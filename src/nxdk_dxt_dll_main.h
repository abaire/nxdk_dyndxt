#ifndef DYNDXT_LOADER_NXDK_DXT_DLL_MAIN_H
#define DYNDXT_LOADER_NXDK_DXT_DLL_MAIN_H

#include "xbdm.h"

#ifdef __cplusplus
extern "C" {
#endif

extern HRESULT DXTMain(void);

extern void __cdecl __security_init_cookie(void);
extern void _PDCLIB_xbox_libc_init(void);
extern void _PDCLIB_xbox_run_pre_initializers(void);
extern void _PDCLIB_xbox_run_crt_initializers(void);
extern void _PDCLIB_xbox_libc_deinit(void);
extern void exit(int status);
extern IMAGE_TLS_DIRECTORY_32 _tls_used;

// Defined to satisfy requirement in pdclib/.../crt0.c, but this will never
// actually be called.
int main(int argc, char **argv) { return 0; }

// Iniitalizes the nxdk CRT and delegates to DxtMain.
HRESULT __attribute__((dllexport, no_stack_protector, stdcall)) DXTMainCRTStartup() {
  // The security cookie needs to be initialized as early as possible, and from
  // a non-protected function
  __security_init_cookie();

  DWORD tlssize;
  // Sum up the required amount of memory, round it up to a multiple of
  // 16 bytes and add 4 bytes for the self-reference
  tlssize = (_tls_used.EndAddressOfRawData - _tls_used.StartAddressOfRawData) +
            _tls_used.SizeOfZeroFill;
  tlssize = (tlssize + 15) & ~15;
  tlssize += 4;
  *((DWORD *)_tls_used.AddressOfIndex) = (int)tlssize / -4;

  _PDCLIB_xbox_libc_init();
  _PDCLIB_xbox_run_pre_initializers();

  _PDCLIB_xbox_run_crt_initializers();
  return DXTMain();
}

void DLLMainShutdown(void) { _PDCLIB_xbox_libc_deinit(); }

#ifdef __cplusplus
};  // extern "C"
#endif

#endif  // DYNDXT_LOADER_NXDK_DXT_DLL_MAIN_H
