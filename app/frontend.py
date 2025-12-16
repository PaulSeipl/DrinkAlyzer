import streamlit as st
import requests
import time
import os

# --- CONFIG ---
API_URL = "http://localhost:8000/current-drink"

st.set_page_config(
    page_title="Smart Bar",
    page_icon="🍷",
    layout="wide"
)

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
        margin: 2rem 0;
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
        margin: 1.5rem 0 0.5rem 0;
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

    /* Image container */
    .image-container {
        display: flex;
        justify-content: center;
        align-items: center;
        margin: 0.5rem 0;
    }

    /* Remove white background from images */
    .image-container img {
        background: transparent !important;
    }

    /* Confidence display */
    .confidence-container {
        margin: 2rem 0;
    }

    .confidence-label {
        color: #a0a0b0;
        font-size: 0.9rem;
        text-transform: uppercase;
        letter-spacing: 2px;
        margin-bottom: 0.5rem;
    }

    .confidence-value {
        font-size: 2.5rem;
        font-weight: 700;
        color: #fff;
        margin-bottom: 1rem;
    }

    /* Pulse animation for scanning */
    @keyframes pulse {
        0%, 100% {
            opacity: 1;
            transform: scale(1);
        }
        50% {
            opacity: 0.7;
            transform: scale(1.05);
        }
    }

    .scanning {
        animation: pulse 2s ease-in-out infinite;
    }

    /* Progress bar custom styling */
    .stProgress > div > div > div > div {
        background: linear-gradient(90deg, #667eea 0%, #764ba2 100%);
        border-radius: 10px;
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
def get_data():
    try:
        response = requests.get(API_URL, timeout=2)
        if response.status_code == 200:
            return response.json()
    except:
        return None
    return {"label": "API Error", "confidence": 0.0, "status": "Offline"}


data = get_data()

# --- UI LOGIC ---
st.markdown('<h1 class="main-title">🍷 Smart Bar</h1>', unsafe_allow_html=True)

if data:
    status = data.get('status', 'Unknown')
    label = data.get('label', 'Unknown')
    confidence = data.get('confidence', 0.0)

    # Create centered column layout
    col1, col2, col3 = st.columns([1, 2, 1])

    with col2:
        # Status Badge
        status_class = "status-online" if status == "Connected" else "status-offline"
        status_icon = "🟢" if status == "Connected" else "🔴"
        st.markdown(f'<div class="status-badge {status_class}">{status_icon} {status}</div>', unsafe_allow_html=True)

        # Drink Card
        st.markdown('<div class="drink-card">', unsafe_allow_html=True)

        # Drink Name
        display_label = label if label != "Unknown" else "Scanning..."
        st.markdown(f'<div class="drink-name">{display_label}</div>', unsafe_allow_html=True)

        # Image Display
        image_map = {
            "WATER": os.path.join(public_folder, "water_1.png"),
            "BEER": os.path.join(public_folder, "beer_1.png"),
            "WINE": os.path.join(public_folder, "wine_1.png"),
            "APEROL": os.path.join(public_folder, "aperol_1.png"),
            "RUM": os.path.join(public_folder, "rum_1.png")
        }

        fallback_image = os.path.join(public_folder, "scanning.png")
        img_path = image_map.get(label, fallback_image)

        if os.path.exists(img_path):
            img_class = "scanning" if label == "Unknown" else ""
            try:
                st.markdown(f'<div class="image-container {img_class}">', unsafe_allow_html=True)
                col_img1, col_img2, col_img3 = st.columns([1, 2, 1])
                with col_img2:
                    st.image(img_path, width='stretch')
                st.markdown('</div>', unsafe_allow_html=True)
            except Exception as e:
                st.error(f"Error loading image: {e}")
        else:
            st.warning(f"⚠️ Image not found: {img_path}")

        # Confidence Display
        if confidence > 0:
            st.markdown('<div class="confidence-container">', unsafe_allow_html=True)
            st.markdown('<div class="confidence-label">Confidence</div>', unsafe_allow_html=True)
            st.markdown(f'<div class="confidence-value">{int(confidence * 100)}%</div>', unsafe_allow_html=True)
            st.progress(confidence)
            st.markdown('</div>', unsafe_allow_html=True)

        st.markdown('</div>', unsafe_allow_html=True)

else:
    col1, col2, col3 = st.columns([1, 2, 1])
    with col2:
        st.markdown("""
        <div class="error-container">
            <h2>⚠️ Connection Error</h2>
            <p>Cannot connect to FastAPI Backend</p>
            <p>Please ensure the server is running on <code>localhost:8000</code></p>
        </div>
        """, unsafe_allow_html=True)

# Auto-refresh
time.sleep(0.5)
st.rerun()