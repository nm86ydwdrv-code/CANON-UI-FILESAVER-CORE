#!/usr/bin/env python3
"""
Upload files to a Kawazu Kumite M5Stack Core over USB serial.

Protocol (must match firmware/src/main.cpp):
    MAGIC "KKU1" (4 bytes)
    nameLen (1 byte)
    name (nameLen bytes, UTF-8)
    fileSize (4 bytes, little-endian)
    file data (fileSize bytes)
  -> device replies "OK\n" or "ERR\n"

Usage:
    python upload.py <serial-port> <file1> [file2 ...]

Example:
    python upload.py COM5 notes.txt photo.jpg
"""

import os
import sys
import serial

MAGIC = b"KKU1"
BAUD = 115200


def send_file(ser: serial.Serial, path: str) -> bool:
    name = os.path.basename(path)
    name_bytes = name.encode("utf-8")
    if len(name_bytes) > 255:
        print(f"  Skipping {name}: filename too long")
        return False

    size = os.path.getsize(path)

    header = MAGIC + bytes([len(name_bytes)]) + name_bytes + size.to_bytes(4, "little")
    ser.write(header)

    sent = 0
    with open(path, "rb") as f:
        while True:
            chunk = f.read(512)
            if not chunk:
                break
            ser.write(chunk)
            sent += len(chunk)
            print(f"\r  {name}: {sent}/{size} bytes", end="", flush=True)

    print()
    response = ser.readline().decode(errors="replace").strip()
    return response == "OK"


def main():
    if len(sys.argv) < 3:
        print(__doc__)
        sys.exit(1)

    port = sys.argv[1]
    files = sys.argv[2:]

    with serial.Serial(port, BAUD, timeout=10) as ser:
        for path in files:
            if not os.path.isfile(path):
                print(f"Skipping {path}: not a file")
                continue

            print(f"Uploading {path} ...")
            ok = send_file(ser, path)
            print("  -> OK" if ok else "  -> FAILED")


if __name__ == "__main__":
    main()
