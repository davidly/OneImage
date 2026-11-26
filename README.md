# OneImage

OneImage is a prototype ISA that's more efficient than popular ISAs

Details can be found here: [OneImage](https://medium.com/@davidly_33504/oneimage-towards-a-more-perfect-cpu-5ea8b418cd5e)

In short, CPUs with instruction sets similar to the one proposed will run code more efficiently than Arm64, x64, and RISC-V 64. The ISA is easier to target with compilers. Also, because it has fewer registers, fewer transistors would be required.

Two functions in tttoi.s as compared with assembly implementations for other ISAs found [here](https://github.com/davidly/ttt)

![counts](https://github.com/user-attachments/assets/df3ffa27-c77e-4134-b737-101e1eff5859)

The manx, hisoft, and mscv6 folders contain build scripts for compiling oios with CP/M 2.2 and DOS compilers found in [cpm_compilers](https://github.com/davidly/cpm_compilers) and [dos_compilers](https://github.com/davidly/dos_compilers) using NTVCM and NTVDM emulators found in sister repos.


Assembler and interpreter regression tests require Python 3.7 or newer and a C++ compiler:

```sh
python tests/regression.py
```

On Windows, run this from an MSVC developer command prompt. On Linux or macOS,
the default compiler is `c++`; use `--cc clang++` to select another compiler.
Use `--tools-dir .` to test existing `oia` and `oios2`, `oios4`, and `oios8`
executables, or `--sanitize` with GCC/Clang to enable address and undefined-behavior
checks. Tests cover all supported image/interpreter width combinations.


The local BASIC compiler is built from `ba.cxx` and its included `ba8008.hxx`
backend. Run `./mba.sh` on Linux/macOS (a C++11 compiler is required), or
`mba.bat` from an MSVC developer command prompt on Windows. Compile a BASIC
sample to OneImage assembly with `./ba -q -a:o -x basic/tneg1.bas`.
The regression runner builds and tests this local BA automatically; when using
`--tools-dir`, that directory must also contain `ba`. The legacy `runall` and
`runfull` scripts also use the local BA executable, which must be built first.
