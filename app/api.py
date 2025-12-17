from fastapi import FastAPI
from pydantic import BaseModel

app = FastAPI()

# --- DATA MODELS ---
class DrinkData(BaseModel):
    label: str
    confidence: float
    status: str

class SipData(BaseModel):
    duration: int  # Duration in milliseconds

# --- IN-MEMORY STORAGE ---
current_drink_state = {
    "label": "Ready",
    "confidence": 0.0,
    "status": "Waiting for BLE..."
}

current_sip_state = {
    "duration": 0
}

current_mode = {"mode": "SCAN"}

@app.get("/")
def home():
    return {"message": "Sommelier API is running"}

# --- DRINK ENDPOINTS ---
@app.post("/update")
def update_drink(data: DrinkData):
    global current_drink_state
    current_drink_state = data.dict()
    print(f"API (Drink): {current_drink_state}")
    return {"status": "success"}

@app.get("/current-drink")
def get_current_drink():
    return current_drink_state

# --- NEW: SIP ENDPOINTS ---
@app.post("/update-sip")
def update_sip(data: SipData):
    global current_sip_state
    current_sip_state = data.dict()
    print(f"API (Sip): {current_sip_state}")
    return {"status": "success"}

@app.get("/last-sip")
def get_last_sip():
    return current_sip_state

@app.post("/update-mode")
def update_mode(data: dict):
    global current_mode
    current_mode = data
    print(f"API (Mode): {current_mode}")
    return {"status": "success"}

@app.get("/mode")
def get_mode():
    return current_mode