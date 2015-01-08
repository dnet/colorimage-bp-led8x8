#!/usr/bin/env python

from __future__ import division
from serial import Serial
from itertools import product

COLS = 8
ROWS = 8
PIXELS = set(product(xrange(COLS), xrange(ROWS)))

class Remote(object):
    def __init__(self, serial_class=Serial, port='/dev/ttyUSB0'):
        self.port = serial_class(port, 9600, timeout=0.1)
        self.framebuf = [None] * ROWS * COLS
        self.clip = set()
        for pixel in PIXELS:
            self.set_pixel(pixel, False)
        self.flush_pixels()

    def get_pixel(self, pixel):
        return self.framebuf[pixel2fbindex(pixel)]

    def set_pixel(self, pixel, value):
        fbi = pixel2fbindex(pixel)
        if self.framebuf[fbi] == value:
            return
        self.framebuf[fbi] = value
        self.clip.add((pixel[1], pixel[0] // 4))

    def flush_pixels(self):
        self.port.write(''.join(chr((row << 5) | (side << 4) |
            self.encode_side(row, side)) for row, side in sorted(self.clip)))
        self.clip.clear()

    def encode_side(self, row, side):
        base = pixel2fbindex((side * 4, row))
        return sum(bit << (3 - shift) for shift, bit in
                enumerate(self.framebuf[base:base + 4]))

def pixel2fbindex(pixel):
    col, row = pixel
    return row * COLS + col

if __name__ == '__main__':
    from time import sleep
    r = Remote()
    for y in xrange(ROWS):
        for x in xrange(COLS):
            r.set_pixel((x, y), True)
            r.flush_pixels()
            sleep(0.05)
            r.set_pixel((x, y), False)
