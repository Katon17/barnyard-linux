#!/usr/bin/env python3
"""
pooledstrings.py — pengganti DevTools/PooledStrings/PooledStrings.exe (C#) untuk Linux.

Membaca APooledStrings.txt dan menghasilkan APooledStrings.cpp dengan format
yang mengikuti format sumber generator C# upstream (belum diuji berdampingan):

    PooledStrings.cs  ->  {
                              "TPString8 g_str_%s;\\n"
                              "\\t{{ &g_str_%s,{1}\\"%s\\" }},\\n"  (alignment 33 - len)
                          }

Kenapa perlu: APooledStrings.cpp ada di .gitignore dan di-generate oleh prebuild
step milik premake (exe .NET). Di Linux kita pakai skrip ini supaya tidak butuh
mono/dotnet, dan supaya build bisa jalan di CI.

Pemakaian:
    python3 tools/pooledstrings.py <input.txt> <output.cpp>
"""
import sys

HEADER = """//-----------------------------------------------------------------------------
// Contains information about predefined pooled strings.
// Note: Automatically generated source file.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "pch.h"
#include <Toshi/Toshi.h>
#include <Toshi/TPString8.h>

TOSHI_NAMESPACE_USING

// Definitions of global pooled strings
"""


def is_entry(line: str) -> bool:
    """Sama persis dengan aturan C#: bukan komentar, tidak kosong, tanpa spasi."""
    return not line.lstrip().startswith("#") and len(line) >= 1 and " " not in line


def main(argv) -> int:
    if len(argv) != 3:
        print(__doc__)
        return 2

    in_path, out_path = argv[1], argv[2]
    with open(in_path, "r", encoding="utf-8-sig") as f:
        # ReadAllLines: baris tanpa '\n', tapi CR ikut terbuang oleh text mode
        lines = f.read().splitlines()

    entries = [ln for ln in lines if is_entry(ln)]

    out = [HEADER]
    for name in entries:
        out.append(f"TPString8 g_str_{name};\n")

    out.append("\n")
    out.append("static TPString8Initialiser::StringMap oInitStrMap[] = \n")
    out.append("{\n")
    for name in entries:
        pad = " " * (33 - len(name))
        out.append(f"\t{{ &g_str_{name},{pad}\"{name}\" }},\n")
    out.append("};\n")
    out.append("\n")
    out.append(
        f"static TPString8Initialiser oInitStrNode(oInitStrMap, {len(entries)}, TTRUE);\n"
    )
    out.append("\n")

    with open(out_path, "w", encoding="utf-8", newline="\n") as f:
        f.write("".join(out))

    print(f'Input file: {in_path}')
    print(f'Output file: {out_path}')
    print(f'"{out_path}" file has been successfully created. ({len(entries)} strings)')
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
