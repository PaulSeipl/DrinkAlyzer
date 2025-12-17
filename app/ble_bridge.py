import asyncio
import requests
import logging
from bleak import BleakScanner, BleakClient, BleakError

# --- CONFIGURATION ---
BASE_URL = "http://localhost:8000"
API_URL_DRINK = f"{BASE_URL}/update"
API_URL_SIP = f"{BASE_URL}/update-sip"
API_URL_MODE = f"{BASE_URL}/update-mode"

CHARACTERISTIC_UUID = "19B10001-E8F2-537E-4F6C-D104768A1214"

logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)


def send_drink_to_api(label, confidence, status):
    payload = {"label": label, "confidence": confidence, "status": status}
    try:
        requests.post(API_URL_DRINK, json=payload)
    except Exception as e:
        print(f"API Error (Drink): {e}")

def send_mode_to_api(mode_str):
    try:
        requests.post(API_URL_MODE, json={"mode": mode_str})
        print(f"🔄 Mode Switch: {mode_str}")
    except Exception as e:
        print(f"API Error (Mode): {e}")


def send_sip_to_api(duration_ms):
    payload = {"duration": duration_ms}
    try:
        requests.post(API_URL_SIP, json=payload)
        print(f"🥤 Sip Detected: {duration_ms}ms")
    except Exception as e:
        print(f"API Error (Sip): {e}")


def notification_handler(sender, data):
    try:
        text = data.decode('utf-8')

        # --- Check for Mode Change Data ---
        if text.startswith("MODE:"):
            # text is "MODE:SIP" or "MODE:SCAN"
            mode = text.split(":")[1]
            send_mode_to_api(mode)

        # --- Check for Sip Data ---
        if text.startswith("SIP:"):
            # Format is "SIP:2500"
            parts = text.split(":")
            if len(parts) == 2:
                duration = int(parts[1])
                send_sip_to_api(duration)

        # --- Check for Drink Data ---
        elif "," in text:
            # Format is "LABEL,SCORE"
            label, score_str = text.split(',')
            send_drink_to_api(label, float(score_str), "Connected")

    except Exception as e:
        print(f"Parse Error: {e} | Raw Data: {data}")


async def run_ble():
    print("BLE Bridge started. Searching for Sommelier_AI...")
    send_drink_to_api("Scanning...", 0.0, "Scanning")

    while True:
        device = await BleakScanner.find_device_by_filter(
            lambda d, ad: "19b10000-e8f2-537e-4f6c-d104768a1214" in ad.service_uuids
        )

        if device:
            logger.info(f"Found {device.name}. Connecting...")
            disconnected_event = asyncio.Event()

            def on_disconnect(client):
                logger.warning("Device disconnected!")
                disconnected_event.set()

            send_drink_to_api("Connecting...", 0.0, "Connecting")

            try:
                async with BleakClient(device, disconnected_callback=on_disconnect) as client:
                    logger.info("✅ Connected!")
                    send_drink_to_api("Ready to pour", 0.0, "Connected")

                    await client.start_notify(CHARACTERISTIC_UUID, notification_handler)
                    await disconnected_event.wait()

            except Exception as e:
                logger.error(f"Connection Error: {e}")

            send_drink_to_api("Disconnected", 0.0, "Disconnected")

        else:
            await asyncio.sleep(2)


if __name__ == "__main__":
    asyncio.run(run_ble())