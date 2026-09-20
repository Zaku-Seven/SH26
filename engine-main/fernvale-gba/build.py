#!/usr/bin/env python3
"""Build the native C game. Requires an ARM bare-metal GCC toolchain.

Uses devkitARM automatically if installed in the usual location. Python is
only the build helper; the cartridge runs the compiled C and ARM startup code.
"""
import os
from pathlib import Path
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parent
BUILD = ROOT / "build"
ROM = ROOT / "fernvale.gba"

def find_tool(name):
    prefix = os.environ.get("GBA_TOOLCHAIN")
    roots = [Path(prefix)] if prefix else []
    if os.environ.get("DEVKITARM"):
        roots.append(Path(os.environ["DEVKITARM"]) / "bin")
    roots.extend([Path("/opt/devkitpro/devkitARM/bin"),
                  Path("C:/devkitPro/devkitARM/bin")])
    filename = "arm-none-eabi-" + name + (".exe" if os.name == "nt" else "")
    for directory in roots:
        if (directory / filename).is_file():
            return str(directory / filename)
    located = shutil.which("arm-none-eabi-" + name)
    if located:
        return located
    sys.exit("ARM compiler not found. Install devkitARM using the official guide:\n"
             "https://devkitpro.org/wiki/Getting_Started\n"
             "Then reopen VS Code. See README.md for the project setup.")

def run(args):
    print("Building:", Path(str(args[0])).name, flush=True)
    subprocess.run([str(a) for a in args], cwd=ROOT, check=True)

def finalize_rom(path):
    data = bytearray(path.read_bytes())
    # Required GBA boot-header bytes, used by the console's cartridge check.
    logo = bytes.fromhex("""
        24 ff ae 51 69 9a a2 21 3d 84 82 0a 84 e4 09 ad
        11 24 8b 98 c0 81 7f 21 a3 52 be 19 93 09 ce 20
        10 46 4a 4a f8 27 31 ec 58 c7 e8 33 82 e3 ce bf
        85 f4 df 94 ce 4b 09 c1 94 56 8a c0 13 72 a7 fc
        9f 84 4d 73 a3 ca 9a 61 58 97 a3 27 fc 03 98 76
        23 1d c7 61 03 04 ae 56 bf 38 84 00 40 a7 0e fd
        ff 52 fe 03 6f 95 30 f1 97 fb c0 85 60 d6 80 25
        a9 63 be 03 01 4e 38 e2 f9 a2 34 ff bb 3e 03 44
        78 00 90 cb 88 11 3a 94 65 c0 7c 63 87 f0 3c af
        d6 25 e4 8b 38 0a ac 72 21 d4 f8 07
    """)
    assert len(logo) == 156
    data[4:160] = logo
    data[160:172] = b"FERNVALE    "
    data[172:176] = b"FVLE"
    data[176:178] = b"00"
    data[178:192] = bytes([0x96] + [0]*13)
    data[189] = (-sum(data[160:189]) - 0x19) & 255
    # A power-of-two image is friendly to both emulators and flash cartridges.
    size = max(65536, 1 << (len(data)-1).bit_length())
    data.extend(b"\xff" * (size-len(data)))
    path.write_bytes(data)

def main():
    BUILD.mkdir(exist_ok=True)
    gcc, objcopy = find_tool("gcc"), find_tool("objcopy")
    flags = ["-mcpu=arm7tdmi", "-mthumb", "-mthumb-interwork", "-O2", "-g",
             "-ffreestanding", "-fno-builtin", "-fno-common", "-nostdlib",
             "-fno-unwind-tables", "-fno-asynchronous-unwind-tables",
             "-Wall", "-Wextra", "-Werror", "-std=c11", "-Isource"]
    objects = []
    for file in ("startup.s", "city_main.c", "dialogue.c", "story.c", "assets.c", "city.s"):
        source = ROOT / "source" / file
        target = BUILD / (source.stem + ".o")
        run([gcc, *flags, "-c", source, "-o", target])
        objects.append(target)
    elf = BUILD / "fernvale.elf"
    run([gcc, "-mcpu=arm7tdmi", "-mthumb", "-mthumb-interwork", "-nostdlib",
         "-Wl,-T,gba.ld", "-Wl,-Map,build/fernvale.map", *objects, "-o", elf])
    run([objcopy, "-O", "binary", elf, ROM])
    finalize_rom(ROM)
    print(f"Ready: {ROM.name} ({ROM.stat().st_size // 1024} KiB)")
    print("Open fernvale.gba in your GBA emulator.")

if __name__ == "__main__":
    try:
        main()
    except subprocess.CalledProcessError as error:
        sys.exit(error.returncode)
