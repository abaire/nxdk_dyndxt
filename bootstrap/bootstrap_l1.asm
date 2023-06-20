; Trivial loader that simply invokes the given procedure address passing
; (DWORD@io_address, DYNAMIC_DXT_POOL).
;
; This is intended to be used for two things:
; 1) When given the address of DmAllocatePoolWithTag, allocates a block of
;    memory, with the size stored in the last 4 bytes of this binary, then
;    stores the address of the allocated block in the last 4 bytes and returns a
;    XBDM facility (0x2DB) SUCCESS result.
;
; 2) Once the dynamic loader DLL has been loaded, the entrypoint address is
;    given and [io_address] is set to 0 to indicate an entrypoint call. The
;    entrypoint is invoked and the HRESULT returned.

bits 32

%include "macros.asm.inc"

%define DYNAMIC_DXT_POOL 'txDD'

org 0x0000
section .text

; HRESULT bootstrap(void *DmAllocatePoolWithTagAddress)
bootstrap:

    push ebp
    mov ebp, esp

    ; The address of DmAllocatePoolWithTag or the loader entrypoint.
    mov edx, dword [ebp+8]

    RELOCATE_ADDRESS ecx, io_address
    push ecx

    ; Check to see if this is attempting to allocate memory or just call the
    ; HRESULT __stdcall DxtMain(void).
    cmp dword [ecx], 0x00000000
    jz call_entrypoint

    ; DmAllocatePoolWithTag(<size>, DYNAMIC_DXT_POOL);
    push DYNAMIC_DXT_POOL
    push dword [ecx]

call_entrypoint:
    call edx

    ; Restore io_address
    pop ecx

    ; Disable memory protection to allow writing within this code segment.
    mov edx, cr0
    push edx
    and edx, 0xFFFEFFFF
    mov cr0, edx

    mov [ecx], eax

    ; Reenable memory protection
    pop edx
    mov cr0, edx

    ; Return a successful HRESULT
    mov eax, 0x02DB0000

    mov esp, ebp
    pop ebp
    ret 4

; Reserve space for the address at which the requested size will be read, and
; the resultant address will be written.
section .data
align 4
io_address DD 0
