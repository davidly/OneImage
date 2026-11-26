# OneImage

OneImage is a prototype ISA that's more efficient than popular ISAs

Details can be found here: [OneImage](https://medium.com/@davidly_33504/oneimage-towards-a-more-perfect-cpu-5ea8b418cd5e)

In short, CPUs with instruction sets similar to the one proposed will run code more efficiently than Arm64, x64, and RISC-V 64. The ISA is easier to target with compilers. Also, because it has fewer registers, fewer transistors would be required.

Two functions in tttoi.s as compared with assembly implementations for other ISAs found [here](https://github.com/davidly/ttt)

![counts](https://github.com/user-attachments/assets/df3ffa27-c77e-4134-b737-101e1eff5859)

The manx, hisoft, and mscv6 folders contain build scripts for compiling oios with CP/M 2.2 and DOS compilers found in [cpm_compilers](https://github.com/davidly/cpm_compilers) and [dos_compilers](https://github.com/davidly/dos_compilers) using NTVCM and NTVDM emulators found in sister repos.


Build all host tools with a C++11 compiler (GCC or Clang on Linux/macOS,
MSVC on Windows). The build creates the BASIC compiler `ba`, assembler `oia`,
and the 16-, 32-, and 64-bit interpreters `oios2`, `oios4`, and `oios8`.

On Linux/macOS:

```sh
./mall.sh
python3 tests/regression.py --tools-dir .
```

Set `CXX=clang++` to use Clang. On Windows, open an MSVC developer command
prompt and run:

```bat
mall.bat
python tests\regression.py --tools-dir .
```

The build-all scripts stop if any component fails. To rebuild one tool, use
`mba`, `mar`, `mr2`, `mr4`, or `mr8` with the platform's `.sh` or `.bat` suffix.
The shell scripts can also be invoked from outside the repository.

Tests require Python 3.7 or newer. Omit `--tools-dir .` to compile fresh tools
in a temporary directory instead. On Linux/macOS, use
`python3 tests/regression.py --sanitize` for address and undefined-behavior
checks with GCC/Clang, or `--cc clang++` to select a test compiler.
The tests include local BASIC compilation and every supported image/interpreter
width combination.

For example, compile and run a BASIC sample on Linux/macOS:

```sh
./ba -q -a:o -x basic/tneg1.bas
./oia -w:2 basic/tneg1.s
./oios2 basic/tneg1
```

On Windows, use the same commands without `./` and with Windows path separators.
BA is built from `ba.cxx` and its included `ba8008.hxx` backend.
The legacy `runall.sh` and `runall.bat` runners also use the local BA, but require
an external `diff` command and a `baseline_test_oios.txt` file, which is not
included in this repository. Use the Python regression runner for a self-contained
test of a checkout.
