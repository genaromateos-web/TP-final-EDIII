# monitor.py
import serial
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.animation as animation
import threading

# ── Serial / protocol settings ────────────────────────────────────────────────
PORT     = 'COM8'
BAUDRATE = 9600
PKT_SIZE = 7
VCC      = 3.3   # LPC1769 supply voltage (V)

WAVE_NAMES = ['Senoidal', 'Cuadrada', 'Triangular']

# ── Shared state (updated by the serial thread, read by the animation) ────────
state = {'frequency': 1, 'amplitude': 0, 'wave_type': 0}
state_lock = threading.Lock()
running = True


def parse_packet(data: bytes):
    """Validate and decode a 7-byte wave-parameter packet.

    Packet layout:
        [0]    0xAA  start delimiter
        [1:2]  frequency (big-endian, Hz)
        [3:4]  amplitude (big-endian, Q10 units 0-1024)
        [5]    wave type (0=sine, 1=square, 2=triangle)
        [6]    0xFF  end delimiter

    Returns a dict with 'frequency', 'amplitude', 'wave_type',
    or None if the packet is invalid.
    """
    if len(data) != PKT_SIZE:
        return None
    if data[0] != 0xAA or data[6] != 0xFF:
        return None
    return {
        'frequency': (data[1] << 8) | data[2],
        'amplitude': (data[3] << 8) | data[4],
        'wave_type':  data[5]
    }


def serial_reader(ser):
    """Background thread: continuously reads the serial port and updates state.

    Synchronises on the 0xAA start byte before reading the rest of the packet,
    which keeps the reader aligned even after a partial or corrupted frame.
    """
    global running
    while running:
        byte = ser.read(1)
        if byte and byte[0] == 0xAA:
            pkt = byte + ser.read(PKT_SIZE - 1)
            parsed = parse_packet(pkt)
            if parsed:
                with state_lock:
                    state.update(parsed)


def waveform(phase, wave_type):
    """Return normalised waveform samples (-1 to +1) for the given phase array.

    Args:
        phase:     NumPy array of phase values (radians).
        wave_type: 0 = sine, 1 = square, 2 = triangle.
    """
    if wave_type == 0:
        return np.sin(phase)
    elif wave_type == 1:
        return np.sign(np.sin(phase))
    else:
        return (2 / np.pi) * np.arcsin(np.sin(phase))


# ── Serial port + reader thread ───────────────────────────────────────────────
ser = serial.Serial(PORT, BAUDRATE, timeout=0.1)
threading.Thread(target=serial_reader, args=(ser,), daemon=True).start()

# ── Plot constants ────────────────────────────────────────────────────────────
N_POINTS = 500   # samples per frame
N_CYCLES = 2     # number of waveform cycles shown at once

# Scroll speed (rad/frame) mapped logarithmically over the frequency range
FREQ_MIN   = 1
FREQ_MAX   = 9999
SCROLL_MIN = 0.01   # slowest scroll (low frequencies)
SCROLL_MAX = 0.35   # fastest scroll (high frequencies)

LOG_FREQ_MIN = np.log10(FREQ_MIN)
LOG_FREQ_MAX = np.log10(FREQ_MAX)


def freq_to_scroll_step(freq):
    """Map frequency (Hz) to a scroll speed (rad/frame) on a log scale."""
    f = np.clip(freq, FREQ_MIN, FREQ_MAX)
    t = (np.log10(f) - LOG_FREQ_MIN) / (LOG_FREQ_MAX - LOG_FREQ_MIN)
    return SCROLL_MIN + t * (SCROLL_MAX - SCROLL_MIN)


# ── Figure layout ─────────────────────────────────────────────────────────────
fig, (ax_wave, ax_info) = plt.subplots(1, 2, figsize=(12, 4),
                                        gridspec_kw={'width_ratios': [3, 1]})
fig.patch.set_facecolor('#0d0d0d')

# Waveform axes
ax_wave.set_facecolor('#0d0d0d')
ax_wave.set_title('Señal generada', color='#00ff41')
ax_wave.tick_params(colors='#00ff41')
ax_wave.grid(color='#1a2a1a', linestyle='--', linewidth=0.5)
[sp.set_color('#1a2a1a') for sp in ax_wave.spines.values()]
ax_wave.set_xlabel('fase (ciclos)', color='#00ff41')
ax_wave.set_xlim(0, N_CYCLES)
ax_wave.set_ylim(-VCC/2 * 1.1, VCC/2 * 1.1)
ax_wave.set_ylabel('V', color='#00ff41')
line, = ax_wave.plot([], [], color='#00ff41', lw=1.5)

# Info panel (text only, no axes)
ax_info.set_facecolor('#0d0d0d')
ax_info.axis('off')
info_text = ax_info.text(0.5, 0.5, '', transform=ax_info.transAxes, color='#00ff41', fontsize=13, ha='center', va='center', fontfamily='monospace')

# Phase array (fixed; scroll_phase shifts it every frame)
x_cycles     = np.linspace(0, N_CYCLES, N_POINTS)
scroll_phase = 0.0


def update(_):
    """Animation callback — redraws the waveform and info panel each frame."""
    global scroll_phase

    with state_lock:
        freq = max(state['frequency'], 1)
        amp  = state['amplitude']
        wt   = state['wave_type']

    amp_volts_pp = (amp / 1024.0) * VCC   # peak-to-peak voltage (display only)
    amp_volts_p  = amp_volts_pp / 2       # peak voltage (used to scale the plot)

    phase = 2 * np.pi * x_cycles + scroll_phase
    y = waveform(phase, wt) * amp_volts_p
    line.set_data(x_cycles, y)

    # Advance the scroll phase and wrap to [0, 2π)
    step = freq_to_scroll_step(freq)
    scroll_phase = (scroll_phase + step) % (2 * np.pi)

    info_text.set_text(
        f"Frecuencia\n{freq} Hz\n\n"
        f"Amplitud\n{amp_volts_pp:.2f} Vpp\n\n"
        f"Forma\n{WAVE_NAMES[wt]}"
    )
    return line, info_text


# ── Run ───────────────────────────────────────────────────────────────────────
ani = animation.FuncAnimation(fig, update, interval=33, blit=True)
plt.tight_layout()
plt.show()

# Cleanup after the window is closed
running = False
ser.close()