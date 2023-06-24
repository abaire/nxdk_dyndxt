; Bootloader that provides a new XBDM debug command processor that can load a
; binary image into some address and invoke it's DxtMain method.

bits 32

%include "macros.asm.inc"

%define DYNAMIC_DXT_POOL 'txDD'

org 0x0000
section .text

; Contains contextual information used when a debug command processor needs to
; do something more than immediately reply with a simple, short response.
struc   CommandContext

  ; Function to be invoked to actually send or receive data. This function
  ; will be called repeatedly until `bytes_remaining` is set to 0.
  ;
  ; typedef HRESULT_API_PTR(ContinuationProc)(struct CommandContext *ctx,
  ;                                           char *response,
  ;                                           DWORD response_len)
  .handler:         resd    1

  ; When receiving data, this will be set to the number of bytes in `buffer`
  ; that contain valid received data. When sending data, this must be set by
  ; the debug processor to the number of valid bytes in `buffer` that should
  ; be sent.
  .data_size:       resd    1

  ; Buffer used to hold send/receive data. XBDM will create a small buffer by
  ; default, this can be reassigned to an arbitrary buffer allocated by the
  ; debug processor, provided it is cleaned up.
  .buffer:          resd    1

  ; The size of `buffer` in bytes.
  .buffer_size:     resd    1

  ; Arbitrary data defined by the command processor.
  .user_data:       resd    1

  ; Used when sending a chunked response, indicates the number of bytes
  ; remaining to be sent. `handler` will be called repeatedly until this is
  ; set to 0, indicating completion of the send.
  .bytes_remaining: resd    1

endstruc


; ===========================================================================
; HRESULT bootstrap_l2()
bootstrap_l2:
    RELOCATE_ADDRESS ecx, ProcessCommand
    push ecx
    RELOCATE_ADDRESS ecx, g_handler_name
    push ecx
    RELOCATE_ADDRESS edx, DmRegisterCommandProcessor
    call [edx]

    ret 0


; ===========================================================================
; char *strchr(const char *str, char find)
strchr:
     push       ebp
     mov        ebp, esp

     mov        dl, byte [ebp + 12]     ; find
     mov        ecx, dword [ebp + 8]    ; str
     dec        ecx

 .loop:
     mov        dh, byte [ecx + 1]      ; *str
     inc        ecx                     ; ++str
     test       dh, dh                  ; if (!*str) goto .done
     je         .done

     cmp        dh, dl                  ; if (*str != find) goto .loop
     jne        .loop

 .done:
     xor        eax, eax

     test       dh, dh                  ; if (!*str) return str
     cmovne     eax, ecx

     test       dl, dl                  ; if (!find) return str
     cmove      eax, ecx

     pop        ebp
     ret


; ===========================================================================
; void strcpy(char *dst, const char *src)
;
; Trashes eax, ecx, edx
strcpy:
    mov edx, [esp + 8]
    mov cl, byte [edx]
    test cl, cl
    mov eax, [esp + 4]
    je .terminate

.loop:
    mov byte [eax], cl
    mov cl, byte[edx + 1]
    inc eax
    inc edx
    test cl, cl
    jne .loop

.terminate:
    mov byte [eax], 0
    ret 8


; ===========================================================================
; int32_t parse_integer(char **esi)
;   Parses the string in [esi] as a positive hex value, prefixed with 0x.
;   esi is incremented to point at the first non-integer character. If esi is
;   not incremented, the value is not a valid hex integer.
;
; Trashes: eax, ecx, esi
parse_integer:
    ; Check that the number starts with 0x
    mov cx, word [esi]
    cmp cx, '0x'
    jne .done
    add esi, 2

    xor eax, eax
    xor ecx, ecx

.loop:
    mov cl, byte [esi]
    test ecx, ecx
    jz .done

    cmp ecx, '0'
    jl .done

    cmp ecx, '9'
    jg .hex
    sub ecx, '0'
    jmp .accumulate

.hex:
    cmp ecx, 'A'
    jl .done

    cmp ecx, 'F'
    jg .lowercase_hex
    sub ecx, 'A' - 10
    jmp .accumulate

.lowercase_hex:
    cmp ecx, 'a'
    jl .done

    cmp ecx, 'f'
    jg .lowercase_hex
    sub ecx, 'a' - 10

.accumulate:
    shl eax, 4
    add eax, ecx

    inc esi
    jmp .loop

.done:
    ret

; ===========================================================================
; integer_to_hex(eax: value, edi: destination)
;   Converts eax into a hex string, stored in [edi]
;
; Trashes: eax, ecx, edx, edi
integer_to_hex:
    push ebx
    RELOCATE_ADDRESS ebx, g_hex_chars

    mov ecx, eax

    mov word [edi], '0x'
    add edi, 2
    mov edx, edi  ; Save the start of the hex string for reversal later.

.loop:
    test ecx, ecx
    jz .reverse

    mov eax, ecx
    and eax, 0x0F
    shr ecx, 4

    xlatb
    mov [edi], al
    inc edi
    jmp .loop

.reverse:
    mov byte [edi], 0 ; Terminate the string, then point at the last valid char.
    dec edi

.reverse_loop:
    mov al, [edx]
    xchg al, [edi]
    mov [edx], al
    inc edx
    dec edi
    cmp edx, edi
    jl .reverse_loop

.done:
    pop ebx
    ret
g_hex_chars                 DB '0123456789abcdef'


; ===========================================================================
; parse_arg
;  Inputs:
;    esi: The input buffer to parse
;    ecx: The argument specifier
;
;  Returns:
;    eax: The parsed value
;    ecx: 0 if an argument was successfully parsed
;
;  Trashes:
;    eax, ecx, esi (incremented past the entire argument on match.)
parse_arg:
    push esi

    repe cmpsb
    jne .arg_mismatch

    test ecx, ecx
    jne .arg_mismatch

    pop eax  ; Discard esi, the arg matched and should be consumed.
    push esi
    call parse_integer

    pop ecx  ; Check that esi was incremented.
    test edi, ecx
    jz .bad_value

    xor ecx, ecx  ; Clear ecx to flag success
    jmp .done

.bad_value:
    mov ecx, 1
    jmp .done

.arg_mismatch:
    mov ecx, 1
    pop esi

.done:
    ret

%defstr BASE_ARG            b=
%strlen BASE_ARG_LEN        BASE_ARG
g_base_arg                  DB BASE_ARG, 0

%defstr ORDINAL_ARG         o=
%strlen ORDINAL_ARG_LEN     ORDINAL_ARG
g_ordinal_arg               DB ORDINAL_ARG, 0

%defstr SIZE_ARG            s=
%strlen SIZE_ARG_LEN        SIZE_ARG
g_size_arg                  DB SIZE_ARG, 0
%defstr ENTRYPOINT_ARG      e=
%strlen ENTRYPOINT_ARG_LEN  ENTRYPOINT_ARG
g_entrypoint_arg            DB ENTRYPOINT_ARG, 0

; ===========================================================================
; HRESULT_API ProcessCommand(const char *command,
;                            char *response,
;                            DWORD response_len,
;                            struct CommandContext *ctx)
ProcessCommand:
    push ebp
    mov ebp, esp
    push esi
    push edi
    push ebx

    mov esi, [ebp + 8]
    add esi, 5  ; Skip "ldxt!"
    lodsb

    cmp al, 'i'
    je .HandleInstall

    cmp al, 'a'
    je .HandleAlloc

    cmp al, 'r'
    jne .invalid_command

.HandleResolve:
    ; Resolve one or more module exports.
    inc esi  ; Skip the 'r' command and move to the args.

    ; Optimistically construct the CommandContext.
    push DYNAMIC_DXT_POOL
    push 4 * 128
    RELOCATE_ADDRESS ecx, DmAllocatePoolWithTag
    call [ecx]

    mov edx, [ebp + 20]
    mov ebx, eax
    mov [edx + CommandContext.user_data], eax
    mov [edx + CommandContext.buffer], eax
    mov [edx + CommandContext.buffer_size], dword 4 * 128
    mov [edx + CommandContext.bytes_remaining], dword 0

.loop_base_or_ordinal:
    RELOCATE_ADDRESS edi, g_base_arg
    mov ecx, BASE_ARG_LEN
    call parse_arg

    test ecx, ecx
    jnz .read_ordinal

    ; edx = image_base
    mov edx, eax

.check_space:
    mov cl, byte [esi]
    cmp cl, ' '
    jne .check_end

.skip_space:
    inc esi
    jmp .loop_base_or_ordinal

.read_ordinal:
    test edx, edx ; Check that some base > 0 was previously given.
    jz .free_buffer_and_fail_usage_resolve

    RELOCATE_ADDRESS edi, g_ordinal_arg
    mov ecx, ORDINAL_ARG_LEN

    call parse_arg
    test ecx, ecx
    jnz .free_buffer_and_fail_usage_resolve

    ; Ordinals must be > 0
    test eax, eax
    jz .free_buffer_and_fail_usage_resolve

    call GetExportAddress
    add ebx, 4

    ; Increment bytes_remaining past the new value
    mov ecx, [ebp + 20]
    mov eax, [ecx + CommandContext.bytes_remaining]
    add eax, 4
    mov [ecx + CommandContext.bytes_remaining], eax

    jmp .check_space

.check_end:
    test cl, cl  ; ecx must be set to [esi]
    jnz .free_buffer_and_fail_usage_resolve

    mov edx, [ebp + 20]
    ; Set ctx->handler to the SendResolvedExports proc.
    RELOCATE_ADDRESS ecx, SendResolvedExports
    mov [edx + CommandContext.handler], ecx

    mov eax, S_BINARY
    jmp .end

.free_buffer_and_fail_usage_resolve:

    ; Free the optimistically allocated buffer
    mov edx, [ebp + 20]
    mov ecx, [edx + CommandContext.user_data]
    push ecx
    RELOCATE_ADDRESS ecx, DmFreePool
    call [ecx]

    ; Null out the CommandContext
    mov edx, [ebp + 20]
    mov [edx + CommandContext.user_data], dword 0
    mov [edx + CommandContext.buffer], dword 0
    mov [edx + CommandContext.buffer_size], dword 0
    mov [edx + CommandContext.bytes_remaining], dword 0

    jmp .fail_usage_resolve

.HandleAlloc:
    ; Allocate space on the debug heap for a future HandleInstall command.
    inc esi  ; Skip the 'a' command and move to the args.
    RELOCATE_ADDRESS edi, g_size_arg
    mov ecx, SIZE_ARG_LEN
    call parse_arg

    test ecx, ecx
    jnz .fail_usage_alloc

    test eax, eax ; Check that some value > 0 was given.
    jz .fail_usage_alloc

    ; Save the to-be-allocated buffer size into the context.
    mov edx, [ebp + 20]
    mov [edx + CommandContext.buffer_size], eax

    ; Allocate eax bytes on the debug heap to receive the image.
    ; DmAllocatePoolWithTag(<size>, DYNAMIC_DXT_POOL);
    push DYNAMIC_DXT_POOL
    push eax
    RELOCATE_ADDRESS ecx, DmAllocatePoolWithTag
    call [ecx]

    test eax, eax
    jz .fail_out_of_memory

    RELOCATE_ADDRESS ecx, g_image  ; Save the allocated buffer in g_image.
    mov [ecx], eax

    ; Compose the OK response.
    push eax
    %defstr MESSAGE_OK         Ready to receive binary data. base=
    %strlen MESSAGE_OK_LEN     MESSAGE_OK
    RELOCATE_ADDRESS ecx, g_message_ok
    push ecx
    mov edi, dword [ebp + 12]
    push edi
    call strcpy

    mov edi, eax ; eax is the address of the null terminator.
    pop eax
    call integer_to_hex

    mov eax, S_OK
    jmp .end

.HandleInstall:
    ; Install the given binary payload and call the DxtMain at the given addr.
    inc esi  ; Skip the 'i' command and move to the args.

    RELOCATE_ADDRESS edi, g_entrypoint_arg
    mov ecx, ENTRYPOINT_ARG_LEN
    call parse_arg

    test ecx, ecx
    jnz .fail_usage_install

    test eax, eax ; Check that some value > 0 was given.
    jz .fail_usage_install

    ; Save the entrypoint to g_image_entrypoint.
    RELOCATE_ADDRESS ecx, g_image_entrypoint
    mov [ecx], eax

    mov edx, [ebp + 20]  ; CommandContext *ctx

    ; Set ctx->buffer to the previously allocated buffer.
    RELOCATE_ADDRESS ecx, g_image
    mov eax, [ecx]
    mov [edx + CommandContext.buffer], eax

    ; Set ctx->bytes_remaining to ctx->buffer_size.
    mov eax, [edx + CommandContext.buffer_size]
    mov [edx + CommandContext.bytes_remaining], eax

    ; Set ctx->handler to the ReceiveImageData proc.
    RELOCATE_ADDRESS ecx, ReceiveImageData
    mov [edx + CommandContext.handler], ecx

    mov eax, S_SEND_BINARY
    jmp .end

.fail_out_of_memory:
    RELOCATE_ADDRESS ecx, g_message_out_of_memory
    push ecx
    push dword [ebp + 12]
    call strcpy
    mov eax, E_ACCESS_DENIED
    jmp .end

.fail_usage_resolve:
    RELOCATE_ADDRESS ecx, g_message_usage_resolve
    push ecx
    push dword [ebp + 12]
    call strcpy
    jmp .fail

.fail_usage_alloc:
    RELOCATE_ADDRESS ecx, g_message_usage_alloc
    push ecx
    push dword [ebp + 12]
    call strcpy
    jmp .fail

.fail_usage_install:
    RELOCATE_ADDRESS ecx, g_message_usage_install
    push ecx
    push dword [ebp + 12]
    call strcpy

.fail:
    mov eax, E_FAIL
    jmp .end

.invalid_command:
    mov eax, E_UNKNOWN_COMMAND

.end:
    pop ebx
    pop edi
    pop esi
    mov esp, ebp
    pop ebp
    ret 16

; ===========================================================================
; HRESULT_API SendResolvedExports(struct CommandContext *ctx,
;                                 char *response,
;                                 DWORD response_len)
SendResolvedExports:
    push ebp
    mov ebp, esp

    mov edx, [ebp + 8]
    mov ecx, [edx + CommandContext.bytes_remaining]
    test ecx, ecx
    jz .done

    ; Set data_size to bytes_remaining, the entire buffer can be sent.
    mov [edx + CommandContext.data_size], ecx
    mov [edx + CommandContext.bytes_remaining], dword 0

    mov eax, S_OK
    jmp .end

.done:
    ; Free the allocated buffer.
    mov edx, [ebp + 8]
    mov [edx + CommandContext.data_size], dword 0
    mov ecx, [edx + CommandContext.user_data]
    push ecx
    RELOCATE_ADDRESS ecx, DmFreePool
    call [ecx]

    mov eax, S_NO_MORE_DATA

.end:
    mov esp, ebp
    pop ebp
    ret 12


; ===========================================================================
; HRESULT_API ReceiveImageData(struct CommandContext *ctx,
;                              char *response,
;                              DWORD response_len)
ReceiveImageData:
    push ebp
    mov ebp, esp

    mov edx, [ebp + 8]
    mov ecx, [edx + CommandContext.data_size]
    test ecx, ecx
    jz .error_empty

    ; buffer += data_size
    mov eax, [edx + CommandContext.buffer]
    add eax, ecx
    mov [edx + CommandContext.buffer], eax

    ; buffer_size -= data_size
    mov eax, [edx + CommandContext.buffer_size]
    sub eax, ecx
    mov [edx + CommandContext.buffer_size], eax

    ; bytes_remaining -= data_size
    mov eax, [edx + CommandContext.bytes_remaining]
    sub eax, ecx
    mov [edx + CommandContext.bytes_remaining], eax

    ; if bytes_remaining return S_OK
    test eax, eax
    jz .receive_completed
    mov eax, S_OK
    jmp .end

.receive_completed:

    ; Call DXTMain()
    RELOCATE_ADDRESS eax, g_image_entrypoint
    call [eax]
    jmp .end

.error_empty:
    ; Unclear if this can ever happen. Presumably it'd indicate some sort of
    ; error and the image_base block should be freed.
    RELOCATE_ADDRESS ecx, g_message_install_failed
    push ecx
    mov eax, dword [ebp + 12]
    push eax
    call strcpy

    mov eax, E_UNEXPECTED

.end:
    mov esp, ebp
    pop ebp
    ret 12


; ===========================================================================
; GetExportAddress
;  Resolves an export and stores the resolved address into [ebx].
;  Assumes that ordinal and base are both > 0
;
;  Inputs:
;    edx: The base of the image.
;    eax: The ordinal of the export to resolve.
;
;  Returns:
;    eax: 0 if the export was successfully resolved.
;
;  Trashes:
;    eax, ecx
GetExportAddress:
    push edi
    push esi
    ; Get the PE header pointer.
    %define kPEHeaderPointer    0x3C
    mov ecx, edx
    add ecx, kPEHeaderPointer

    ; Get the offset of the export table from the base.
    %define kExportTableOffset  0x78
    mov ecx, [ecx]
    add ecx, edx
    add ecx, kExportTableOffset

    ; edi = export_table_base
    mov edi, edx
    add edi, [ecx]

    ; esi = export_count
    %define kExportNumFunctionsOffset   0x14
    mov ecx, edi
    add ecx, kExportNumFunctionsOffset
    mov esi, [ecx]

    ; eax = index into the table (the ordinal - 1)
    dec eax
    cmp eax, esi
    jge .invalid_ordinal

    %define kExportDirectoryAddressOfFunctionsOffset    0x1C
    mov ecx, edi
    add ecx, kExportDirectoryAddressOfFunctionsOffset

    ; esi = &function_address
    mov esi, [ecx]
    add esi, edx
    shl eax, 2
    add esi, eax

    ; eax = function_address
    mov eax, [esi]
    add eax, edx

    ; *ebx = function_address, eax = 0
    mov [ebx], eax
    xor eax, eax
    jmp .end

.invalid_ordinal:
    mov eax, 1
    mov [ebx], dword 0

.end:
    pop esi
    pop edi
    ret


section .data
align 4

g_image:                    DD 0  ; The received loader image.
g_image_entrypoint:         DD 0  ; The DXTMain function within the loaded image.

g_message_install_failed    DB 'Install failed', 0
g_message_out_of_memory     DB 'Out of memory', 0
g_message_ok                DB MESSAGE_OK, 0
g_message_usage_resolve     DB 'Usage "r [b=<base_hex> [o=<ordinal>]]"', 0
g_message_usage_alloc       DB 'Usage "a s=<size_hex>"', 0
g_message_usage_install     DB 'Usage "i e=<entrypoint_hex>"', 0
g_handler_name              DB 'ldxt', 0

; Reserve space for the import table.
DmFreePool                  DD 0
DmAllocatePoolWithTag       DD 0
DmRegisterCommandProcessor  DD 0
