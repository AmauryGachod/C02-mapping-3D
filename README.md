# CO2 Mapping in 3D

Student project for real-time 3D mapping of CO2 concentration in indoor spaces using UWB positioning and environmental sensing.

## Description

This project combines Ultra-Wideband (UWB) positioning technology with CO2 sensing to create an interactive 3D visualization of indoor air quality. A mobile tag equipped with a CO2 sensor measures concentration levels while transmitting its position via Bluetooth to a computer for real-time visualization.

## Hardware

### Mobile Tag
- **UWB Module**: DW1000 (positioning)
- **CO2 Sensor**: SenseAir S8 (NDIR measurement)
- **Communication**: Bluetooth to computer
- **Microcontroller**: Arduino/ESP32

### Fixed Anchors
- **UWB Modules**: DW1000 (position references)
- **Configuration**: Minimum 4 anchors for 3D positioning

## System Architecture

```
Mobile Tag (DW1000 + SenseAir S8)
    ↓ UWB ranging with anchors
    ↓ Distance measurements
    ↓ Bluetooth transmission
Computer
    ↓ Python processing
    ↓ 3D position calculation
Streamlit Dashboard (3D visualization)
```

## Project Structure

```
C02-mapping-3D/
├── C02map/
│   ├── anchor/              # Anchor firmware
│   │   └── anchor.ino       # Arduino code for anchors
│   ├── tag/                 # Tag firmware
│   │   ├── tag.ino          # Arduino code for mobile tag
│   │   ├── link.cpp         # UWB communication management
│   │   └── link.h           # Communication header
│   └── visualization/       # Python visualization
│       ├── dashboard.py     # Streamlit dashboard
│       └── detection.py     # Data processing module
└── README.md
```

## Installation

### Requirements
- Python 3.8+
- Arduino IDE
- DW1000 Arduino library

### Python Setup

```bash
git clone https://github.com/AmauryGachod/C02-mapping-3D.git
cd C02-mapping-3D
pip install -r requirements.txt
```

### Key Dependencies
- `streamlit` - Interactive dashboard
- `numpy` - Numerical computations
- `pandas` - Data handling
- `plotly` - 3D visualization
- `pyserial` - Bluetooth/Serial communication
- `scipy` - Positioning algorithms

## Configuration

1. **Program Anchors**: Upload `C02map/anchor/anchor.ino` to each anchor with unique ID
2. **Program Tag**: Upload `C02map/tag/tag.ino` to the mobile tag
3. **Position Anchors**: Place anchors at known locations in the space
4. **Update Coordinates**: Set anchor positions in Python code

## Usage

### Launch Dashboard

```bash
streamlit run C02map/visualization/dashboard.py
```

Access at `http://localhost:8501`

### Data Acquisition

1. Power on the 4 fixed anchors
2. Start the mobile tag
3. Connect tag to computer via Bluetooth
4. Launch dashboard to begin data collection
5. Move through the space with the mobile tag

### Visualization Features

- Real-time 3D position and CO2 display
- CO2 concentration heatmap
- Temporal evolution graphs
- Basic statistics (min, max, average)

## Technical Specifications

### SenseAir S8 Sensor
- **Range**: 0-10000 ppm
- **Accuracy**: ±40 ppm ±3% of reading
- **Technology**: NDIR (Non-Dispersive Infrared)
- **Interface**: UART
- **Sampling**: ~1 Hz

### DW1000 Module
- **Standard**: IEEE 802.15.4-2011 UWB
- **Frequency**: 3.5-6.5 GHz
- **Interface**: SPI
- **Range**: 50-200m (line of sight)

### Positioning
- **Method**: 3D multilateration from distance measurements
- **Accuracy**: ±10-30 cm (configuration dependent)
- **Update Rate**: 10-100 Hz (configurable)

## License

Student project developed for educational purposes.

## Resources

- [DW1000 Datasheet](https://www.decawave.com/)
- [SenseAir S8 Documentation](https://senseair.com/)
- [Streamlit Documentation](https://docs.streamlit.io/)
- [Plotly Python](https://plotly.com/python/)
