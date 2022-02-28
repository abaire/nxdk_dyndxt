# dyndxt_loader

Installs an XBDM command processor that can be used to load static libraries built with the
[nxdk](https://github.com/XboxDev/nxdk) at runtime. This is intended to be safer than using
actual dxt executables, which are automatically loaded by the debug kernel and can prevent
the system from booting fully.

Intended for use with [xbdm_gdb_bridge](https://github.com/abaire/xbdm_gdb_bridge)'s
`@bootstrap` command.

A demonstration of how to build a DLL loadable by this loader can be found at
[abaire/nxdk_demo_dyndxt](https://github.com/abaire/nxdk_demo_dyndxt)

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

# Building additional plugins

The dyndxt_loader loader is intended to be used with DLLs that provide a
`_DXTMain` entrypoint. The top level `CMakeLists.txt` builds the dyndxt_loader
in this manner and can be used as a template.

# Design

*Technique inspired by https://github.com/XboxDev/xboxpy*

A two stage bootstrapping process is used to load the dyndxt_loader itself,
generally leveraging XBDM's `setmem` method to overwrite a host function that
may then be invoked via the normal XBDM command process.

The host function chosen is `DmResumeThread`, which is invoked more or less
verbatim by the `resume` XBDM command. This method takes a single DWORD
parameter and returns an HRESULT.

## L1 bootstrap

The L1 bootstrap operates in two modes, depending on the value stored in the
last 4 bytes (`io_val`) of the loaded image.

### Allocation mode: `io_val != 0`
Allocates an XBDM memory block via `DmAllocatePoolWithTag` (address provided via
`resume` `thread` argument). The size of the block is specified by the `io_val`
and is set to the number of bytes to allocate before invoking. `io_val` is
then updated to contain the results of the allocation for retrieval via
`getmem`.

### Entrypoint mode: `io_val == 0`
Calls the given nullary function, which is generally expected to be the
`DxtMain` entrypoint after loading and relocating the loader DLL.

## Dynamic DXT Loader

Interaction with the dyndxt_loader is accomplished via XBDM commands with the
`ddxt!` (Dynamic DXT loader) prefix.

E.g., "ddxt!hello" will return a dump of known method exports if the loader has
been installed successfully. "dxt!load" can be used to load a new DXT DLL,
etc...
