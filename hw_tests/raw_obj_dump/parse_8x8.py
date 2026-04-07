#!/usr/bin/env python3
"""
Parse raw #Obj CSV lines from TMF8828 and display 8x8 distance grid.
Uses the known-good zone mapping from shabaz123/TMF8828 viewer.

Usage:
  python3 parse_8x8.py /dev/ttyUSB0
  # or pipe from file:
  python3 parse_8x8.py < capture.txt
"""

import sys, os

# Zone mapping: for each subcapture (0-3), the 16 grid indices (into 8x8 flat array)
# where the 16 object-0 zones land. From shabaz123/TMF8828 viewer (confirmed against ams EVM).
ZONE_MAP = {
    0: [8,9,12,13,24,25,28,29,40,41,44,45,56,57,60,61],
    1: [10,11,14,15,26,27,30,31,42,43,46,47,58,59,62,63],
    2: [0,1,4,5,16,17,20,21,32,33,36,37,48,49,52,53],
    3: [2,3,6,7,18,19,22,23,34,35,38,39,50,51,54,55],
}

def parse_obj_line(line):
    """Parse an #Obj CSV line. Returns (subcapture, distances[36], confidences[36])."""
    parts = line.strip().split(',')
    if len(parts) < 6 or parts[0] != '#Obj':
        return None
    result_number = int(parts[2])
    subcapture = result_number & 0x03
    # After the 6 header fields, pairs of (distance, confidence) for 36 zones
    data = parts[6:]
    distances = []
    confidences = []
    for i in range(0, min(len(data), 72), 2):
        distances.append(int(data[i]))
        confidences.append(int(data[i+1]))
    return subcapture, distances, confidences

def strip_ref_channels(values):
    """Strip reference channels (every 9th: indices 0,9,18,27) from 36 values → 32."""
    out = []
    for i, v in enumerate(values):
        if i % 9 != 0:
            out.append(v)
    return out

def assemble_8x8(subcaptures):
    """Assemble 8x8 grid from 4 subcaptures. Returns (dist[64], conf[64])."""
    dist = [0] * 64
    conf = [0] * 64
    for sub_id, (dists, confs) in subcaptures.items():
        # Strip reference channels: 36 → 32, take first 16 (object 0)
        d = strip_ref_channels(dists)[:16]
        c = strip_ref_channels(confs)[:16]
        for j in range(16):
            idx = ZONE_MAP[sub_id][j]
            dist[idx] = d[j]
            conf[idx] = c[j]
    return dist, conf

def print_grid(label, values, width=5):
    """Print an 8x8 grid."""
    print(f"\n{label}:")
    for row in range(8):
        cells = []
        for col in range(8):
            cells.append(f"{values[row*8+col]:>{width}}")
        print("  " + " ".join(cells))

def process_stream(stream):
    subcaptures = {}
    frame_count = 0
    for line in stream:
        line = line.strip()
        result = parse_obj_line(line)
        if result is None:
            continue
        sub, dists, confs = result
        subcaptures[sub] = (dists, confs)
        if len(subcaptures) == 4:
            dist, conf = assemble_8x8(subcaptures)
            frame_count += 1
            print(f"\n{'='*60}")
            print(f"Frame {frame_count}")
            print_grid("Distance (mm)", dist)
            print_grid("Confidence", conf, width=3)
            subcaptures = {}

if __name__ == '__main__':
    if len(sys.argv) > 1 and os.path.exists(sys.argv[1]):
        # Serial port
        import serial
        ser = serial.Serial(sys.argv[1], 115200, timeout=1)
        # Reset ESP32
        ser.dtr = False
        import time; time.sleep(0.1)
        ser.dtr = True; time.sleep(0.5)
        
        class SerialStream:
            def __init__(self, ser):
                self.ser = ser
            def __iter__(self):
                return self
            def __next__(self):
                line = self.ser.readline()
                if line:
                    return line.decode('utf-8', errors='replace')
                return ''
        
        try:
            process_stream(SerialStream(ser))
        except KeyboardInterrupt:
            pass
        finally:
            ser.close()
    else:
        # stdin
        process_stream(sys.stdin)
