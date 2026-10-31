#!/usr/bin/env python3
import os, sys
print("argv:", sys.argv)
fd = int(sys.argv[-1])
st = os.fstat(fd)
print("fd:", fd, "mode:", oct(st.st_mode), "size:", st.st_size)
try:
    print("read:", os.read(fd, 64).hex())
except OSError as exc:
    print("read_error:", repr(exc))
