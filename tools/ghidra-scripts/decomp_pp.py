from ghidra.app.decompiler import DecompInterface
from ghidra.util.task import ConsoleTaskMonitor
import os

prog = currentProgram
base = prog.getImageBase().getOffset()
print('image base = %#x' % base)
af = prog.getAddressFactory().getDefaultAddressSpace()
fm = prog.getFunctionManager()
ifc = DecompInterface()
ifc.openProgram(prog)
mon = ConsoleTaskMonitor()
os.makedirs('/tmp/pp_decomp', exist_ok=True)

targets = [0x951680, 0x951900, 0x3f9b06, 0x3f8c5c, 0x5375d4, 0x954380, 0x3f9af8]
for t in targets:
    a = af.getAddress(base + t)
    f = fm.getFunctionContaining(a)
    print('== target %#x -> %s' % (t, f))
    if f is None:
        continue
    res = ifc.decompileFunction(f, 600, mon)
    if res.decompileCompleted():
        c = res.getDecompiledFunction().getC()
        p = '/tmp/pp_decomp/f_%x.c' % f.getEntryPoint().getOffset()
        fh = open(p, 'w')
        fh.write(c)
        fh.close()
        print('   saved %s (%d chars)' % (p, len(c)))
    else:
        print('   DECOMP FAIL: %s' % res.getErrorMessage())
print('SCRIPT DONE')
