# OneImage

OneImage is a prototype ISA that's more efficient than popular ISAs

Details can be found here: [OneImage](https://medium.com/@davidly_33504/oneimage-towards-a-more-perfect-cpu-5ea8b418cd5e)

In short, CPUs with instruction sets similar to the one proposed will run code more efficiently than Arm64, x64, and RISC-V 64. The ISA is easier to target with compilers. Also, because it has fewer registers, fewer transistors would be required.

Two functions in tttoi.s as compared with assembly implementations for other ISAs found [here](https://github.com/davidly/ttt)

![counts](https://github.com/user-attachments/assets/df3ffa27-c77e-4134-b737-101e1eff5859)

The manx, hisoft, and mscv6 folders contain build scripts for compiling oios with CP/M 2.2 and DOS compilers found in (cpm_compilers)[https://github.com/davidly/cpm_compilers] and (dos_compilers)[https://github.com/davidly/dos_compilers] using NTVCM and NTVDM emulators found in sister repos.
