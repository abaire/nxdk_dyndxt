# dyndxt_loader

Installs an XBDM command processor that can be used to load static libraries built with the
[nxdk](https://github.com/XboxDev/nxdk) at runtime. This is intended to be safer than using
actual dxt executables, which are automatically loaded by the debug kernel and can prevent
the system from booting fully.

Intended for use with [xbdm_gdb_bridge](https://github.com/abaire/xbdm_gdb_bridge)'s
`@bootstrap` command.

# git hooks

This project uses [git hooks](https://git-scm.com/book/en/v2/Customizing-Git-Git-Hooks)
to automate some aspects of keeping the code base healthy.

Please copy the files from the `githooks` subdirectory into `.git/hooks` to
enable them.

# Building

## `CLion`

The CMake target can be configured to use the toolchain from the nxdk:

* CMake options

    `-DCMAKE_TOOLCHAIN_FILE=<absolute_path_to_nxdk>/share/toolchain-nxdk.cmake`

* Environment

    `NXDK_DIR=<absolute_path_to_nxdk>`

# Design

*Technique inspired by https://github.com/XboxDev/xboxpy*

A two stage bootstrapping process is used to load the dyndxt_loader itself,
generally leveraging XBDM's `setmem` method to overwrite a host function that
may then be invoked via the normal XBDM command process.

The host function chosen is `DmResumeThread`, which is invoked more or less
verbatim by the `resume` XBDM command. This method takes a single DWORD
parameter and returns an HRESULT.

## L1 bootstrap

The L1 bootstrap simply allocates an XBDM memory block via
`DmAllocatePoolWithTag`. Since the address of `DmAllocatePoolWithTag` is
provided via the `resume` argument, the L1 loader reserves a block of memory at
the end of the image to use for I/O. Specifically it must be set to the number
of bytes to allocate before invoking, and is updated to contain the results of
the allocation for retrieval via `getmem`

## Loading the dyndxt_loader

The dyndxt_loader itself is intended to be treated like a DLL, the COFF objects
in the library are loaded individually via the L1 bootstrap with relocations
applied once the final addresses of the sections are known.

Once loading is completed, the L1 bootstrap should be replaced with the L2
thunk and invoked via `resume` with the address of the dyndxt_loader's
entrypoint.

At this point, loading is completed and the original `DmResumeThread` memory can
be replaced.

Interaction with the dyndxt_loader is accomplished via XBDM commands with the
`bl2!` (boot loader 2) prefix.

E.g., "bl2!hello" will return a canned response if the loader has been installed
successfully.
