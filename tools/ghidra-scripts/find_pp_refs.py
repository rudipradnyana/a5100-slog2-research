# find_pp_refs.py — Ghidra headless postScript (Jython)
# Locates PP-related strings, all references (incl. movw/movt pairs and literal pools),
# decompiles containing functions.
from ghidra.app.decompiler import DecompInterface
from ghidra.util.task import ConsoleTaskMonitor
from ghidra.program.model.scalar import Scalar
import os

monitor = ConsoleTaskMonitor()
prog = currentProgram
af = prog.getAddressFactory().getDefaultAddressSpace()
mem = prog.getMemory()
fm = prog.getFunctionManager()
rm = prog.getReferenceManager()
listing = prog.getListing()

patterns = [
    ('PP_FORLIRO_ERR', 'SetPictureProfileForLiro NON SUPPORTED'),
    ('GAMMA_UNKNOWN_ERR', 'PARAMID_PP_GAMMA_TYPE UNKNOWN'),
    ('GAMMA_NULL_ERR', 'PARAMID_PP_GAMMA_TYPE is NULL'),
    ('SLOG2_NAME', 'VALUEID_PP_GAMMA_SLOG2'),
    ('CINEMATONE_FORLIRO_ERR', 'SetCinematoneModeForLiro NON SUPPORTED'),
]

found = {}
rev = {}
for label, pat in patterns:
    addr = mem.findBytes(af.getAddress(0), pat, None, True, monitor)
    if addr is None:
        print('MISS  %s' % label)
        continue
    found[label] = addr
    rev[addr.getOffset()] = label
    print('FOUND %s @ %s' % (label, addr))

tset = set(rev.keys())

# --- 1. Reference manager hits ---
ref_hits = []   # (label, fromAddr, how)
for label, addr in found.items():
    for r in rm.getReferencesTo(addr):
        ref_hits.append((label, r.getFromAddress(), 'refmgr'))
print('REFMGR total: %d' % len(ref_hits))
for label, a, how in ref_hits:
    f = fm.getFunctionContaining(a)
    print('  [%s] %s  func=%s' % (label, a, f))

# --- 2. Instruction scan: movw/movt pairs + literal pools ---
regw = {}
ins_hits = []  # (label, insAddr, how)
count = 0
for ins in listing.getInstructions(True):
    count += 1
    if monitor.isCancelled():
        break
    try:
        mn = ins.getMnemonicString()
        addr = ins.getAddress().getOffset()
        # any immediate write to a register = potential movw low half
        if len(ins.getOpObjects()) >= 2:
            dst = ins.getOpObjects(0)
            src = ins.getOpObjects(1)
            if len(dst) == 1 and len(src) == 1 and isinstance(src[0], Scalar):
                # movt?
                if mn == 'movt':
                    reg = dst[0]
                    if reg in regw:
                        wa, wv = regw[reg]
                        if 0 < addr - wa < 0x80:
                            full = (src[0].getUnsignedValue() << 16) | wv
                            if full in tset:
                                ins_hits.append((rev[full], ins.getAddress(), 'movw/movt @ %x' % wa))
                elif src[0].getUnsignedValue() < 0x10000:
                    regw[dst[0]] = (addr, src[0].getUnsignedValue())
        # literal pool load
        if mn.startswith('ldr'):
            for ref in ins.getReferencesFrom():
                to = ref.getToAddress().getOffset()
                if to in tset:
                    ins_hits.append((rev[to], ins.getAddress(), 'direct pool'))
                else:
                    try:
                        v = mem.getInt(af.getAddress(to)) & 0xffffffff
                        if v in tset:
                            ins_hits.append((rev[v], ins.getAddress(), 'literal @ %x' % to))
                    except:
                        pass
    except:
        pass
print('INSTRUCTIONS scanned: %d' % count)
print('INSTRUCTION hits: %d' % len(ins_hits))
for label, a, how in ins_hits:
    f = fm.getFunctionContaining(a)
    print('  [%s] %s via %s  func=%s' % (label, a, how, f))

# --- 3. Decompile containing functions ---
funcs = {}
for label, a, how in ref_hits + ins_hits:
    f = fm.getFunctionContaining(a)
    if f is not None:
        funcs.setdefault(f.getEntryPoint().getOffset(), (f, []))
        funcs[f.getEntryPoint().getOffset()][1].append((label, a, how))

outdir = '/tmp/pp_decomp'
try:
    os.makedirs(outdir)
except:
    pass

ifc = DecompInterface()
ifc.openProgram(prog)
for ep in sorted(funcs.keys()):
    f, hits = funcs[ep]
    res = ifc.decompileFunction(f, 240, monitor)
    print('### FUNC %s @ %#x  hits=%s' % (f.getName(), ep, hits[:8]))
    if res.decompileCompleted():
        code = res.getDecompiledFunction().getC()
        path = os.path.join(outdir, 'func_%x.c' % ep)
        fh = open(path, 'w')
        fh.write(code)
        fh.close()
        lines = code.split('\n')
        print('    decompiled %d lines -> %s' % (len(lines), path))
        for ln in lines[:25]:
            print('    | ' + ln)
    else:
        print('    decompile FAILED: %s' % res.getErrorMessage())
print('DONE')
