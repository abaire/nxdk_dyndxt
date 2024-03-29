%ifndef HANDLER_LOADER_BOOTSTRAP_MACROS_ASM_INC_
%define HANDLER_LOADER_BOOTSTRAP_MACROS_ASM_INC_

; HRESULT facility field for XBOX results
%define FACILITY_XBOX 0x2db
%define DEF_SUCCESS(code) ((FACILITY_XBOX << 16) + (code))
%define DEF_ERROR(code) (0x80000000 + (FACILITY_XBOX << 16) + (code))

%define S_OK DEF_SUCCESS(0x00)
%define S_MULTILINE DEF_SUCCESS(0x02)
%define S_BINARY DEF_SUCCESS(0x03)
%define S_SEND_BINARY DEF_SUCCESS(0x04)

; Used to terminate binary data transfers, prefixed with S as it's an expected
; termination condition rather than a typical error.
%define S_NO_MORE_DATA DEF_ERROR(0x104)

%define E_UNEXPECTED DEF_ERROR(0x00)
%define E_MAX_CONNECTIONS_EXCEEDED DEF_ERROR(0x01)
%define E_FILE_NOT_FOUND DEF_ERROR(0x02)
%define E_NO_SUCH_MODULE DEF_ERROR(0x03)
%define E_MEMORY_NOT_MAPPED DEF_ERROR(0x04)
%define E_FAIL DEF_ERROR(0x05)
%define E_UNKNOWN_COMMAND DEF_ERROR(0x07)
%define E_NOT_STOPPED DEF_ERROR(0x08)
%define E_FILE_MUST_BE_COPIED DEF_ERROR(0x09)
%define E_EXISTS DEF_ERROR(0x0A)
%define E_DIRECTORY_NOT_EMPTY DEF_ERROR(0x0B)
%define E_FILENAME_INVALID DEF_ERROR(0x0C)
%define E_CREATE_FILE_FAILED DEF_ERROR(0x0D)
%define E_ACCESS_DENIED DEF_ERROR(0x0E)
%define E_TYPE_INVALID DEF_ERROR(0x11)
%define E_DATA_NOT_AVAILABLE DEF_ERROR(0x12)
%define E_DEDICATED_CONNECTION_REQUIRED DEF_ERROR(0x16)

; RELOCATE_ADDRESS(register, label)
; Set the given register to the absolute address of the given label.
%macro RELOCATE_ADDRESS 2
            call %%geteip
%%geteip:   pop %1
            add %1, %2 - %%geteip
%endmacro

%endif  ; HANDLER_LOADER_BOOTSTRAP_MACROS_ASM_INC_
