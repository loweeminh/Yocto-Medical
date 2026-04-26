#!/usr/bin/env python3
import time
import os
import numpy as np

# Path to the device node created by the kernel driver
DEVICE_PATH = "/dev/max30102"

# Algorithm Constants
BUFFER_SIZE = 100       # Number of samples to store for calculation (approx. 10 seconds of data)
SAMPLING_RATE = 10      # 10Hz sampling frequency (matches sleep 0.1s)
MIN_BPM = 45            # Minimum realistic heart rate
MAX_BPM = 180           # Maximum realistic heart rate

class HeartRateFilter:
    def __init__(self):
        self.ir_buffer = []
        self.timestamps = []
        self.last_bpm = 0

    def low_pass_filter(self, data, alpha=0.1):
        """ Smooths out raw signal to remove high-frequency noise """
        filtered_data = [data[0]]
        for i in range(1, len(data)):
            filtered_data.append(alpha * data[i] + (1 - alpha) * filtered_data[i-1])
        return np.array(filtered_data)

    def calculate_bpm(self, ir_value):
        self.ir_buffer.append(ir_value)
        self.timestamps.append(time.time())

        # Maintain buffer size to prevent memory leakage
        if len(self.ir_buffer) > BUFFER_SIZE:
            self.ir_buffer.pop(0)
            self.timestamps.pop(0)

        # Wait until buffer is full for accurate calculation
        if len(self.ir_buffer) < BUFFER_SIZE:
            return None 

        # 1. Apply Low-pass filter
        filtered = self.low_pass_filter(self.ir_buffer)
        
        # 2. DC Removal: Subtract mean to get the AC signal (pulsatile component)
        ac_signal = filtered - np.mean(filtered)

        # 3. Simple Peak Detection Algorithm
        peaks = 0
        for i in range(1, len(ac_signal) - 1):
            # A peak is defined as a local maximum greater than 0
            if ac_signal[i] > ac_signal[i-1] and ac_signal[i] > ac_signal[i+1] and ac_signal[i] > 0:
                peaks += 1

        # 4. Calculate BPM based on peaks detected within the buffer duration
        duration = self.timestamps[-1] - self.timestamps[0]
        if duration <= 0:
            return int(self.last_bpm)
            
        bpm = (peaks / duration) * 60

        # Filter BPM for realistic values and apply moving average for visual stability
        if MIN_BPM <= bpm <= MAX_BPM:
            self.last_bpm = 0.7 * self.last_bpm + 0.3 * bpm 
            return int(self.last_bpm)
            
        return int(self.last_bpm)

def main():
    # Check if the character device exists
    if not os.path.exists(DEVICE_PATH):
        print(f"Error: {DEVICE_PATH} not found. Ensure the driver is loaded!")
        return

    hr_filter = HeartRateFilter()
    print("--- Yocto Medical: Advanced Heart Rate Monitoring ---")
    print("Initializing sensor... Please place your finger on the sensor.")

    try:
        # Open the device node for reading
        with open(DEVICE_PATH, "r") as f:
            while True:
                line = f.read().strip()
                if line:
                    try:
                        red, ir = map(int, line.split(','))
                        
                        # Detect if a finger is present (IR thresholding)
                        if ir < 30000:
                            print("Status: No finger detected                  ", end='\r')
                            hr_filter.ir_buffer = [] # Reset buffer if finger is removed
                        else:
                            bpm = hr_filter.calculate_bpm(ir)
                            status = f"BPM: {bpm if bpm else 'Calculating...'}"
                            # Using end='\r' to refresh the same line in the terminal
                            print(f"RED: {red:7d} | IR: {ir:7d} | {status}      ", end='\r')
                            
                    except ValueError:
                        # Handle potential data corruption or incomplete lines
                        continue
                
                # Maintain 10Hz sampling rate
                time.sleep(0.1) 
    except KeyboardInterrupt:
        print("\nMonitoring stopped by user. Exiting...")
    except Exception as e:
        print(f"\nAn unexpected error occurred: {e}")

if __name__ == "__main__":
    main()