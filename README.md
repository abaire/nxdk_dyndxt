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

