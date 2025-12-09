"""
UWB Tag Dashboard - WORKING VERSION
"""

import streamlit as st
import pandas as pd
import plotly.express as px
import plotly.graph_objects as go
from pathlib import Path
import numpy as np
from scipy.optimize import least_squares
import time
import shutil
import subprocess

# ============================================================
# CONFIGURATION
# ============================================================
st.set_page_config(
    page_title="UWB Tag Live Dashboard",
    page_icon="📡",
    layout="wide"
)

# CSV PATH - ABSOLUTE (SAME AS detection.py)
CSV_PATH = Path(__file__).parent / "logs" / "uwb_data.csv"

# POSITIONS DES ANCHORS
ANCHOR_POSITIONS = {
    '8817': (4, -4.5),
    '8717': (0.0, 0.0),
    '8617': (4.50, 6.5),
}

# ============================================================
# HELPER FUNCTIONS
# ============================================================
def check_ble_status():
    """Check if detection.py is running"""
    try:
        result = subprocess.run(
            ['pgrep', '-f', 'detection.py'],
            capture_output=True,
            text=True,
            timeout=2
        )
        return bool(result.stdout.strip())
    except:
        return None

def get_latest_distances(df):
    """Get latest distance for each anchor"""
    distances = {}
    for anchor_addr in df['anchor_addr'].unique():
        anchor_addr_str = str(anchor_addr)
        anchor_data = df[df['anchor_addr'] == anchor_addr]
        
        if len(anchor_data) > 0:
            latest_dist = anchor_data['range_m'].iloc[-1]
            distances[anchor_addr_str] = float(latest_dist)
    
    return distances

def trilaterate_2d(anchors_data):
    """Calculate 2D position"""
    if len(anchors_data) < 3:
        return None
    
    positions = []
    distances = []
    
    for anchor_addr, distance in anchors_data.items():
        if anchor_addr in ANCHOR_POSITIONS:
            positions.append(ANCHOR_POSITIONS[anchor_addr])
            distances.append(distance)
    
    if len(positions) < 3:
        return None
    
    P = np.array(positions[:3])
    r = np.array(distances[:3])
    
    def error_func(x):
        return np.linalg.norm(P - x, axis=1) - r
    
    x0 = np.mean(P, axis=0)
    result = least_squares(error_func, x0)
    
    return tuple(result.x) if result.success else None

def calculate_trajectory_sampled(df, sample_interval_sec=10):
    """Calculate trajectory"""
    trajectory = []
    df_sorted = df.sort_values('received_at')
    start_time = df_sorted['received_at'].min()
    end_time = df_sorted['received_at'].max()
    
    time_bins = pd.date_range(start=start_time, end=end_time, freq=f'{sample_interval_sec}s')
    
    for time_point in time_bins:
        window_start = time_point - pd.Timedelta(seconds=2.5)
        window_end = time_point + pd.Timedelta(seconds=2.5)
        
        df_window = df_sorted[
            (df_sorted['received_at'] >= window_start) & 
            (df_sorted['received_at'] <= window_end)
        ]
        
        if len(df_window) == 0:
            continue
        
        distances = {}
        co2_value = df_window['co2_ppm'].median()
        
        for anchor_addr in df_window['anchor_addr'].unique():
            anchor_addr_str = str(anchor_addr)
            anchor_data = df_window[df_window['anchor_addr'] == anchor_addr]
            median_dist = anchor_data['range_m'].median()
            distances[anchor_addr_str] = float(median_dist)
        
        position = trilaterate_2d(distances)
        
        if position:
            trajectory.append({
                'timestamp': time_point,
                'x': position[0],
                'y': position[1],
                'co2': co2_value
            })
    
    return pd.DataFrame(trajectory)

# ============================================================
# MAIN
# ============================================================
import time
def main():

#   # Clear CSV only once per session when dashboard starts
#     if 'csv_cleared' not in st.session_state:
#         if CSV_PATH.exists():
#             CSV_PATH.write_text("timestamp_ms,anchor_addr,range_m,rssi_dbm,co2_ppm,received_at\n")
#             st.session_state.csv_cleared = True
#         else:
#             st.session_state.csv_cleared = True



    st.title("📡 UWB Tag Real-Time Dashboard")
    
    # Show CSV path for debugging
    st.caption(f"📁 Reading from: {CSV_PATH.absolute()}")
    
    # ============================================
    # CONTROL PANEL
    # ============================================
    col_ble, col_clear = st.columns([3, 1])
    
    with col_ble:
        ble_status = check_ble_status()
        
        if ble_status is True:
            st.success("✅ BLE Collection: Running")
        elif ble_status is False:
            st.error("❌ BLE Collection: Not Running")
            st.info("Run: `python3 detection.py`")
        else:
            st.warning("⚠️ Cannot check BLE status")
    
    with col_clear:
        if st.button("🗑️ Clear", type="secondary", use_container_width=True):
            if CSV_PATH.exists():
                backup_path = CSV_PATH.parent / f"backup_{int(time.time())}.csv"
                shutil.copy(CSV_PATH, backup_path)
                CSV_PATH.write_text("timestamp_ms,anchor_addr,range_m,rssi_dbm,co2_ppm,received_at\n")
                st.success("✅ Cleared!")
                time.sleep(1)
                st.rerun()
    
    st.markdown("---")
    
    # ============================================
    # LOAD DATA
    # ============================================
    if not CSV_PATH.exists():
        st.error(f"❌ CSV not found at: {CSV_PATH}")
        return
    
    try:
        df = pd.read_csv(CSV_PATH)
        
        if len(df) == 0:
            st.warning("⏳ Waiting for data...")
            time.sleep(2)
            st.rerun()
            return
        
        df['received_at'] = pd.to_datetime(df['received_at'])
        
    except Exception as e:
        st.error(f"Error: {e}")
        return
    
    df_latest = df.tail(100)
    latest_distances = get_latest_distances(df_latest)
    current_position = trilaterate_2d(latest_distances)
    current_co2 = df_latest['co2_ppm'].iloc[-1]
    
    # ============================================
    # METRICS
    # ============================================
    col1, col2, col3, col4 = st.columns(4)
    
    with col1:
        st.metric("Active Anchors", df_latest['anchor_addr'].nunique())
    with col2:
        st.metric("Avg Range", f"{df_latest['range_m'].mean():.2f} m")
    with col3:
        st.metric("CO2 Level", f"{current_co2} ppm")
    with col4:
        st.metric("Total Points", len(df))
    
    st.markdown("---")
    
    # ============================================
    # CHARTS
    # ============================================
    c1, c2 = st.columns(2)
    
    with c1:
        st.subheader("📏 Distance to Anchors")
        fig = px.line(df_latest, x='received_at', y='range_m', color='anchor_addr')
        fig.update_layout(height=350)
        st.plotly_chart(fig, use_container_width=True)
    
    with c2:
        st.subheader("📶 Signal Strength")
        fig = px.line(df_latest, x='received_at', y='rssi_dbm', color='anchor_addr')
        fig.update_layout(height=350)
        st.plotly_chart(fig, use_container_width=True)
    
    c3, c4 = st.columns(2)
    
    with c3:
        st.subheader("💨 CO2")
        df_co2 = df_latest.drop_duplicates(subset=['received_at'], keep='last')
        fig = go.Figure()
        fig.add_trace(go.Scatter(x=df_co2['received_at'], y=df_co2['co2_ppm'],
                                  mode='lines+markers', line=dict(color='green', width=2)))
        fig.update_layout(height=350)
        st.plotly_chart(fig, use_container_width=True)
    
    with c4:
        st.subheader("🎯 Current Distances")
        latest = df_latest.groupby('anchor_addr').last().reset_index()
        fig = px.bar(latest, x='anchor_addr', y='range_m', color='rssi_dbm',
                    color_continuous_scale='Viridis')
        fig.update_layout(height=350)
        st.plotly_chart(fig, use_container_width=True)
    
    # ============================================
    # TABLE
    # ============================================
    st.subheader("📋 Recent Data")
    st.dataframe(df_latest.tail(20)[['received_at', 'anchor_addr', 'range_m', 'rssi_dbm', 'co2_ppm']],
                use_container_width=True)
    st.caption(f"Last: {df_latest['received_at'].iloc[-1]}")
    
    st.markdown("---")
    
    # ============================================
    # MAP
    # ============================================
    st.subheader("📍 Position & CO2 Map")
    
    trajectory_df = calculate_trajectory_sampled(df, 10)

    ANCHOR_NAMES = {
    '8617': 'Anchor 1',
    '8717': 'Anchor 2',
    '8817': 'Anchor 3',
}

# ...

    if not trajectory_df.empty:
        fig = go.Figure()

        # Anchors
        for aid, (x, y) in ANCHOR_POSITIONS.items():
            fig.add_trace(go.Scatter(
                x=[x],
                y=[y],
                mode='markers+text',
                marker=dict(size=15, color='lightgray', symbol='square'),
                text=[ANCHOR_NAMES.get(aid, f"A{aid[-2:]}")],
                textposition='top center',
                showlegend=False,
                hoverinfo='skip'
            ))

    # # ... reste inchangé ...

    # if not trajectory_df.empty:
    #     fig = go.Figure()
        
    #     # Anchors
    #     for aid, (x, y) in ANCHOR_POSITIONS.items():
    #         fig.add_trace(go.Scatter(x=[x], y=[y], mode='markers+text',
    #             marker=dict(size=15, color='lightgray', symbol='square'),
    #             text=[f'A{aid[-2:]}'], textposition='top center',
    #             showlegend=False, hoverinfo='skip'))
        
        # Trajectory
        fig.add_trace(go.Scatter(x=trajectory_df['x'], y=trajectory_df['y'],
            mode='markers+lines',
            marker=dict(size=12, color=trajectory_df['co2'], colorscale='RdYlGn_r',
                       showscale=True, colorbar=dict(title='CO2<br>(ppm)'),
                       cmin=400, cmax=1000, line=dict(width=1, color='black')),
            line=dict(color='lightblue', width=1, dash='dot'),
            name='Trajectory'))
        
        # Current
        if current_position:
            fig.add_trace(go.Scatter(x=[current_position[0]], y=[current_position[1]],
                mode='markers', marker=dict(size=20, color='red', symbol='star',
                line=dict(width=2, color='black')), name='Current'))
        
        fig.update_layout(height=500,
            xaxis=dict(title='X (m)', scaleanchor='y', scaleratio=1),
            yaxis=dict(title='Y (m)', scaleanchor='x', scaleratio=1),
            showlegend=True)
        
        st.plotly_chart(fig, use_container_width=True)
        st.info(f"📊 {len(trajectory_df)} positions")
    
    # Auto-refresh
    time.sleep(3)
    st.rerun()

if __name__ == "__main__":
    main()
