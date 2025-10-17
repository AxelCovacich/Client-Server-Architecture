# 1. Introduction

This system is the foundation for future implementations and features of the complete project. It is based on a client-server environment where the server manages connections using TCP and UDP sockets. Clients can connect to the server using its address and ports, and interact via CLI commands.
    
The system is set in a 'Last of Us' world scenario with the central server coordinating Hubs and Warehouses, each with inventory, security, and notification systems. The main goal of this project is to establish a functional server that efficiently, safely, and concurrently manages basic communications with multiple clients, supporting many simultaneous connections without performance degradation.

# 2. System Architecture

The system follows a client-server architecture. The central server is responsible for managing all business logic, client connections, inventory data, notifications, and security. Clients connect to the server using TCP and UDP sockets, interacting via a command-line interface.

**Main components:**
- **Server:** Handles multiple simultaneous client connections, manages authentication, inventory, notifications, logging, and exposes metrics for monitoring.
- **Clients:** CLI applications that connect to the server, authenticate, send commands (such as inventory queries or updates), and receive notifications.
- **Monitoring stack:** Prometheus collects runtime metrics exposed by the server, and Grafana provides dashboards for visualization.
- **Backup scripts:** Python scripts are provided to automate backup of logs and database files.

**Communication:**
- Clients connect to the server over TCP for commands and session management.
- UDP is used for real-time notifications from the server to clients.
- Internal server components may use UNIX sockets for IPC.

**Persistence and monitoring:**
- All persistent data (database, logs, backups) are stored in dedicated directories, which are mapped as Docker volumes for durability.
- The server exposes a metrics endpoint for Prometheus, enabling real-time monitoring and alerting.

**System Architecture Diagram:**

```mermaid
---
config:
  layout: dagre
---
flowchart LR
    subgraph Client["Client (C Executable)"]
        CLI["CLI (Command Line Interface)"]
        Dashboard["Python Dashboard"]
        CLI -- "POSIX MQ (IPC, internal)" --> Dashboard
    end

    subgraph Server["Server (C++)"]
        Srv["Main Server"]
        AlertSensor["Python Alert Sensor"]
        AlertSensor -- "UNIX File (IPC)" --> Srv
    end

    CLI <-->|"TCP (JSON, bidirectional)"| Srv
    CLI <-->|"UDP (Keepalive, bidirectional)"| Srv
    Srv -- "UDP (Notifications, server to client)" --> CLI

    Srv -- "Metrics (Prometheus-cpp)" --> Prometheus[Prometheus]
    Prometheus -- "Data Source" --> Grafana[Grafana]

    %% Volumes for persistence
    Srv -- "DB, Logs, Backups (SQLite via SQLiteCpp, Docker Volumes)" --- Storage[(Persistent Storage)]
  
  ```

**Description & Clarifications:**

- **CLI (Command Line Interface):**
  Handles user commands and communicates with the server via TCP (JSON messages, bidirectional). UDP and IPC are handled internally by the client, not exposed via CLI.

- **TCP (JSON):**
  Bidirectional communication; client sends commands, server responds with acknowledgements, data, or errors.

- **UDP (Keepalive):**
  Bidirectional; client sends periodic "ping" messages, server responds with "pong" or status.

- **UDP (Notifications):**
  Unidirectional; server sends notifications to client.

- **IPC (POSIX MQ):**
  Client uses internal IPC to send notifications to the Python dashboard for display.

- **Alert Sensor (Python):**
  Sends alerts to the server via IPC (UNIX file); server reads these alerts and broadcasts them to clients.

- **Persistence:**
  All persistent data (database, logs, backups) are stored using SQLite (via SQLiteCpp library) and mapped as Docker volumes for durability.

- **Metrics:**
  Server exposes metrics using prometheus-cpp, scraped by Prometheus and visualized in Grafana.

> **Notes:** 
- SQLiteCpp is a C++ wrapper for SQLite, which is a lightweight, file-based relational database (not MySQL).
- All persistent data (database, logs, backups) are stored in mapped Docker volumes for durability.


# 3. Design Decisions

# 4. Implementation Details

# 5. Requirements Coverage

# 6. Testing & Validation

# 7. Issues & Solutions

# 8. Limitations & Future Work

# 9. Conclusions