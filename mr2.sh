OS=${OSTYPE//[0-9.]/}
set staticflag=
if [[ "$OS" != 'darwin' ]]; then
    staticflag=-static
fi

g++ -Wno-tautological-constant-out-of-range-compare -Wno-deprecated -Wno-return-type -ggdb -Ofast -fno-builtin -D OI2 -D NDEBUG -I . oios.c oi.c trace.c oidis.c -o oios2 $staticflag
