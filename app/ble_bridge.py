import asyncio
import requests
import logging
from bleak import BleakScanner, BleakClient, BleakError

# --- CONFIGURATION ---
API_URL = "http://localhost:8000/update"
DEVICE_NAME = "Sommelier_AI"
CHARACTERISTIC_UUID = "19B10001-E8F2-537E-4F6C-D104768A1214"

logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

def send_to_api(label, confidence, status):
    """Sends the data to your FastAPI endpoint"""
    payload = {
        "label": label,
        "confidence": confidence,
        "status": status
    }
    try:
        requests.post(API_URL, json=payload)
    except Exception as e:
        print(f"API Error: Is the server running? {e}")


def notification_handler(sender, data):
    try:
        text = data.decode('utf-8')
        if "," in text:
            label, score_str = text.split(',')
            # Send to API!
            send_to_api(label, float(score_str), "Connected")
    except Exception as e:
        print(f"Parse Error: {e}")


async def run_ble():
    print("BLE Bridge started. Searching for Arduino...")
    send_to_api("Scanning...", 0.0, "Scanning")

    while True:
        device = None

        device = await BleakScanner.find_device_by_filter(
            lambda d, ad: "19b10000-e8f2-537e-4f6c-d104768a1214" in ad.service_uuids
        )

        if device is None:
            logger.warning("Not found yet... make sure Arduino is blinking/on.")
            await asyncio.sleep(2)

        if device:
            logger.info(f"Found {device.name}. Connecting...")

            disconnected_event = asyncio.Event()
            def on_disconnect(client):
                logger.warning("Device disconnected!")
                disconnected_event.set()

            send_to_api("Connecting...", 0.0, "Connecting")

            try:
                async with BleakClient(device, disconnected_callback=on_disconnect) as client:
                    logger.info("✅ Connected!")
                    send_to_api("Ready to pour", 0.0, "Connected")

                    await client.start_notify(CHARACTERISTIC_UUID, notification_handler)

                    await disconnected_event.wait()

            except BleakError as e:
                logger.error(f"Bluetooth Error: {e}")
            except Exception as e:
                logger.error(f"Unexpected Error: {e}")

            send_to_api("Disconnected", 0.0, "Disconnected")

        await asyncio.sleep(2)


if __name__ == "__main__":
    asyncio.run(run_ble())