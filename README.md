# C-Wizard

Tool to encode LPC10 audio into tms5220 audio stream.

# Notice

Inspired by python_wizard and BlueWizard:

- https://github.com/ptwz/python_wizard
- https://github.com/patrick99e99/BlueWizard

# LPC10 library

Contains header-only library for encoding/decoding audio streams called: `lpc10_enc_dec.h`.

# Building

Tested on:
## Windows 10, Visual Studio 2022 msvc version 19.39.33523 for x64

0. Open terminal and run vcvars64.exe to setup environment.
1. execute:
```cmd
full_rebuild.bat rel
```
2. done!

to build debug version run `full_rebuild.bat` without `rel`.

## Arch linux, to be tested...
