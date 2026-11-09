import sys
import numpy as np
# import matplotlib.pyplot as plt

def analyze_diff(before_path, after_path, base_addr=0x92ec0000):
    print(f"Loading {before_path} and {after_path}...")
    a = np.fromfile(before_path, dtype=np.uint8)
    b = np.fromfile(after_path, dtype=np.uint8)
    
    if len(a) != len(b):
        print(f"Warning: File size mismatch ({len(a)} vs {len(b)})! Truncating to min.")
        min_len = min(len(a), len(b))
        a = a[:min_len]
        b = b[:min_len]
        
    diff_indices = np.nonzero(a != b)[0]
    total_diff_bytes = len(diff_indices)
    print(f"Total different bytes: {total_diff_bytes} / {len(a)} ({total_diff_bytes/len(a)*100:.2f}%)")
    
    if total_diff_bytes == 0:
        print("[-] Identical dumps! No changes detected.")
        return
        
    # Group contiguous diff regions (gap <= 64 bytes)
    regions = []
    if total_diff_bytes > 0:
        start = diff_indices[0]
        prev = diff_indices[0]
        for idx in diff_indices[1:]:
            if idx - prev > 64:
                regions.append((start, prev + 1))
                start = idx
            prev = idx
        regions.append((start, prev + 1))
        
    print(f"\nFound {len(regions)} contiguous changed regions:")
    lut_candidates = []
    for s, e in regions:
        length = e - s
        phys_addr = base_addr + s
        print(f"  Offset: 0x{s:06x} - 0x{e:06x} (Phys: 0x{phys_addr:08x}) | Length: {length} bytes")
        
        # Check if length matches common LUT sizes (512, 1024, 2048, 4096 bytes)
        if 256 <= length <= 8192:
            lut_candidates.append((s, e, phys_addr, length))
            
    print("\n--- LUT CANDIDATE ANALYSIS ---")
    for s, e, phys, l in lut_candidates:
        chunk_a = a[s:e]
        chunk_b = b[s:e]
        print(f"\nAnalyzing Candidate @ Phys: 0x{phys:08x} (Size: {l} bytes):")
        
        # Try evaluating as uint16 LE entries
        if l % 2 == 0:
            u16_a = np.frombuffer(chunk_a, dtype='<u2')
            u16_b = np.frombuffer(chunk_b, dtype='<u2')
            
            # Check monotonicity (naik teratur)
            is_mono_a = np.all(np.diff(u16_a) >= 0)
            is_mono_b = np.all(np.diff(u16_b) >= 0)
            
            print(f"  u16 entries: {len(u16_a)}")
            print(f"  Range Before: min={np.min(u16_a)}, max={np.max(u16_a)} | Monotonic: {is_mono_a}")
            print(f"  Range After : min={np.min(u16_b)}, max={np.max(u16_b)} | Monotonic: {is_mono_b}")
            
            if is_mono_b or (np.max(u16_b) <= 4096 and np.min(u16_b) >= 0):
                print(f"  [>>> JACKPOT CANDIDATE: LOOKS LIKE A REAL HARDWARE TONE CURVE LUT! <<<]")

if __name__ == '__main__':
    if len(sys.argv) < 3:
        print("Usage: python3 memdiff_lut.py <lut_baseline.bin> <lut_changed.bin>")
    else:
        analyze_diff(sys.argv[1], sys.argv[2])

