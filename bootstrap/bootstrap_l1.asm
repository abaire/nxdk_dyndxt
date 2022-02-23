; Trivial loader that simply allocates a fixed size block of memory, stores the address of the block in the last 4 bytes
; of the program, and returns a SUCCESS HRESULT.

bits 32

%include "macros.asm.inc"

%define DYNAMIC_DXT_POOL 'txDD'

org 0x0000
section .text

; HRESULT bootstrap(void *DmAllocatePoolWithTagAddress)
bootstrap:

    push ebp
    mov ebp, esp

    ; The address of DmAllocatePoolWithTag
    mov edx, dword [ebp+8]

    relocate_address ecx, io_address
    push ecx

    ; DmAllocatePoolWithTag(BOOTSTRAP_STAGE_2_SIZE, <size>);
    push DYNAMIC_DXT_POOL
    push dword [ecx]
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

; Reserve space for the address at which the requested size will be read, and the resultant address will be written.
section .data
align 4
io_address DD 0
