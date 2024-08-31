# turn off deprecated warnings since this code also needs to build on CP/M 2.2

OS=${OSTYPE//[0-9.]/}

echo $OS

set staticflag=
if [[ "$OS" != 'darwin' ]]; then
    staticflag=-static
fi

g++ -Wno-deprecated -ggdb -Ofast -fno-builtin -D FORCETRACING -D NDEBUG -I . oia.c oidis.c -o oia $staticflag
