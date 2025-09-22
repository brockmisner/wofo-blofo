from mitmproxy import http
import paho.mqtt.client as mqtt
import json

cell_data = [
    {
        "cellId": 12345,
        "locationAreaCode": 6789,
        "mobileCountryCode": 310,
        "mobileNetworkCode": 260,
        "signalStrength": -75,
    }
]

def on_mqtt_message(client, userdata, msg):
    global cell_data
    if msg.topic == "esp32/all/config":
        payload = json.loads(msg.payload.decode())
        cell_data = payload.get("cell", cell_data)

def forge_response(flow: http.HTTPFlow):
    if "geolocation/v1/geolocate" in flow.request.pretty_url:
        response = {
            "location": {"lat": 40.6782, "lng": -73.9442},
            "accuracy": 20.0,
            "cellTowers": cell_data,
        }
        flow.response = http.HTTPResponse.make(
            200,
            json.dumps(response).encode(),
            {"Content-Type": "application/json"},
        )

mqtt_client = mqtt.Client()
mqtt_client.username_pw_set("your_username", "your_password")
mqtt_client.connect("localhost", 1883, 60)
mqtt_client.subscribe("esp32/all/config")
mqtt_client.on_message = on_mqtt_message
mqtt_client.loop_start()
