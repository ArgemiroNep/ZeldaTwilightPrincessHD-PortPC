#!/usr/bin/env python3
"""Release notes: what is new (the version's section of CHANGELOG.md), how to install, checksums.

usage: notes.py CHANGELOG.md VERSION SHA256SUMS.txt > notes.md
"""
import re
import sys


def changes(text, version):
    """The body of the CHANGELOG section whose heading starts with "## <version>"."""
    m = re.search(r"^## %s\b.*?$\n(.*?)(?=^## |\Z)" % re.escape(version), text, re.S | re.M)
    return m.group(1).strip() if m else ""


def main():
    changelog, version, sums = sys.argv[1:4]
    with open(changelog, encoding="utf-8") as f:
        text = f.read()
    with open(sums) as f:
        checksums = f.read().strip()
    print("""**Twilight Princess HD for Windows, Linux & Android (native port), %s**

This release contains **no game files, no game code and no keys**. You need your own dump of the game
(Wii U, USA): an extracted game folder (`code`, `content`, `meta`), a disc image (.wux/.wud with its keys)
or a Cemu .wua archive. The first start builds the game from it, on your own computer.

**Install**
- **Windows** (x86-64, Windows 10 1903+ or 11, Vulkan 1.3 GPU): `TwilightPrincessHD-%s-windows-x86_64.zip`.
  Unzip it anywhere and start `Twilight Princess HD.exe`. The release is not code-signed: if SmartScreen
  appears, choose *More info > Run anyway*.
- **Linux** (glibc 2.35+, Vulkan 1.3 GPU): `TwilightPrincessHD-%s-linux-x86_64.zip` (or `linux-aarch64` when
  available; match `uname -m`). Unzip it and start `twilight-princess-hd` (or `Twilight Princess HD.desktop`).
  It uses your Python 3.
- **Android** (arm64, Android 13+, Vulkan 1.3): built by you from your own game on a computer, never
  downloaded. See "Android" in the README.

The first start prepares the game once (several minutes); later starts launch it directly.

## What's new

%s

## Checksums (SHA-256)

```
%s
```
""" % (version, version, version, changes(text, version) or "See CHANGELOG.md.", checksums))


if __name__ == "__main__":
    main()
