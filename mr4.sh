OS=${OSTYPE//[0-9.]/}
set staticflag=
if [[ "$OS" != 'darwin' ]]; then
    staticflag=-static
fi

g++ -Wno-deprecated -Wno-return-type -ggdb -Ofast -fno-builtin -D OI4 -D NDEBUG -I . oios.c oi.c trace.c oidis.c -o oios4 $staticflag
