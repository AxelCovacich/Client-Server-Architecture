#!/usr/bin/env python3
import sys
import json
import os

#obtain path to the current script
base_dir = os.path.dirname(os.path.abspath(__file__))

# construct the path to the external libraries
external_libs = os.path.join(base_dir, "../external/python_libs")

# Add that path to sys.path (at the beginning so it takes priority)
if external_libs not in sys.path:
    sys.path.insert(0, external_libs)

import posix_ipc
import time
from rich.console import Console
from rich.panel import Panel
from rich.text import Text

# =====================
# CONFIG
# =====================
QUEUE_NAME = "/client_ipc_queue"
console = Console()

def print_header(client_id):
    header_text = f"[bold cyan]CLIENT {client_id} NOTIFICATION DASHBOARD[/bold cyan]"
    console.print(Panel(header_text, expand=False, style="bold cyan"))

def format_and_print(msg_json):
    category = msg_json.get("category", "").lower()
    message = msg_json.get("message", "")
    
    if category == "keepalive":
        color = "green"
        label = f"KEEPALIVE ✅"
    elif category == "alert":
        color = "yellow"
        label = f"ALERT ⚠️"
    elif category == "exit":
        color = "magenta"
        label = f"EXIT 🚪"
    else:
        color = "white"
        label = f"UNKNOWN ❓"
    
    panel_text = Text(f"{label}\n", style=f"bold {color}")
    panel_text.append(message, style=color)
    
    console.print(Panel(panel_text, expand=False, border_style=color))

def main():
    if len(sys.argv) < 2:
        print("Usage: dashboard.py <client_id>")
        sys.exit(1)

    client_id = sys.argv[1]
    print_header(client_id)

    try:
        mq = posix_ipc.MessageQueue(QUEUE_NAME, posix_ipc.O_CREAT)
    except Exception as e:
        console.print(f"[red]Error opening message queue: {e}[/red]")
        sys.exit(1)

    console.print("[bold green]Waiting for messages...[/bold green]")

    while True:
        try:
            message, _ = mq.receive()
            try:
                msg_json = json.loads(message.decode().rstrip('\x00'))
            except json.JSONDecodeError:
                console.print(f"[red]Invalid JSON: {message}[/red]")
                continue

            format_and_print(msg_json)

            if msg_json.get("category", "").lower() == "exit":
                console.print("[bold magenta]Exit message received. Closing dashboard...[/bold magenta]")
                time.sleep(2)
                break

        except KeyboardInterrupt:
            console.print("[bold red]Dashboard interrupted by user[/bold red]")
            break

    mq.close()

if __name__ == "__main__":
    main()
