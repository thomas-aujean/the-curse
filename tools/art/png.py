"""Minimal indexed-PNG writer (no dependency): rescomp reads 8-bit palette PNGs."""
import struct
import zlib


def _chunk(kind, data):
    body = kind + data
    return struct.pack(">I", len(data)) + body + struct.pack(">I", zlib.crc32(body) & 0xFFFFFFFF)


def write_indexed(path, pixels, palette):
    """pixels: list of rows of palette indices; palette: list of (r, g, b), at most 256."""
    height = len(pixels)
    width = len(pixels[0])
    raw = b"".join(b"\x00" + bytes(row) for row in pixels)
    plte = b"".join(bytes(c) for c in palette)
    # tRNS: index 0 is transparent, like on the Mega Drive.
    trns = b"\x00" + b"\xff" * (len(palette) - 1)
    data = (b"\x89PNG\r\n\x1a\n"
            + _chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 3, 0, 0, 0))
            + _chunk(b"PLTE", plte)
            + _chunk(b"tRNS", trns)
            + _chunk(b"IDAT", zlib.compress(raw, 9))
            + _chunk(b"IEND", b""))
    with open(path, "wb") as f:
        f.write(data)


def md_color(rgb):
    """Snap to the Mega Drive's 3 bits per channel (what the console will really show)."""
    return tuple(min(255, round(c / 36) * 36) for c in rgb)
