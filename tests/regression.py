#!/usr/bin/env python3
"""OneImage assembler/VM regression tests; Python 3.7+.

python tests/regression.py                build with cl (Windows) or c++
python tests/regression.py --tools-dir .  use existing local executables
python tests/regression.py --sanitize    build with GCC/Clang ASan + UBSan
"""
import argparse
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
PARSER = argparse.ArgumentParser(description=__doc__)
PARSER.add_argument('--tools-dir', type=Path)
PARSER.add_argument('--sanitize', action='store_true')
PARSER.add_argument('--cc', default=os.environ.get('CXX', 'cl' if os.name == 'nt' else 'c++'))
ARGS, REMAINING = PARSER.parse_known_args()


class Regression(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp = tempfile.TemporaryDirectory(prefix='oi-tests-')
        cls.work = Path(cls.temp.name)
        cls.exe = '.exe' if os.name == 'nt' else ''
        cls.tools = ARGS.tools_dir.resolve() if ARGS.tools_dir else cls.work / 'tools'
        cls.environment = dict(os.environ)
        # The assembler owns process-lifetime label tables; diagnose invalid
        # memory accesses without treating its exit-time allocations as leaks.
        cls.environment['ASAN_OPTIONS'] = 'detect_leaks=0'
        if not ARGS.tools_dir:
            cls.tools.mkdir()
            cls.build('ba', ['ba.cxx'], [])
            cls.build('oia', ['oia.c', 'oidis.c'], ['FORCETRACING'])
            for width in (2, 4, 8):
                cls.build('oios%d' % width, ['oios.c', 'oi.c', 'trace.c', 'oidis.c'], ['OI%d' % width])

    @classmethod
    def tearDownClass(cls):
        cls.temp.cleanup()

    @classmethod
    def build(cls, name, sources, defines):
        target = str(cls.tools / (name + cls.exe))
        if Path(ARGS.cc).stem.lower() == 'cl':
            command = [ARGS.cc, '/nologo', '/O2', '/EHsc', '/DNDEBUG', '/I' + str(ROOT)]
            command += ['/D' + d for d in defines]
            command += [str(ROOT / s) for s in sources] + ['/Fe:' + target]
        else:
            command = [ARGS.cc, '-std=c++11', '-O2', '-DNDEBUG', '-I', str(ROOT)]
            if ARGS.sanitize:
                command += ['-g', '-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-fno-omit-frame-pointer']
            command += ['-D' + d for d in defines]
            command += [str(ROOT / s) for s in sources] + ['-o', target]
        result = subprocess.run(command, cwd=str(cls.tools), stdout=subprocess.PIPE,
                                stderr=subprocess.STDOUT, universal_newlines=True, timeout=120)
        if result.returncode:
            raise RuntimeError(result.stdout)

    def invoke(self, tool, *arguments):
        return subprocess.run([str(self.tools / (tool + self.exe))] + list(arguments),
                              cwd=str(self.work), env=self.environment,
                              stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                              universal_newlines=True, timeout=30)

    def assemble(self, source, width, success=True, listing=False):
        (self.work / 'case.s').write_text(source)
        result = self.invoke('oia', '-w:%d' % width, *(['-l'] if listing else []), 'case.s')
        if success:
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        else:
            self.assertNotEqual(result.returncode, 0)
            self.assertIn('error:', result.stdout.lower(), result.stdout + result.stderr)
            self.assertNotIn('Sanitizer', result.stderr)
        return result

    def matrix(self, source, expected):
        for image in (2, 4, 8):
            with self.subTest(image=image):
                self.assemble(source, image)
                for native in (2, 4, 8):
                    if native < image:
                        continue
                    result = self.invoke('oios%d' % native, 'case')
                    self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                    self.assertEqual(result.stdout, expected, 'image=%d native=%d' % (image, native))
                    self.assertEqual(result.stderr, '')

    def test_existing_instruction_suite(self):
        self.matrix((ROOT / 'testoi.s').read_text(), '1 argc\ncase.oi\ndone\n')

    def test_signed_frame_offsets(self):
        source = '.code\n call frame\n halt\nframe:\n' + ' push rzero\n' * 4
        expected = ''
        for offset in (-4, -3, -2, -1):
            value = offset + 5
            source += (' ldi rarg1,%d\n stf rarg1,%d\n ldf rarg1,%d\n syscall 2\n'
                       ' pushf %d\n pop rarg1\n syscall 2\n'
                       ' ldi rarg1,%d\n cstf rarg1,rarg1,eq,%d\n ldf rarg1,%d\n syscall 2\n') % (
                           value, offset, offset, offset, value + 5, offset, offset)
            expected += '%d%d%d' % (value, value, value + 5)
        source += ' pop rzero\n' * 4 + ' ret\n.codeend\n'
        self.matrix(source, expected)
        self.assemble(source, 4, listing=True)
        listing = (self.work / 'case.lst').read_text()
        self.assertIn('pushf -4', listing)
        self.assertIn('cstf rarg1, rarg1, eq, -4', listing)

    def test_relative_loops_and_returns(self):
        for branch in ('jrel', 'jrelb'):
            self.matrix('.data\n byte pad\n image_t item\n.dataend\n.code\n'
                        ' ldi rarg2,item\n ldi rarg1,3\nloop:\n dec rarg1\n'
                        ' %s rarg1,rarg2,0,ne,loop\n syscall 2\n ret\n.codeend\n' % branch, '0')
            for target in ('ret', 'retnf', 'ret0', 'ret0nf'):
                self.matrix('.data\n string msg "unreachable"\n.dataend\n.code\n'
                            ' %s rzero,rzero,0,eq,%s\n ldi rarg1,msg\n syscall 1\n ret\n.codeend\n' % (branch,target), '')

    def test_relative_range_boundaries(self):
        for branch in ('jrel', 'jrelb'):
            for width in (2, 4, 8):
                prefix = '.code\nstart:\n'
                self.assemble(prefix + ' inc rres\n' * 128 +
                              ' %s rarg1,rarg2,0,ne,start\n ret\n.codeend\n' % branch, width)
                self.assemble(prefix + ' inc rres\n' * 129 +
                              ' %s rarg1,rarg2,0,ne,start\n ret\n.codeend\n' % branch, width, False)
                self.assemble('.code\n %s rarg1,rarg2,0,ne,end\n' % branch +
                              ' inc rres\n' * 123 + 'end:\n ret\n.codeend\n', width)
                self.assemble('.code\n %s rarg1,rarg2,0,ne,end\n' % branch +
                              ' inc rres\n' * 124 + 'end:\n ret\n.codeend\n', width, False)

    def test_indexed_native_and_byte_operations(self):
        self.matrix('.data\n image_t items[8]\n.dataend\n.code\n'
                    ' ldi rarg1,items\n ldib rarg2,1\n stoi rarg1[rarg2],-7\n'
                    ' ldor rres,rarg1[rarg2]\n mov rarg1,rres\n syscall 2\n'
                    ' ldi rarg1,items\n ldi rres,42\n stor rarg1[rarg2],rres\n'
                    ' ldor rarg1,rarg1[rarg2]\n syscall 2\n'
                    ' ldi rarg1,items\n stoib rarg1[rzero],-3\n ldorb rarg1,rarg1[rzero]\n syscall 2\n'
                    ' ldi rarg1,items\n ldib rres,6\n storb rarg1[rzero],rres\n'
                    ' ldorb rarg1,rarg1[rzero]\n syscall 2\n'
                    ' push rarg1\n ldib rres,3\n andst\n mov rarg1,rres\n syscall 2\n halt\n.codeend\n', '-742-362')

    def test_native_stadd(self):
        self.matrix('.data\n image_t items[4]\n.dataend\n.code\n'
                    ' ldi rarg1,items\n zero rres\n ldib rarg2,4\n ldib rtmp,9\n memf\n'
                    ' zero rtmp\n ldib rarg2,2\n ldib rres,3\n stadd\n'
                    ' ldor rres,rarg1[rzero]\n push rres\n ldib rarg2,1\n'
                    ' ldor rarg1,rarg1[rarg2]\n syscall 2\n pop rarg1\n syscall 2\n ret\n.codeend\n', '90')

    def test_local_basic_compiler(self):
        (self.work / 'hello.bas').write_text(
            '10 s% = 0\n20 for i% = 1 to 5\n30 s% = s% + i%\n'
            '40 next i%\n50 print "sum: "; s%\n60 end\n')
        result = self.invoke('ba', '-q', '-a:o', '-x', 'hello.bas')
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.matrix((self.work / 'hello.s').read_text(), 'sum: 15\n')

    def test_numeric_stores_and_quoted_semicolon(self):
        self.matrix('.data\n string msg "one;two\\n" ; actual comment\n.dataend\n'
                    'define where 100\n.code\n sti 100,1\n stib where,2\n'
                    ' ldi rarg1,msg\n syscall 1\n ret\n.codeend\n', 'one;two\n')

    def test_cross_width_arithmetic(self):
        for image in (2, 4, 8):
            bits = image * 8
            for operation in ('idiv', 'idivst'):
                source = '.code\n ldi rres,%d\n ldi rarg2,-1\n' % -(1 << (bits-1))
                if operation == 'idiv':
                    source += ' idiv rres,rarg2\n'
                else:
                    source += ' push rres\n mov rres,rarg2\n idivst\n'
                source += ' mov rarg1,rres\n syscall 2\n ret\n.codeend\n'
                self.assemble(source, image)
                for native in (2,4,8):
                    if native >= image:
                        result = self.invoke('oios%d' % native, 'case')
                        self.assertEqual(result.returncode, 0, result.stderr)
                        self.assertEqual(result.stdout, str(-(1 << (bits-1))))
            self.assemble('.code\n ldi rarg1,32767\n inc rarg1\n ldi rarg2,2\n'
                          ' idiv rarg1,rarg2\n syscall 2\n ret\n.codeend\n', image)
            for native in (2,4,8):
                if native >= image:
                    self.assertEqual(self.invoke('oios%d' % native,'case').stdout,
                                     '-16384' if image==2 else '16384')

    def test_ldiw_high_label_and_ldi(self):
        for width in (4,8):
            prefix = '.data\n byte pad[40000]\n byte marker\n.dataend\n.code\n'
            self.assemble(prefix+' ldiw rarg1,marker\n ret\n.codeend\n',width,False)
            self.assemble(prefix+' ldi rarg1,marker\n syscall 2\n ret\n.codeend\n',width)
            result=self.invoke('oios%d'%width,'case')
            self.assertEqual(result.returncode,0,result.stderr)
            self.assertGreater(int(result.stdout),40000)

    def test_assembler_limits_and_reserved_operands(self):
        self.assemble('.code\n'+' inc rres\n'*2100+'.codeend\n',2,False)
        self.assemble('.data\n'+''.join(' string s%d "%s"\n'%(i,'x'*150) for i in range(230))+
                      '.dataend\n.code\n ret\n.codeend\n',2,False)
        self.assemble('.code\n'+'x'*220+':\n ret\n.codeend\n',2,False)
        for reg in ('rzero','rpc','rsp'):
            self.assemble('.code\n zero %s\n ret\n.codeend\n'%reg,2,False)
        self.assemble('.code\n ldi rres,foo@bar\n ret\n.codeend\n',2,False)


if __name__ == '__main__':
    unittest.main(argv=[sys.argv[0]]+REMAINING)
