from fastapi import FastAPI
from pydantic import BaseModel

app = FastAPI()

# Data Model
class DrinkData(BaseModel):
    label: str
    confidence: float
    status: str

# In-Memory Database (Just a global variable for now)
# Default state
current_state = {
    "label": "Ready",
    "confidence": 0.0,
    "status": "Waiting for BLE..."
}

@app.get("/")
def home():
    return {"message": "Sommelier API is running"}

# 1. Endpoint for BLE Script to PUSH data
@app.post("/update")
def update_drink(data: DrinkData):
    global current_state
    current_state = data.dict()
    print(f"API Received: {current_state}") # Log to terminal
    return {"status": "success"}

# 2. Endpoint for Streamlit to GET data
@app.get("/current-drink")
def get_current_drink():
    return current_state