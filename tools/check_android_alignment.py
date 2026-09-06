"""Read ELF program headers directly; fail if a 64-bit APK library is not 16KB aligned."""
import pathlib
import struct
import sys

checked = 0
for lib in pathlib.Path(sys.argv[1]).rglob("*.so"):
    data = lib.read_bytes()
    if data[:4] != b"\x7fELF":
        raise SystemExit(f"Not ELF: {lib}")
    if data[4] != 2:
        continue  # 16KB requirement applies to the 64-bit Android ABIs.
    endian = "<" if data[5] == 1 else ">"
    offset = struct.unpack_from(endian + "Q", data, 32)[0]
    size, count = struct.unpack_from(endian + "HH", data, 54)
    loads = 0
    for index in range(count):
        kind, = struct.unpack_from(endian + "I", data, offset + index * size)
        if kind == 1:
            alignment, = struct.unpack_from(endian + "Q", data, offset + index * size + 48)
            if alignment < 16384:
                raise SystemExit(f"Unaligned: {lib}, segment {index}: {alignment}")
            loads += 1
    if not loads:
        raise SystemExit(f"Missing LOAD segments: {lib}")
    checked += 1
    print(f"16KB ELF alignment PASS: {lib.name} ({lib.parent.name}), {loads} LOAD segments")
if checked < 2:
    raise SystemExit("Expected both arm64-v8a and x86_64 shared libraries")
