# 1. General Description

The system consists on a client-server architecture using sockets to communicate. The system works as a logistic coordination server to manage clients basic communication, inventory and notifications (new features will be added in the future). 

The main executables are:
- **server**: the backend service that handles all logic and communication.
- **client**: a simple client to interact with the server.

# 2. Features

- User authentication (login with username and password)
- Inventory query and update (with permission control)
- Stock consultation
- Notifications and alerts
- Data backup and restore
- Metrics monitoring (Prometheus/Grafana)
- Logging and audit
- Client lock based on 3 consecutive failed login attempts or triggering an alert.

**Technical highlights:**
- Communication via TCP and UDP sockets (IPv4 and IPv6)
- Modular architecture for easy extension
- Exposes metrics endpoint for Prometheus
- Multiple connections managed by epoll + thread pool
- Docker image provided to run system on a container
- Scripts provided to backup resources and dedicated user configuration.

# 3. Executables

## Server

The main backend service of the application.  
When launched, it initializes all required resources (TCP, UDP, and UNIX sockets, database, and log files) and starts listening for client connections.

The server exposes:
- A TCP and UDP interface for client communication.
- An IPC (UNIX socket) interface for internal alerts and notifications.
- A metrics endpoint for Prometheus monitoring.

The server must be running for clients to connect and interact with the system.  
It manages user authentication, inventory updates, stock queries, notifications, and logs all relevant activity.

The server is designed to handle multiple simultaneous client connections efficiently and securely.
The server implements log rotation (default threshold size is 10Mb, but can be changed in the config.yaml). When the max sixe is reached, the log file is compressed and a new file is created to continue logging. The compressed file can be saved in a backup along with the data base file with a python script here provided.

## Client

The client is the frontend application used to interact with the server.

When launched, it initializes its resources (TCP and UDP sockets, log files) and attempts to connect to the server via TCP.  
If the connection is successful, the client prompts the user for login credentials and available commands to interact with the server (such as querying or updating inventory, depending on permissions).

Upon successful login:
- The client maintains a keepalive communication with the server.
- A notification dashboard is launched to receive real-time alerts from the server via UDP.

All activity is logged locally, and the client can be cleanly terminated at any time.

# 4. Requirements 

## Recommended (using Docker)

- [Docker](https://docs.docker.com/get-docker/) (tested with version 20.10+)
- [Docker Compose](https://docs.docker.com/compose/) (v2+)
- [Git](https://git-scm.com/) (for cloning the repository)

All other dependencies (CMake, compilers, libraries, Prometheus, Python, etc.) are handled automatically inside the Docker container.

## Optional (manual build, not recommended)

If you want to build and run the system outside Docker, you will need:

- CMake >= 3.25
- GCC/G++ >= 9 (with C++20 and C17 support)
- Python 3.x
- SQLite3 and libsqlite3-dev
- zlib1g-dev
- git
- wget
- Prometheus-cpp and its dependencies (must be installed manually)
- Grafana (optional) (must be installed manually)
- yaml-cpp, nlohmann-json, bcrypt, cJSON (these are fetched automatically by CMake)

# 5. Installation

Clone the repository:
```sh
git clone https://github.com/ICOMP-UNC/eager_galileo.git

cd eager_galileo

docker-compose up --build

```
If you manually installed all dependencies on your local host, you should be able to build and compile manually as follows:
- cmake -S . -B build (from the root project directory)
- To compile the server: cmake --build build --target server -- -j$(nproc)
- To compile the client: cmake --build build --target client -- -j$(nproc)

# 6. Launching and usage

## Launching
### Server

The server can be launched without arguments. Launching without arguments will automaticly use default path to config.yaml in root directory of the project root_project/etc/config.yaml, where ports are defined for both tcp and udp, and other configuration variables that you can change. You can specify the ports por tcp and udp via arguments when launching in the command line, via enviroment variables or changing the values in the config.yaml


### Using docker
After building the image, you can simply run 
```sh
docker-compose up galileo-server
```
If you want metrics with grafana: 
```sh
docker-compose up galileo-server prometheus grafana
```
If you wish to add arguments to launch the server, you should add them in the docker-compose.yaml under the galileo-server service in the command line: 
- command: build/bin/server <[optional]path to config> <[optional] tcp port> <[optional] udp port>

### Manually 
To run the server from project root directory:
```sh
./build/bin/server [optional_path_to_config.yaml] [optional_tcp_port] [optional_udp_port]
```
A script is provided under the scripts directory to run the server as a dedicated user and make it a service 

### Client

The client must be launched with the address of the server. If the server is locally running, command should be 'client localhost'. If not, the ip address should be indicated. Make sure the ports match the ones that the server is using. Defaults are 8888 for both tcp and udp.

### Using docker

After building the docker image, you can run the client in two ways to use the CLI
```sh
docker-compose run --rm galileo-client
```
This will create a new container for the client with a terminal so you can use the CLI

Or you can run the client and then open a terminal inside:
```sh
docker-compose up galileo-client
docker exec -it <name of the container> bin/bash
```
You can search for the container name with the `docker ps` command.

### Connecting from another network or host

To connect from a client running on a different machine or network, use the server's IP address and ensure the ports match the server configuration.

Example:
```sh
./build/bin/client 192.168.1.100 8888 8888
```
Where 192.168.1.100 is the IP address of the server, and 8888 is the TCP/UDP port used by the server. You should change the values to match the server configuration on the docker-compose.yaml file

### Manually

If all dependencies are installed, you can run the client by running the bin file, example from the root directory:
```sh
./build/bin/client <server address> <[optional] server tcp port> <[optional] server udp port> 
```

Make sure to use the right ports to match the ones that the server is using

## Using the client

When launched the client will acept commands via CLI. You should first login with valid credentials to be able to interact with the server. The admin of the server must register your user and password. Example:
```sh
login user password
```
 If 3 consecutive failed login attemps are reached, the user will be locked for 15 minutes. Once loged in, a dashboard will be launched where notifications from the server will be recieved and displayed such as alerts, keepalive messages or exit messages. Some commands are available depending on the type of user (Hub, Warehouse or admin). Hubs can consult individual stock, inventory history and full inventory stock. Examples:
 ```sh
 get_stock food water
 get_inventory
 get_history
 ```
  Warehouses in addition to the mencioned comands, can update stock. Stock products are already predifined in the server, so invalid types of stock will be rejected. Example: 
  ```sh
  update_stock medicine bandages 500
  ```
The admin user in addition to previous commands, can unlock clients when locked due to triggering an alert with a secret passphrase defined in the config.yaml file. Admin client can also register new users in the system. Examples:
```sh
unlock_client username_to_unlock secret_phrase
register_user username password 
```

To close the client gracefully you can enter the end command:
```sh
end
```
This will clean all open resources and exit the client.

# 7. Monitoring & Metrics

This system exposes runtime metrics for monitoring via [Prometheus](https://prometheus.io/) and [Grafana](https://grafana.com/).

## Using Docker (Recommended)

1. **Build and launch the monitoring stack:**
   ```sh
   docker-compose up galileo-server prometheus grafana
   ```
   - This will start the server, Prometheus, and Grafana containers.
   - Prometheus will automatically scrape metrics from the server (as configured in `prometheus.yml`).

2. **Access the monitoring dashboards:**
   - **Prometheus UI:** [http://localhost:9091](http://localhost:9091)
   - **Grafana UI:** [http://localhost:3001](http://localhost:3001)
     - Default login: `admin` / `admin`

3. **Add Prometheus as a data source in Grafana:**
   - Go to Grafana → “Add data source” → Select **Prometheus**.
   - Set the URL to `http://prometheus:9090` (from Grafana’s perspective inside Docker Compose).
   - Save & Test.

4. **Create dashboards:**
   - You can now create custom dashboards in Grafana using the metrics exposed by the server.

### Important notes:
- The server must be running and exposing its metrics endpoint (default port: `8086`).
- The `prometheus.yml` file in the project root is mounted into the Prometheus container and should list the correct target (e.g., `galileo-server:8086`).
- If you change the metrics port or service name, update `prometheus.yml` accordingly.

---

## Manual Setup (Without Docker)

1. **Install Prometheus and Grafana** on your system (see their official docs).
2. **Configure Prometheus:**
   - Edit your `prometheus.yml` to add a scrape target pointing to your server’s metrics endpoint, for example:
     ```yaml
     scrape_configs:
       - job_name: 'galileo-server'
         static_configs:
           - targets: ['localhost:8086']
     ```
   - Start Prometheus with:
     ```sh
     prometheus --config.file=prometheus.yml
     ```

3. **Start Grafana** and access [http://localhost:3000](http://localhost:3000).
4. **Add Prometheus as a data source** in Grafana (URL: `http://localhost:9090`).
5. **Create dashboards** as desired.

### Manual setup notes:
- Ensure the server is running and accessible from Prometheus (check firewalls and ports).
- If running Prometheus or Grafana in Docker but the server outside, make sure to use the host’s IP address in `prometheus.yml`.
- If running everything on different machines, ensure network connectivity and open ports as needed.

---

# 8. Logs & Backups

## Logging (Client & Server)
- **Both the client and server components generate activity logs.**
  - The **client** writes logs to local files, ensuring thread safety via mutexes and creating the log directory if it does not exist.
  - The **server** implements an advanced logging system:
    - Logs are written to a file and **each log entry is also persisted in the database** for audit and traceability.
    - Log format includes UTC timestamp, component, level, and message.
    - Supported log levels: DEBUG, INFO, WARNING, ERROR.

### Log Rotation (Server)
- The server implements **automatic log rotation**:
  - When the log file reaches the configured maximum size, it is renamed with a timestamp and automatically compressed as `.gz` to save space.
  - The active log file is recreated and logging continues seamlessly.

### Automated Backups
- The `scripts/backup.py` script is provided to automate backups of the database and compressed logs.
  - The script:
    - Creates compressed (`.gz`) backups of the database and copies all compressed logs to the `backup/` directory.
    - Can be run manually or scheduled to run automatically (e.g., weekly) using `cron`.
    - **Detailed automation instructions are included as comments within the script itself.**

### Persistent Volumes in Docker Compose
- The `docker-compose.yaml` file defines **persistent volumes** to ensure that data and logs are not lost when containers are restarted or recreated:
  - `./var/lib:/Last_of_Us_System/var/lib` (database)
  - `./var/logs:/Last_of_Us_System/var/logs` (client and server logs)
  - `./backup:/Last_of_Us_System/backup` (generated backups)
- This allows access to log and backup files from the host, making management and auditing outside containers straightforward.

### Best Practices & Notes
- Log and backup directories are automatically created with secure permissions (`0755`) if they do not exist.
- It is recommended to monitor available disk space, especially if many logs or backups are generated.
- Rotated and compressed logs can be deleted manually or with additional scripts if periodic cleanup is required.
- The system is designed to be robust against failures: critical logs are stored both in files and the database, and backups can be automated to minimize data loss in case of incidents.

# 9. Troubleshooting

## Common Issues

- **Connection Refused / Timed Out:**
  - Ensure the server is running.
  - Check firewall settings to ensure ports are open.
  - Verify the server address and port in the client command.

- **Permission Denied (on files or sockets):**
  - Ensure correct file and directory permissions.
  - If running inside Docker, ensure volumes are correctly mounted with appropriate permissions.

- **Log or Database Files Missing:**
  - Ensure the server has permission to create and write to these files.
  - Check the configured paths in `config.yaml`.

- **Metrics Not Showing in Prometheus/Grafana:**
  - Ensure the server is running and the metrics endpoint is accessible.
  - Check Prometheus configuration (`prometheus.yml`) for the correct scrape target.
  - If running in Docker, ensure network settings allow communication between containers.

## Debugging Tips

- Check the log files for both the server and client for any error messages or warnings.
- Use tools like `curl` or `wget` to test connectivity to the server's metrics endpoint from within the Grafana container (if using Docker).

# 10. Known Issues

- The client and server must have matching port configurations; mismatches will cause connection failures.
- If running outside Docker, file permissions on log and database directories may cause errors; ensure correct ownership.
- User registration is currently only available to admin users via command line.
- Log rotation and backup rely on available disk space; low disk space may cause failures.
- If Prometheus or Grafana are run outside Docker, network/firewall settings may block access to metrics.
- The system has not been tested on Windows hosts.
- There is **no reconnection or logout command** for the client: once logged in, to log out you must close and restart the client application.
- There is **no mechanism to prevent the same user from logging in simultaneously from multiple clients**. The same user can be logged in from two different clients at the same time, which may cause undefined or unexpected behaviors.
- When running the server in Docker, **console output is not visible** (e.g., startup success or failure messages). In this case, you must check the log file for server status and errors.
