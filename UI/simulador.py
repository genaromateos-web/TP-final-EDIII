# simulador.py
import serial
import time

PORT = 'COM10'
ser = serial.Serial(PORT, 9600)

FREQ_MIN  = 1
FREQ_MAX  = 9999
FREQ_STEP = 50

AMP_MIN  = 0
AMP_MAX  = 1024
AMP_STEP = 8

DELAY = 0.03

wave      = 0
freq      = FREQ_MIN
freq_dir  = 1

amp       = AMP_MAX
amp_dir   = -1

while True:
    pkt = bytes([
        0xAA,
        (freq >> 8) & 0xFF, freq & 0xFF,
        (amp >> 8) & 0xFF, amp & 0xFF,
        wave,
        0xFF
    ])
    ser.write(pkt)
    print(f"Enviado: freq={freq} amp={amp} wave={wave}")

    freq += freq_dir * FREQ_STEP
    if freq >= FREQ_MAX:
        freq = FREQ_MAX
        freq_dir = -1
    elif freq <= FREQ_MIN:
        freq = FREQ_MIN
        freq_dir = 1
        wave = (wave + 1) % 3

    amp += amp_dir * AMP_STEP
    if amp >= AMP_MAX:
        amp = AMP_MAX
        amp_dir = -1
    elif amp <= AMP_MIN:
        amp = AMP_MIN
        amp_dir = 1

    time.sleep(DELAY)