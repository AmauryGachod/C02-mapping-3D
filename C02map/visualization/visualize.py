"""
UWB Tag BLE Reader - WITH OUTLIER FILTERING
Filters out sudden distance jumps and keeps previous valid value
"""

import asyncio
import json
from datetime import datetime
from pathlib import Path
from bleak import BleakClient, BleakScanner

# ============================================================
# CONFIG
# ============================================================
TARGET_NAME = "UWB_Tag"
SERVICE_UUID = "12341234-1234-1234-1234-123412341234"
CHARACTERISTIC_UUID = "12341234-1234-1234-1234-123412341236"

CSV_PATH = Path(__file__).parent / "logs" / "uwb_data.csv"

# FILTERING PARAMETERS
MAX_DISTANCE_JUMP = 2.0  # Maximum allowed change in meters
MIN_DISTANCE = 0.1       # Minimum valid distance
MAX_DISTANCE = 15.0      # Maximum valid distance

# ============================================================
# GLOBAL STATE - Last valid distance for each anchor
# ============================================================
last_valid_distances = {}

# ============================================================
# INIT
# ============================================================
CSV_PATH.parent.mkdir(exist_ok=True)
if not CSV_PATH.exists():
    CSV_PATH.write_text("timestamp_ms,anchor_addr,range_m,rssi_dbm,co2_ppm,received_at\n")
    
print(f"✓ CSV Path: {CSV_PATH.absolute()}")
print(f"✓ Filtering enabled: max jump = {MAX_DISTANCE_JUMP}m\n")

# ============================================================
# FILTERING LOGIC
# ============================================================
def filter_distance(anchor_addr, raw_distance):
    """
    Filter distance measurement for outliers
    Returns filtered distance (either raw value or previous valid value)
    """
    global last_valid_distances
    
    # Check basic bounds
    if raw_distance < MIN_DISTANCE or raw_distance > MAX_DISTANCE:
        if anchor_addr in last_valid_distances:
            print(f"  ⚠️  Out of bounds ({raw_distance:.2f}m) → Using previous: {last_valid_distances[anchor_addr]:.2f}m")
            return last_valid_distances[anchor_addr]
        else:
            # No previous value, accept it anyway
            print(f"  ⚠️  Out of bounds ({raw_distance:.2f}m) → No previous value, accepting")
            last_valid_distances[anchor_addr] = raw_distance
            return raw_distance
    
    # Check for large jump
    if anchor_addr in last_valid_distances:
        previous = last_valid_distances[anchor_addr]
        jump = abs(raw_distance - previous)
        
        if jump > MAX_DISTANCE_JUMP:
            print(f"  ⚠️  Large jump ({jump:.2f}m) → Using previous: {previous:.2f}m")
            return previous  # Keep previous value
        else:
            # Valid measurement, update
            last_valid_distances[anchor_addr] = raw_distance
            return raw_distance
    else:
        # First measurement for this anchor
        last_valid_distances[anchor_addr] = raw_distance
        return raw_distance

# ============================================================
# SAVE DATA
# ============================================================
def save_data(data_dict):
    """Save one data point to CSV with filtering"""
    anchor_addr = str(data_dict['A'])
    raw_distance = float(data_dict['R'])
    
    # Apply filtering
    filtered_distance = filter_distance(anchor_addr, raw_distance)
    
    # Prepare line
    line = f"{data_dict['T']},{anchor_addr},{filtered_distance},{data_dict['Rx']},{data_dict['C']},{datetime.now().isoformat()}\n"
    
    # Write to CSV
    with open(CSV_PATH, 'a') as f:
        f.write(line)
    
    # Print with filter indicator
    if abs(filtered_distance - raw_distance) < 0.01:
        # No filtering applied
        print(f"📡 Anchor: 0x{anchor_addr} | Range: {filtered_distance:.2f}m | RSSI: {data_dict['Rx']:.1f}dBm | CO2: {data_dict['C']}ppm ✓")
    else:
        # Filtering applied
        print(f"📡 Anchor: 0x{anchor_addr} | Range: {filtered_distance:.2f}m (filtered from {raw_distance:.2f}m) | RSSI: {data_dict['Rx']:.1f}dBm | CO2: {data_dict['C']}ppm 🔧")

# ============================================================
# BLE HANDLER
# ============================================================
def notification_handler(sender, data):
    """Handle BLE notifications"""
    try:
        json_str = data.decode('utf-8', errors='ignore')
        if not json_str or json_str == "Ready":
            return
        
        parsed = json.loads(json_str)
        entries = parsed if isinstance(parsed, list) else [parsed]
        
        for entry in entries:
            save_data(entry)
            
    except Exception as e:
        print(f"Error: {e}")

# ============================================================
# MAIN
# ============================================================
async def main():
    # Find device
    print("Scanning...")
    devices = await BleakScanner.discover(timeout=10)
    
    address = None
    for device in devices:
        if device.name and TARGET_NAME in device.name:
            address = device.address
            print(f"✓ Found {device.name}")
            break
    
    if not address:
        print("✗ Device not found")
        return
    
    # Connect
    print("Connecting...")
    async with BleakClient(address, timeout=10) as client:
        print("✓ Connected")
        
        await client.start_notify(CHARACTERISTIC_UUID, notification_handler)
        print("✓ Streaming data...\n")
        print("="*70)
        
        try:
            while True:
                await asyncio.sleep(1)
        except KeyboardInterrupt:
            print("\n" + "="*70)
            print("✓ Stopped")
            print(f"\nLast valid distances: {last_valid_distances}")

if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("✓ Exit")
