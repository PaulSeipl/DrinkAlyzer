import streamlit as st
import requests
import time
import os
import base64

# --- CONFIG ---
API_BASE = "http://localhost:8000"
URL_DRINK = f"{API_BASE}/current-drink"
URL_SIP = f"{API_BASE}/last-sip"
URL_MODE = f"{API_BASE}/mode"

st.set_page_config(
    page_title="NanoBartender",
    page_icon=":wine_glass:",
    layout="wide"
)


# --- IMAGE TO BASE64 ---
def get_img_as_base64(file_path):
    if not os.path.exists(file_path):
        return None
    with open(file_path, "rb") as f:
        data = f.read()
    return base64.b64encode(data).decode()


# --- CUSTOM CSS ---
st.markdown("""
<style>
    /* Main background */
    .stApp {
        background: linear-gradient(135deg, #1e1e2e 0%, #2d1b3d 100%);
    }

    /* Hide Streamlit branding */
    #MainMenu {visibility: hidden;}
    footer {visibility: hidden;}
    header {visibility: hidden;}

    /* Custom card styling */
    .drink-card {
        background: rgba(255, 255, 255, 0.05);
        backdrop-filter: blur(10px);
        border-radius: 24px;
        padding: 2rem;
        border: 1px solid rgba(255, 255, 255, 0.1);
        box-shadow: 0 8px 32px rgba(0, 0, 0, 0.3);
        text-align: center;
        margin: 1rem 0;
        display: flex;
        flex-direction: column;
        align-items: center;
    }

    /* [NEW] Sip Card Styling */
    .sip-container {
        margin-top: 1.5rem;
        padding: 1rem;
        background: rgba(255, 255, 255, 0.08);
        border-radius: 12px;
        width: 80%;
    }

    .sip-label {
        font-size: 0.8rem;
        text-transform: uppercase;
        letter-spacing: 2px;
        color: #a0a0b0;
        margin-bottom: 0.2rem;
    }

    .sip-value {
        font-size: 1.8rem;
        font-weight: 700;
        color: #4fd1c5; /* Teal color for contrast */
    }

    /* Title styling */
    .main-title {
        font-size: 3.5rem;
        font-weight: 700;
        text-align: center;
        background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
        -webkit-background-clip: text;
        -webkit-text-fill-color: transparent;
        margin-bottom: 1rem;
        text-shadow: 0 0 30px rgba(102, 126, 234, 0.3);
    }

    /* Drink name */
    .drink-name {
        font-size: 3rem;
        font-weight: 700;
        color: #fff;
        margin: 1rem 0;
        text-transform: uppercase;
        letter-spacing: 3px;
        text-shadow: 0 0 20px rgba(255, 255, 255, 0.3);
    }

    /* Status badge */
    .status-badge {
        display: inline-block;
        padding: 0.5rem 1.5rem;
        border-radius: 50px;
        font-weight: 600;
        font-size: 0.9rem;
        margin-bottom: 1rem;
    }

    .status-online {
        background: linear-gradient(135deg, #11998e 0%, #38ef7d 100%);
        color: white;
        box-shadow: 0 4px 15px rgba(56, 239, 125, 0.4);
    }

    .status-offline {
        background: linear-gradient(135deg, #ee0979 0%, #ff6a00 100%);
        color: white;
        box-shadow: 0 4px 15px rgba(238, 9, 121, 0.4);
    }

    /* Image styling */
    .drink-img {
        max-width: 100%;
        height: auto;
        max-height: 300px;
        border-radius: 12px;
        margin: 1rem 0;
        transition: transform 0.3s ease;
    }

    /* Pulse animation for scanning */
    @keyframes pulse {
        0%, 100% { opacity: 1; transform: scale(1); }
        50% { opacity: 0.6; transform: scale(1.1); }
    }
    .scanning {
        animation: pulse 2s ease-in-out infinite;
    }

    /* Custom HTML Progress Bar */
    .progress-wrapper {
        width: 100%;
        background-color: rgba(255,255,255,0.1);
        border-radius: 10px;
        margin-top: 10px;
        height: 10px;
        overflow: hidden;
    }
    .progress-fill {
        height: 100%;
        background: linear-gradient(90deg, #667eea 0%, #764ba2 100%);
        border-radius: 10px;
        transition: width 0.5s ease;
    }

    .confidence-label {
        color: #a0a0b0;
        font-size: 0.9rem;
        text-transform: uppercase;
        letter-spacing: 2px;
        margin-top: 1rem;
    }

    .confidence-value {
        font-size: 2rem;
        font-weight: 700;
        color: #fff;
    }

    /* Error message styling */
    .error-container {
        background: rgba(239, 68, 68, 0.1);
        border: 2px solid rgba(239, 68, 68, 0.3);
        border-radius: 16px;
        padding: 2rem;
        text-align: center;
        color: #ef4444;
    }
</style>
""", unsafe_allow_html=True)

# --- PATH SETUP ---
current_dir = os.path.dirname(os.path.abspath(__file__))
public_folder = os.path.join(current_dir, "public")


# --- FETCH DATA FROM API ---
def get_json(url):
    try:
        response = requests.get(url, timeout=0.5)
        if response.status_code == 200:
            return response.json()
    except:
        pass
    return None


data = get_json(URL_DRINK)
sip_data = get_json(URL_SIP)
mode_data = get_json(URL_MODE)

# --- UI LOGIC ---
st.markdown('<h1 class="main-title">🍷 NanoBartender </h1>', unsafe_allow_html=True)

if mode_data:
    current_mode = mode_data.get("mode", "UNKNOWN")

    # Visual Logic
    if current_mode == "SIP":
        mode_color = "#f6ad55"  # Orange
        mode_icon = "🥤"
    elif current_mode == "SCAN":
        mode_color = "#63b3ed"  # Blue
        mode_icon = "📡"
    else:
        mode_color = "#718096"  # Grey
        mode_icon = "❓"

    st.markdown(f"""
    <div style="
        position: fixed; 
        top: 20px; 
        right: 20px; 
        background-color: {mode_color}; 
        padding: 10px 20px; 
        border-radius: 30px; 
        font-weight: bold; 
        color: white; 
        box-shadow: 0 4px 6px rgba(0,0,0,0.1);
        z-index: 9999;">
        {mode_icon} {current_mode} MODE
    </div>
    """, unsafe_allow_html=True)

# Create centered column layout
col1, col2, col3 = st.columns([1, 2, 1])

with col2:
    if data:
        status = data.get('status', 'Connected')
        label = data.get('label', 'Unknown')
        confidence = data.get('confidence', 0.0)

        # 1. PREPARE IMAGE
        image_map = {
            "WATER": "water_1.png",
            "BEER": "beer_1.png",
            "WINE": "wine_1.png",
            "APEROL": "aperol_1.png",
            "RUM": "rum_1.png"
        }

        filename = image_map.get(label, "unknown_1.png")

        if status == "Disconnected":
            status_class = "status-offline"
            status_icon = "🔴"
            filename = "disconnected_1.png"

        if status == "Connecting":
            status_class = "status-offline"
            status_icon = "🟠"
            filename = "connecting_1.png"

        full_path = os.path.join(public_folder, filename)
        b64_img = get_img_as_base64(full_path)

        img_html = ""
        img_class = "scanning" if label == "Unknown" else ""
        if status == "Connecting": img_class = "scanning"

        if b64_img:
            img_html = f'<img src="data:image/png;base64,{b64_img}" class="drink-img {img_class}" />'
        else:
            img_html = f'<div style="color:white; padding: 2rem;">⚠️ Image not found: {filename}</div>'

        # 2. PROGRESS BAR
        conf_percent = int(confidence * 100)
        progress_html = ""
        if confidence > 0:
            progress_html = f"""
<div style="width: 100%; margin-top: 1rem;">
    <div class="confidence-label">Confidence</div>
    <div class="confidence-value">{conf_percent}%</div>
    <div class="progress-wrapper">
        <div class="progress-fill" style="width: {conf_percent}%;"></div>
    </div>
</div>"""

        # 3. [NEW] SIP DURATION SECTION
        sip_html = ""
        if sip_data and sip_data.get('duration', 0) > 0:
            duration_sec = sip_data['duration'] / 1000.0
            sip_html = f"""
<div class="sip-container">
    <div class="sip-label">Last Sip Duration</div>
    <div class="sip-value">{duration_sec:.1f}s</div>
</div>
"""

        # 4. RENDER CARD
        if status != "Disconnected":
            status_class = "status-online"
            status_icon = "🟢"
        display_label = label if label != "Unknown" else "Scanning..."

        html_block = f"""
<div class="drink-card">
    <div class="status-badge {status_class}">{status_icon} {status}</div>
    <div class="drink-name">{display_label}</div>
    {img_html}
    {progress_html}
    {sip_html} </div>
"""

        st.markdown(html_block, unsafe_allow_html=True)

    else:
        # Offline State
        st.markdown("""
        <div class="error-container">
            <div class="status-badge status-offline">🔴 Offline</div>
            <h2>⚠️ Connection Error</h2>
            <p>Cannot connect to Backend API</p>
            <p style="font-size: 0.8rem; opacity: 0.7">Ensure server is running on localhost:8000</p>
        </div>
        """, unsafe_allow_html=True)

# Auto-refresh
time.sleep(0.5)
st.rerun()