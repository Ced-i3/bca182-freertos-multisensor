"""
pre:build_compiler.py
PlatformIO pre-build script that:
1. Adds MinGW to the subprocess PATH so gcc/g++ are found.
2. Forces the console subsystem linker flag so MinGW produces
   a console application (main entry point) instead of a Windows
   GUI application (WinMain entry point).
"""
Import("env")

env.AppendENVPath("PATH", r"C:\Tools\mingw64\bin")
# Force console subsystem — overrides MinGW's default Windows subsystem
# so the linker looks for main() instead of WinMain().
env.Append(LINKFLAGS=["-Wl,--subsystem,console"])
