import socket
import json
import sys

SOCKET_PATH = "/tmp/server_ipc.sock"

def send_alert(alert_type, message, client_id=None):
    """Creates and send an alert through Unix socket to server."""
    
    # create alert object (python dictionary)
    alert_data = {
        "type": alert_type,
        "message": message,
    }
    if client_id:
        alert_data["clientId"] = client_id

    json_string = json.dumps(alert_data)

    print(f"Connecting to socket at {SOCKET_PATH}...")
    
    try:
        #sock unix, stream to ensure data is delivered
        sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        sock.connect(SOCKET_PATH)
        print("Connection successful.")

        print(f"Sending JSON: {json_string}")
        sock.sendall(json_string.encode('utf-8'))

    except socket.error as e:
        print(f"Socket error: {e}")
    finally:
        if 'sock' in locals():
            sock.close()
        print("Script finished.")

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: python3 scripts/sensor_simulator.py <alert_type> \"<message>\" \"client_id[OPTIONAL]\"")
        sys.exit(1)
    
    client_id = sys.argv[3] if len(sys.argv) > 3 else None
    send_alert(sys.argv[1], sys.argv[2])