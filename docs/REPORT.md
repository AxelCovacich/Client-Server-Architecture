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
        Cli["Hub or Warehouse"]
        Dashboard["Python Dashboard"]
        Cli -- "POSIX MQ (IPC, internal)" --> Dashboard
    end

    subgraph Server["Server (C++)"]
        Srv["Main Server"]
        AlertSensor["Python Alert Sensor"]
        AlertSensor -- "UNIX File (IPC)" --> Srv
    end

    Cli <-->|"TCP (JSON)"| Srv
    Cli <-->|"UDP (Keepalive)"| Srv
    Srv -- "UDP (Notifications)" --> Cli

    Srv -- "Metrics (Prometheus-cpp)" --> Prometheus[Prometheus]
    Prometheus -- "Data Source" --> Grafana[Grafana]

    %% Volumes for persistence
    Srv -- "DB, Logs, Backups" --- Storage[(Persistent Storage)]
  
  ```

**Description & Clarifications:**

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
- All persistent data (database, logs, backups) are stored in mapped Docker volumes.


# 3. Design Decisions

Python was chosen for the scripts because it is easier and faster to write small utilities and dashboards compared to C or C++. This allowed for quick setup of components like the dashboard and the alert sensor without the need to manage low-level details.

Regarding modularity, the code was divided by responsibility and function. For example, a main class was implemented to start all components, a server class to handle connections and configuration, an authenticator class dedicated to authentication, a storage class for database communication and so on with each class. Each class is responsible for a specific task, which makes the code easier to understand and maintain.

TCP was selected for commands to ensure that no messages are lost. Commands are important and always require a response from the server, whether it is an acknowledgment or data. TCP provides reliable communication, which is suitable for this purpose.

UDP was used for notifications and keepalive messages because it is faster and the information is not critical. If a notification or keepalive message is lost, it does not affect the system significantly, making UDP an appropriate choice.

For IPC, at least one mechanism was required in both the server and client. On the client side, a POSIX message queue was used because it allows setting priorities for messages, which is useful for notifications—more important messages can be handled first. On the server side, a UNIX file was used for the alert sensor, as it is simple and practical: the sensor writes alerts to the file, and the server reads them.

SQLiteCpp was selected for the database due to its ease of integration with the codebase. While it may not be the most efficient database available, it is sufficient for this project and requires minimal setup. With CMake's FetchContent, it can be fetched and used immediately.

Log rotation and backup were implemented as required. A configurable maximum size for the server log is set in a YAML file. When the log exceeds this size, it is compressed and a new log file is created. Additionally, a Python script is provided to back up the database and the compressed log files.

## Concurrency and Performance

Initially, the server used the `select` system call with a one-thread-per-connection approach for handling client connections. While this worked well for a small number of clients, it quickly became unfeasible for supporting a large number of simultaneous connections (e.g., 10,000), as each thread consumes significant system resources and context switching becomes a bottleneck.

To address this, the `clientSession` class and the server's main loops were restructured to integrate `epoll` with a thread pool. This approach allows the server to efficiently handle many concurrent connections: `epoll` monitors file descriptors for read/write events without blocking on each connection, and any thread from the pool can process incoming events. This design enables the server to scale to thousands of clients with minimal performance degradation. The `threadpool` class was created to manage the pool of worker threads, with a default of 16 threads, and to assign tasks as events are detected.

## Connection, Authentication and Permissions

When a client connects, it is assigned a `clientSession` object that persists for the duration of the connection. This object manages the entire lifecycle for the client, including command interpretation, login, and all interactions. A `sessionManager` oversees all active `clientSession` instances, maintaining lists of sessions, offline users, and locked users.

Upon connecting, a user must authenticate before being allowed to execute any commands. Each user has three login attempts; after three failures, the account is locked for 15 minutes. If the login is successful, the session state is updated to authenticated, allowing access to further commands.

User registration is currently handled by the admin through a special `register_user` command, which only the admin can execute. (This process is not fully automated and could be improved; this limitation is also noted in the known issues section.)

Each user has specific command permissions:  
- **Hubs** can query stock, inventory, and history (read-only operations).
- **Warehouses** have all the permissions of Hubs, plus the ability to update stock.
- **Admin** users can perform all previous actions, unlock users blocked by alerts, and register new users.

If a user triggers an alert (simulated by the Python sensor), that user is locked until the admin unlocks the account using a secret phrase stored in the `config.yaml` file.

For authentication and security, passwords are stored as bcrypt hashes in the database, ensuring secure authentication even though this adds some computational overhead. Logging of password entries is handled carefully: a small function censors the password in logs to avoid exposing sensitive information.

## Error Handling and Data Integrity

To ensure error prevention and data integrity, try-catch blocks were used throughout the code, especially during message parsing, command execution, and data validation. On the client side, every command entered by the user is checked for correct format; if the input is invalid, the client notifies the user with an error and the expected usage. The client also attempts to construct JSON messages properly before sending them.

On the server side, all expected fields in incoming messages are validated. If any required field is missing or invalid, the server throws a controlled exception and informs the client of the error. The severity of the error determines the response: for example, if an attempt is made to update an invalid stock item (the inventory maintains a list of valid stock), the server notifies the client of the error.

Every important action that involves transmitting data or passing information between functions is validated. For example, in the storage module, all operations are wrapped in try-catch blocks to ensure that any failure is handled appropriately—whether by notifying the client, closing the connection, or shutting down the server in case of a critical error.

In the configuration class, the contents of the `config.yaml` file are read and validated. If any required value is missing or invalid, the server automatically shuts down to prevent running with incorrect settings.

## Monitoring and Testing

Prometheus and Grafana were integrated to provide real-time metrics for message counts and errors, which was also a non-functional requirement. Metrics exposed include counts of TCP, UDP, and IPC messages, successful operations, errors, connections, and reconnections. These metrics are useful for monitoring the server and overall system status, detecting potential issues with specific protocols, observing behaviors, and gaining insight into server performance.

For testing, unit tests were implemented using Unity, as it was practical and straightforward for this project. Unit tests were written to cover each class and relevant cases, focusing on expected logic and error handling rather than complex integration scenarios such as TCP connection testing. The achieved test coverage is approximately 70% for both server and client components. Quality was ensured by testing edge cases and expected behaviors wherever possible. Additional tools such as Clang, SonarQube, CDash, and Valgrind were used to support code quality and reliability.


# 4. Implementation Details


## General Structure

The project is organized following the Linux Filesystem Hierarchy Standard (FHS), with all components contained within a single workspace directory. This means there are no system-wide installations required, except for external dependencies such as Grafana or Prometheus.

### Server

The server is built around several key classes, each responsible for a specific aspect of the system. Here are detailed some of the most relevant:

- **Server:** Handles configuration, manages network connections, and sets up all sockets (TCP, UDP, UNIX).
- **ClientSession:** Represents an individual client connection; one instance is created for each TCP connection.
- **CommandProcessor:** Processes user commands received from clients.
- **Storage:** Interfaces with the database for all data operations.
- **Logger:** Manages logging of server activity and log rotation.
- **Inventory:** Handles inventory management and stock operations.
- **Authenticator:** Manages user authentication and password verification.
- **UdpHandler:** Manages UDP communication for notifications and keepalive messages.
- **IpcHandler:** Handles inter-process communication for alerts and dashboard integration.
- **AlertManager:** Processes and broadcasts alerts received from the alert sensor.

### Client

The client is implemented in C and organized as small modules, each with a focused responsibility. The main modules are:

- **client:**  Configures and establishes TCP/UDP sockets (single setup function accepts a protocol parameter). Provides a cleanup function that closes sockets, shuts down the IPC queue and logger, and releases other resources.

- **client_context:**  Holds runtime state in a struct (TCP/UDP sockets, IPC queue descriptor, client_id, mutex, etc.). Acts as the shared context passed to other modules for accessing and updating client state safely.

- **input_handler:**  Reads user input, determines the user action (send, quit, continue, error) and builds JSON messages to send to the server. Returns a `json_build_result` (string + status enum) so callers can handle syntax or memory errors distinctly.

- **ipc_handler:**  Manages the POSIX message queue used to send notifications to the Python dashboard. Initializes the queue, enqueues messages with priority, and provides safe shutdown semantics.

- **logger:**  Opens/creates the log file, serializes writes with a mutex, and exposes `logger_log`/`logger_close` for modules to record events and errors.

- **output_handler:**  Formats and prints server responses for the user. Detects successful login responses and stores the returned client_id in the `ClientContext`.

- **session_handler:**  Orchestrates the client ↔ server communication loop. Starts the main send/receive loop, executes transactions (send JSON, read reply), and launches auxiliary threads (keepalive sender, UDP listener, dashboard launcher) after successful login.

- **transport:**  Provides low-level TCP/UDP I/O functions (tcp_send/recv, udp_send/recv) and manages the peer UDP address used by `sendto`. Acts as the abstraction over socket API used by the session handler.

- **udp_handler:**  Implements the UDP listener thread and the keepalive thread. Received UDP messages are forwarded to the `ipc_handler` for the dashboard; keepalives are periodically sent to the server using the transport helpers.


### Key Features

#### Connection handling
Connections are managed via sockets configured with `getaddrinfo` for both IPv4 and IPv6 (TCP and UDP). All server sockets are set non-blocking and registered with `epoll`. The main server loop calls `epoll_wait` and dispatches events until a shutdown signal is received.

- On a new TCP connection the server `accept()`s the socket, makes it non-blocking, adds it to the `epoll` instance and creates one `ClientSession` (shared_ptr) per connection. That `ClientSession` owns the lifecycle for that client (receive/parse messages, authenticate, process commands, send responses).
- UDP and UNIX (IPC) sockets are also added to `epoll`; UDP events are delegated to the `UdpHandler`, and new UNIX connections are handled by `IpcHandler` tasks in the thread pool.
- `epoll` is used together with `EPOLLONESHOT` to avoid concurrent event handling on the same FD. After a worker finishes processing a session it re-arms the FD (adds `EPOLLIN` and `EPOLLOUT` when pending writes exist).

#### Authentication and permissions

Authentication and permissions are centralized in the `Authenticator` and `SessionManager`:

- Users and bcrypt-hashed passwords are stored in the database. Login attempts are validated against those records.
- A user has three login attempts; on three consecutive failures the account is locked for 15 minutes.
- Roles and permissions are enforced by checking the user role (Hub, Warehouse, Admin) before executing commands:
  - Hubs: read-only (inventory, stock, history).
  - Warehouses: read/write (can update stock).
  - Admin: full privileges (including user registration and unlock).
- User registration is currently performed by an admin-only command. The initial admin is created at startup (not yet automated).

#### Concurrency, epoll and thread pool

To scale to many simultaneous clients the server uses an `epoll` + thread pool model:

- `epoll` efficiently monitors thousands of file descriptors for readiness (it avoids scanning all FDs like `select`/`poll`).
- Sockets are non-blocking; `epoll_wait` returns ready FDs and worker threads from the `ThreadPool` perform the heavy work (reading, parsing, command processing).
- `EPOLLONESHOT` is used so only one thread handles a given FD at a time; the FD is re-armed after processing.
- Critical shared resources (storage, inventory, authenticator, logger, session lists) are protected by mutexes to guarantee thread-safety.
- This design minimizes context switches and resource usage versus a thread-per-connection model, enabling the server to handle large numbers of concurrent connections.

#### Storage (database)

The `Storage` module is the single interface to persistent data:

- It opens/creates a SQLite file under `project_root/var/lib` and initializes the schema (tables: `users`, `inventory`, `logs`).
- All DB reads/writes go through `Storage` methods (e.g., `createUser`, `getStock`, `saveStockUpdate`, `userExists`).
- Access to the DB is serialized where required (mutex) to maintain consistency.
- SQLiteCpp is used as the C++ wrapper; CMake fetches and configures it automatically.

#### Logging and rotation

Logging follows the project requirements:

- Each module uses the shared `Logger` instance and calls `log(level, component, message, clientID)`; writes are protected by a mutex.
- Logs are persisted in a plain log file and also recorded/usable via the `logs` DB table.
- A maximum log file size is configured in `config.yaml`. When the limit is reached the current log file is compressed (gzip via zlib) and a new log file is created (rotation). Compressed files are kept for backup.
- A Python backup script is provided to periodically save the database file and compressed logs.

#### Scripts and IPC

Scripts live in the `scripts/` folder and cover administration and simulated sensors:

- An alert sensor script simulates external alerts by writing messages to the server's UNIX IPC file (alert type, message, optional clientID). The server reads these entries and uses `AlertManager` to broadcast alerts to clients.
- A backup script automates periodic backup of the SQLite DB and compressed logs (instructions inside the script).
- A helper bash script is provided to create a dedicated system user and install the server as a service.

#### Metrics (Prometheus & Grafana)

Monitoring is implemented with `prometheus-cpp` and Grafana:

- `TrafficReporter` registers counters/gauges for TCP/UDP/IPC message counts, successful operations, errors, connections and reconnections.
- Metrics are exposed on the configured port (via `TrafficReporter::startPrometheusExposer`).
- Prometheus scrapes the exposed endpoint and Grafana visualizes the metrics in dashboards (admin must add Prometheus as Grafana data source).

### Client features

- Ports and startup:
  - TCP/UDP ports can be set via environment variables or passed as command-line arguments. Default ports are used if none are provided.
- Network support:
  - IPv4 and IPv6 support for TCP and UDP is provided via getaddrinfo.
- Command input and validation:
  - Commands are entered via the keyboard. Unknown commands may be sent (the server will ignore or reject them), but the client does not treat them as local errors.
  - The `input_handler` validates command syntax and required arguments before sending. If a required field is missing (e.g., `update_stock` without the stock id), the user is informed of the correct usage and asked to re-enter the command.
- Login and dashboard:
  - After a successful login the Python notification dashboard is launched automatically and UDP keepalive begins.
  - Keepalive: the client sends a ping every 1 minute and expects a pong/status reply from the server.
  - Server UDP notifications and keepalive responses are displayed in the dashboard; the command-line remains available for further input while the dashboard runs.
- Session lifecycle:
  - The `end` command or a termination signal (e.g., Ctrl-C) closes the client and the dashboard (if launched) cleanly.
  - There is currently no reconnection or logout command—closing the client is required to end a session.
- Output formatting:
  - The `output_handler` formats server responses into human-readable output (stock, inventory, history, error messages).
- Logs and backups:
  - The client logs to a local logfile but currently has no rotation or backup mechanism (logs are expected to be small).

### Main Flows

#### Client session lifecycle (command flow)
- A new TCP connection is accepted, set non-blocking and registered with epoll. The server creates one `ClientSession` (shared_ptr) per connection.
- `ClientSession::run()` is the main per-connection loop invoked by a worker thread after epoll signals activity on the socket. It performs the read, JSON parse and calls `processMessage(...)`.
- Messages between client and server use TCP with JSON. `processMessage` returns a `processResult` (`std::pair<std::string,bool>`) where the string is the JSON response and the bool indicates whether the session should remain open.
- If the client is not authenticated and the message is not a login, the server returns an error asking the user to authenticate. If the message is a login, `Authenticator` validates credentials (bcrypt hashes stored in DB). On success the session state (authenticated boolean) is set and the client can issue other commands.
- Authenticated commands are routed to `CommandProcessor` which executes the requested action by calling the responsible module (e.g., `Inventory`, `Storage`). `CommandProcessor` builds the response based on the operation result and returns it to the `ClientSession`.
- The worker thread sends the JSON response back to the client. If `processResult.second` is false (e.g., fatal error or `end` command), the session is closed and cleaned up .
- UDP is used in parallel for keepalive and notifications. Keepalive messages are handled by `UdpHandler::handleKeepalive`, which updates the session UDP address via `SessionManager` and replies with a PONG. Notification delivery (server→client) uses UDP; if a target client is offline the message is enqueued in `EventQueue` for later delivery.
- Critical sections (storage, inventory, session lists, logger) are protected by mutexes to ensure thread-safety when multiple worker threads process events concurrently.

#### Alert flow (sensor → server → clients)
- The alert sensor (Python script) writes alert messages to the server's UNIX IPC file.
- An epoll notification for the IPC FD schedules `IpcHandler` work in the thread pool. A worker reads the alert message from the FD and forwards it to `AlertManager`.
- `AlertManager` parses the alert (type, message, optional `clientID`). Certain alert types (e.g., `enemy_threat`, `infection`) trigger a user lock: `SessionManager` marks the affected `clientID` as locked. Informational alerts (e.g., `weather`) do not lock users.
- The alert is then broadcast via `UdpHandler::broadcastMessage` to all active clients. `UdpHandler` looks up active UDP addresses from `SessionManager` and sends the alert via `sendto`. Each successful/failed send updates metrics via `TrafficReporter`.
- For clients that are offline, the alert is queued using `EventQueue.pushEvent(user, Event{...})`. When the client next reconnects, queued events are delivered (server checks `hasPendingMessages()` / `trySendPendingMessage()` in the `ClientSession` flow).
- Note: only authenticated (logged-in) sessions receive notifications. If a session is not authenticated, it will not receive UDP notifications.

#### Additional notes
- epoll + thread pool model: epoll detects ready FDs and worker threads perform I/O and command processing; `EPOLLONESHOT` is used to avoid concurrent handling of the same FD and FDs are re-armed after processing.
- Responses are always constructed as JSON with status/metadata; error handling and input validation are enforced at each parsing/execution step and communicated back to the client.

### Validation and Error Handling

- Input validation is performed at every step where external data is received or transferred. JSON messages are parsed and required fields are checked (clientSession, CommandProcessor). Missing or malformed fields are caught and produce a controlled error response to the client.

- Exceptions are handled with try/catch blocks around parsing and execution paths:
  - `clientSession::processMessage` parses incoming JSON inside a protected block and handles expected JSON exceptions (missing fields, type errors). If parsing fails, a clear JSON error reply is returned and the session remains open (unless the error is critical).
  - Unexpected exceptions that propagate to the top of `processMessage` are caught and treated as internal errors: an error reply is sent and the session is closed to avoid undefined behavior.

- Error severity determines the reaction:
  - Recoverable errors (bad input, validation failures) generate an error response to the client and do not terminate the server or necessarily close the session.
  - Fatal or internal errors (critical exceptions, invalid configuration at startup) may lead to closing the client session or shutting down the server (for example, invalid `config.yaml` makes the server exit on startup).

- Each component returns explicit success/failure indicators where appropriate (bool or int return values). Calling code checks these results and acts accordingly (retry, notify client, close session).

- Logging and metrics are integrated with error handling:
  - The shared `Logger` records warnings, errors and critical events (passwords are redacted before logging).
  - `TrafficReporter` counters are incremented for protocol errors (e.g., `incrementError("tcp","rx")`) so monitoring reflects validation and runtime issues.

- Concurrency and safety: critical sections (storage, inventory, authenticator, session lists, logger) are protected with mutexes. Storage and other modules validate inputs and wrap DB operations in try/catch to avoid corrupt state; failures are reported and handled centrally.

- Configuration validation: the `Config` module validates `config.yaml` on startup. If required values are missing or invalid, the server logs the issue and exits to prevent running with incorrect settings.

Overall, the system favors explicit validation, controlled exception handling, informative client responses, and conservative shutdown behavior for unrecoverable errors.

### Client-side Flows (function-level)

This section describes the main runtime flows on the client side from the point of view of functions/modules. It explains the path a command or notification follows from user input to server and back, and how auxiliary threads and IPC are used.

### Command / request flow (user → client → server → user)
1. The CLI reads user input via input_handler:
   - input_handler reads a line, determines the action (UserInputAction) and builds the JSON payload (json_build_result).
   - If the input is syntactically invalid, input_handler returns an error status and prompts the user again (no network send).

2. The session is executed by session_handler:
   - session_handler.start_communication(...) initializes the transaction and enters communication_loop().
   - communication_loop calls execute_client_action(...) for each user action.

3. execute_client_action(...) performs the network transaction:
   - It calls transport (transport.tcp_send) to send the JSON string over the TCP socket.
   - It then waits for the server reply with transport.tcp_recv
   - The raw reply is passed to session_handler.session_process_server_msg(...).

4. Response processing and UI:
   - session_handler forwards the server JSON to output_handler.print_readable_response(...).
   - output_handler formats the data (stock, inventory, history, errors) and prints it to the console.
   - If the response indicates a successful login, output_handler informs session_handler so it can store client_id into ClientContext and launch post-login actions.

5. Post-login actions:
   - session_handler.session_start_aux_threads(...) is invoked after successful authentication.
   - Auxiliary threads launched include:
     - keepalive thread (udp_handler.keepalive_thread_func) that periodically sends a ping via transport.udp_send.
     - UDP listener thread (udp_handler.udp_listener_thread_func) that listens for notifications.
     - Dashboard launcher (session_handler.launch_dashboard) that initializes ipc_handler and opens the POSIX message queue.

6. Closing:
   - The `end` command or termination signal triggers session_handler to cleanly shutdown auxiliary threads, close sockets and call client_cleanup() to free resources.

### Notification flow (server UDP → client UDP listener → ipc → dashboard)
1. The UDP listener thread (udp_handler.udp_listener_thread_func) blocks on the UDP socket via transport.udp_recv.
2. On receipt of a UDP notification:
   - udp_handler passes the message to ipc_handler.ipc_send_message(...), adding a priority if required.
   - ipc_handler enqueues the message to the POSIX message queue (mq_send) consumed by the Python dashboard.
   - The dashboard reads the POSIX MQ and displays notifications; the client console remains available for command entry.

3. Keepalive/UDP bidirectional behavior:
   - keepalive thread sends periodic ping messages using transport.udp_send.
   - The server replies with pong/status; udp_listener processes the pong as a lightweight status update (may update session state or metrics).

4. Offline delivery:
   - If the client is not currently connected or cannot receive UDP, the server may queue events. When the client reconnects, the server automaticly will attempt to deliver queued content and the queued notifiations will be displayed on the dashboard when launching.

### Error handling, validation and shutdown
- Every stage validates inputs/outputs: input_handler validates user commands; session_handler and handle_server_transaction validate server replies.
- session_handler contains a signal handler that sets exit_requested (`volatile sig_atomic_t`) to safely terminate loops from signal context. The main communication loop checks this flag and performs an orderly shutdown.
- On fatal errors or invalid configuration, the client performs a clean shutdown: stop auxiliary threads, close IPC queue, close sockets, flush logs and exit.

## Class Diagram for Server

The follow diagram shows the interactions between each module nad its main attributes. For simplicity, only the most relevant attributes or methods are listed.

```mermaid 
---
config:
  layout: dagre
---

classDiagram
    class Server {
        +Server(Config, SystemClock, Storage, Logger, TrafficReporter)
        +run()
        +getClientIp()
        -setupServer()
        -setTCPConfig()
        -setUDPConfig()
        -setUNIXconfig()
        -handleTcpConnection()
        -handleUNIXConnection()
        -tcpHandling(fd)
        -pendingMessages(fd)
        -m_clientSessions : map < int, ClientSession >
        -m_tcpPort: int;
        -m_udpPort: int;
        -m_serverTCPFD: int;
        -m_serverUDPFD: int;
        -m_serverUnixFD: int;
        -m_epollFD: int;
        -m_config: Config&;
        -m_clock: SystemClock&;
        -m_storage: Storage&;
        -m_logger: Logger&;
        -m_traffocReporter: TrafficReporter&;
        -m_threadPool: ThreadPool;
        -m_eventQueue: EventQueue;
        -m_inventory: Inventory;
        -m_authenticator: Authenticator;
        -m_alert: AlertManager;
        -m_sessionManager: SessionManager;
        -m_udpHandler: UdpHandler;
        -m_ipcHandler: IpcHandler;
    }
    class enable_shared_from_this
    class ClientSession {
        +ClientSession(int, Inventory, Authenticator, Logger, Config, TrafficReporter, EventQueue, UdpHandler)
        +run(): bool
        +isAuthenticated(): bool
        +processMessage(json_string: string): processResult
        +createLoggableRequest(request: json): json
        +setUdpAddress(addr: sockaddr_storage)
        +getUdpAddress(): shared_ptr<sockaddr_storage>
        +handleEventQueue(): bool
        +trySendPendingMessage(): bool
        +hasPendingMessages(): bool
        +sendWelcomeMessage(): bool
        -processResult: pair< string, bool > (type alias)
        -m_clientSocket: int;
        -m_clientID: string;
        -m_isAuthenticated: bool;
        -m_clientIP: string;
        -m_udpAddress: shared_ptr < sockaddr_storage >
        -m_sessionMutex: mutex;
        -m_pendingMessages: deque< string >
        -m_trafficReporter: TrafficReporter&;
        -m_inventory: Inventory&;
        -m_authenticator: Authenticator&;
        -m_eventQueue: EventQueue&;
        -m_logger: Logger&;
        -m_storage: Storage&;
        -m_config: Config&;
        -m_sessionManager: SessionManager&;
        -m_udpHandler: UdpHandler&;

    }
    class CommandProcessor {
        <<namespace>>
        +processCommand() :commandResult
        +handleStatusCommand()
        +handleEndCommand()
        +handleGetInventoryCommand()
        +handleGetStockCommand()
        +handleUpdateStockCommand()
        +handleGetHistoryCommand()
        +handleUnlockClientCommand()
        +handleRegisterUserCommand()

    }
    class Storage {
        -m_db: SQLite::Database;
        -m_dbMutex: std::mutex;
        +Storage(string &dbPath, int openFlags)
        +initializeSchema()
        +readData() 
        +writeData()
        +createUser(hostname, password)
        +saveStockUpdate()
        +getStock()
        +userExists()

    }
    class Authenticator {
        -m_clock: SystemClock&;
        -m_mutex: std::mutex;
        -m_storage: Storage&;
        -m_logger: Logger&;
        +Authenticator(Storage, SystemClock, Logger)
        +authenticate(hostname, password, BlockDuration):AuthResult
    }
    class Inventory {
        -m_mutex: std::mutex;
        -m_storage: Storage&;
        -m_logger: Logger&;
        -m_validItems: map< string, set< string>>;
        -m_fullyCachedClients: set< string >;
        -m_inventories: map< string, map< string, map < string, int>>>
        +Inventory(Storage, Logger)
        +getInventorySummary()
        +updateStock()
        +getStock()
    }
    class Logger {
        -m_logFile: offstream;
        -m_fileMutex: mutex
        +Logger(Storage&, SystemClock&, ostream&, Config&)
        +log(level, component, message, clientID)
        +closeLogFile()
        +logRotation()
        +openLogFile(filePath)
        +compressFileGzip()
    }
    class SessionManager {
        -m_activeSessions: map< string, shared_ptr< clientSession >>;
        -m_lockedClients: set< string >;
        -m_connectedUsers: unordered_set< string >;
        -m_offlineUsers: unordered_set< string >
        -m_mutex: mutex;
        +SessionManager(Storage&, Logger&, TrafficReporter&) 
        +registerSession()
        +removeSession()
        +lockClient()
        +unlockClient()
        +getOfflineUsers()
        +getActiveUdpAddresses()
    }
    class ThreadPool {
        -m_threads: vector < thread >
        -m_taskQueue: queue < function < void >>;
        -m_queueMutex: mutex;
        -m_queueCV: condition_variable;
        -m_running: atomic < bool >;
        -workerThread()
        +ThreadPool(numThreads)
        +enqueueTask(task)
        +stop()
    }
    class UdpHandler {
        -m_udpSocketFd: int;
        -handleKeepAlive(json request, client_addr)
        +UdpHandler(UdpSocketFd, Logger&, SessionManager&, TrafficReporter&, EventQueue&)
        +broadcastMessage(message)
        +handleMessage()
        +setSocketFd(fd)
        +sendMessageToClient( clientID, message, client_addr)
    }
    class IpcHandler {
        +IpcHanlder(Logger&, AlertManager&, TrafficReporter&)
        +handleConnection(fd)
    }
    class TrafficReporter {
        -m_message_counters: map;
        -m_error_counters: map;
        -m_reconnections_counters: map;
        +initPrometheusMetrics()
        +incrementMessage(protocol, dir)
        +startPrometheusExposer(port)
        +incrementError(protocol, direction)
        +incrementReconnection(protocol, direction)
    }
    class Config {
        -m_configNode: YAML:NODE;
        -validatePort(args)
        +Config(args)
        +logConfig()
        +serverConfig(args)
        +databaseConfig()
        +securityConfig()
        +getTcpPort()
        +getUdpPort()
        +getUnixSocketPath()
        +getMaxClients()
        +getMetricHostPort()
    }
    class EventQueue {
        -m_queueSize: int
        -m_eventQueueMap: unordered_map< string, dequeue < Event >>;
        +EventQueue(queueSize, Logger&)
        +enqueue(clientID, eventin)
        +dequeue(clientID, eventout)
    }
    class AlertManager {
        +alertManager(Logger&, SessionManager&, UdpHandler&)
        +processAlert(alertMessage)
    }

    %% Relationships (composition/association)
    Server "1" --> "N" ClientSession : manages
    Server "1" --> "1" Logger : uses
    Server "1" --> "1" ThreadPool : uses
    Server "1" --> "1" UdpHandler : uses
    Server "1" --> "1" IpcHandler : uses
    Server "1" --> "1" Config : uses

    clientSession --|> enable_shared_from_this : inherits
    ClientSession --> Inventory : uses
    ClientSession --> Authenticator : uses
    ClientSession --> Logger : uses
    ClientSession --> Storage : uses
    ClientSession --> SessionManager : uses
    ClientSession --> Config : uses
    ClientSession --> TrafficReporter : uses
    ClientSession --> EventQueue : uses
    ClientSession --> UdpHandler : uses
    ClientSession --> CommandProcessor : uses

    CommandProcessor --> Inventory : uses
    CommandProcessor --> Storage : uses
    CommandProcessor --> Logger : uses
    CommandProcessor --> SessionManager : uses
    CommandProcessor --> Config : uses
    CommandProcessor --> TrafficReporter : uses

    Authenticator --> Storage : uses
    Authenticator --> Logger : uses

    Inventory --> Storage : uses
    Inventory --> Logger : uses

    SessionManager --> Logger : uses
    SessionManager --> TrafficReporter : uses
    SessionManager --> Storage : uses

    UdpHandler --> Logger : uses
    UdpHandler --> SessionManager : uses
    UdpHandler --> TrafficReporter : uses
    UdpHandler --> EventQueue : uses

    IpcHandler --> Logger : uses
    IpcHandler --> AlertManager : uses
    IpcHandler --> TrafficReporter : uses

    AlertManager --> Logger : uses
    AlertManager --> SessionManager : uses
    AlertManager --> UdpHandler : uses

    EventQueue --> Logger : uses

    %% Showing only relevant methods for clarity  
```

## Class Diagram for Client

```mermaid
---
config:
  layout: dagre
---
classDiagram
    class client {
      <<module>>
     +client_cleanup(clientContext*)
     +socket_validation(addrinfo, clientContext*, sockfd, protocol)
     +setup_and_connect(clientContext*, client_config, protocol)
    }

    class client_context {
      +client_context_init(clientContext*)
      +client_context_set_id(clientContext*, client_id)
      +client_context_get_id(clientContext*)
    }

    class input_handler {
      <<module>>
      +build_json_from_input(raw_input): json_build_result
      +process_user_input(buffer): userInputAction
      +parse_argumments(argc, argv[], client_config)
      +build_json_for_command(command): json_build_result
      +config_arguments_ports(argc, argv[], client_config)
    }

    class ipc_handler {
      <<module>>
      +ipc_init(clientContext)
      +ipc_send_message(clientContext*, message)
      +priority_check(message)
      +ipc_exit(clientContext*)
    }

    class logger {
      <<module>>
      -log_mutex: pthread_mutex_t
      -*pFILE: FILE
      +logger_init(log_path)
      +logger_log(level, comp, message)
      +logger_close()
      +log_level_to_string(level)
    }

    class output_handler {
      <<module>>
      +print_readable_response(clientContext*, server_response, input_buffer, output_stream)
      +print_command_response(response_string, output_stream)
      +handle_login_repsonse(clientContext*, response_string, output_stream)
      +post_login_procedures(clientContext*, cJSON root, status, message, output_stream)
    }

    class session_handler {
      <<module>>
      -exit_requested: sig_atomic_t
      -handle_server_transaction(clientContext*, message, recv_fn, send_fn, input_buffer_copy): transaction_result
      -comunication_loop(clientContext*, recv_fn, send_fn, input_fn, buffer)
      +execute_client_action(clientContext*, userInputAction, buffer, recv_fn, send_fn): transaction_result
      +start_communication(clientContext*, recv_fn, send_fn, input_fn)
      +session_start_aux_threads(clientContext*)
      +launch_dashboard(clientContext*)
      +signal_handler(signum)
    }

    class transport {
      <<module>>
      -peer_addr: sockaddr_storage
      -peer_addr_len: peer_addr_len
      +tcp_send(sockfd, buf, size_t len, flags): ssize_t
      +tcp_recv(sockfd, buf, len, flags): ssize_t
      +udp_send(sockfd, buf, len, flags): ssize_t
      +udp_recv(sockfd, buf, len, flags): ssize_t
      +initialize_udp_peer_address(addr, len)
    }

    class udp_handler {
      <<module>>
      +*udp_listener_thread_func(*arg)
      +*keepalive_thread_func(*arg)
    }

    class ClientContext {
      <<struct>>
     -client_id: char
     -lock: pthread_mutex_t
     -tcp_socket: int
     -udp_socket: int 
     -ipc_queue: mqd_t
    }

    class transaction_result{
    <<enum>>
    TRANSACTION_SUCCESS
    TRANSACTION_SERVER_CLOSED
    TRANSACTION_ERROR
    TRANSACTION_CLOSE
    TRANSACTION_LOGIN_SUCCESS
    }

    class UserInputAction{
    <<enum>>
    INPUT_ACTION_SEND
    INPUT_ACTION_QUIT
    INPUT_ACTION_CONTINUE
    INPUT_ACTION_ERROR
    }

    class json_build_status{
    <<enum>>
    JSON_BUILD_SUCCESS
    JSON_BUILD_ERROR_SYNTAX
    JSON_BUILD_ERROR_MEMORY
    }

    class json_build_result{
        <<struct>>
        -json_string: char*
        -status: json_build_status
    }

    class client_config{
        <<struct>>
        -host: char*
        -port_tcp: char*
        -port_udp: char*
    }


    %% Relationships (one-to-one uses / ownership)
    client "1" --> "1" client_context : uses
    client "1" --> "1" transport : uses
    client "1" --> "1" ipc_handler : uses
    client "1" --> "1" input_handler : uses
    client "1" --> "1" logger : uses
    client --> transaction_result : contains

    client_context --> ClientContext : contains

    session_handler "1" --> "1" client_context : uses
    session_handler "1" --> "1" transport : uses
    session_handler "1" --> "1" ipc_handler : uses
    session_handler "1" --> "1" logger : uses
    session_handler --> output_handler : uses
    session_handler --> input_handler : uses
    session_handler --> udp_handler : uses
    session_handler --> client: uses

    udp_handler "1" --> "1" client_context : uses
    udp_handler --> transport : uses
    udp_handler --> logger : uses
    udp_handler --> ipc_handler: uses

    input_handler --> logger: uses
    input_handler --> UserInputAction : contains
    input_handler --> json_build_status : contains
    input_handler --> json_build_result : contains
    input_handler --> client_config : contains

    ipc_handler "1" --> "1" logger : uses
    ipc_handler --> client_context : uses

    output_handler --> client : uses
    output_handler --> logger : uses
    output_handler --> session_handler : uses
    output_handler --> client_context: uses
```

# 5. Requirements Coverage

| Req ID | Type   | Description (short) | Status | Notes |
|--------|--------|----------------------|:------:|-------|
| C001 | Constrain | Server in C++20 | ✅ | Constraint fulfilled. |
| C002 | Constrain | Client in C | ✅ | Client implemented in C. |
| C003 | Constrain | Use sockets for comms | ✅ | TCP/UDP sockets implemented. |
| C004 | Constrain | JSON message structure | ✅ | JSON used for TCP messages. |
| C005 | Constrain | Server broadcast emergency messages | ✅ | Alerts broadcast via UDP. |
| C006 | Constrain | Client ack reception system | ✅ | TCP replies/acks implemented. |
| C007 | Constrain | Keep-alive every minute | ✅ | UDP keepalive implemented (1 min). |
| C008 | Constrain | Graceful shutdown | ✅ | Signal handling + clean shutdown implemented. |
| C009 | Constrain | Follow FHS | ✅ | Project structured following FHS in workspace. |
| C010 | Constrain | Deployable as Linux service (Lab2) | N/A | TP2 |
| C011 | Constrain | Modular server (network/auth/etc) | ✅ | Clear modular separation implemented. |
| C012 | Constrain | Dynamic updates without recompilation | ❌ | Not implemented. |
| C013 | Constrain | Handle multiple simultaneous connections | ✅ | epoll + thread pool implemented. |
| C014 | FR | Secure client authentication | ✅ | bcrypt password hashing and auth implemented. |
| C015 | Constrain | IPC data sanitized | ✅ | Validation and try/catch used on IPC paths. |
| C016 | Constrain | Least privilege for clients | ✅ | Role-based permissions implemented. |
| C017 | Constrain | Server runs under dedicated user | ✅ | Deployment scripts / notes provided. |
| C018 | Constrain | Logging with timestamps/component/level | ✅ | Central Logger and DB log table implemented. |
| C019 | Constrain | At least one internal IPC mechanism | ✅ | POSIX MQ and UNIX file IPC used. |
| C020 | Constrain | Client uses shared libraries (.so) | ❌ | Not implemented. |
| C021 | Constrain | TCP/UDP over IPv4 & IPv6 | ✅ | getaddrinfo used for v4/v6. |
| C022 | Constrain | Inventory transaction history retrieval | ✅ | Storage supports history retrieval. |
| C023 | Constrain | Interactive CLI | ✅ | CLI implemented for client. |
| C024 | FR | Order cancellation within 30s | N/A | Not applicable for TP1. |
| C025 | Constrain | Designed for containerized execution | ✅ | Docker support included. |
| C026 | Constrain | Docker-based DB deployment | ⚠️ | Partial — SQLite file mapped to Docker volumes. |
| C027 | Constrain | Rate limit unsuccessful password attempts | ✅ | 3 attempts + 15 min lock implemented. |
| C028 | Constrain | Validate forwarded packages/format | ✅ | JSON validation at parsing stage implemented. |
| C029 | Constrain | Automatic backups + log rotation | ✅ | Log rotation + backup script provided. |
| C030 | Constrain | Server delayed events queue | ⚠️ | Partial — EventQueue exists conceptually; delayed events TBD. |
| C031 | Constrain | Ports configurable via env vars | ✅ | Ports configurable (env/args). |
| C032 | Constrain | Config file for queue sizes/max clients | ✅ | config.yaml validated and used. |
| C033 | Constrain | Scalable: add modules without restart | ❌ | Not implemented. |
| C034 | Constrain | Automatic client updates/rollback | ❌ | Not implemented. |
| C035 | Constrain | Clients cannot access other clients' data | ✅ | SessionManager enforces isolation. |
| C036 | Constrain | Lock clients triggered by alerts | ✅ | Alert types lock users; unlock via admin secret. |
| C037 | Constrain | Automatic traffic reports (counts/errors) | ✅ | prometheus-cpp metrics implemented. |
| FR001 | FR | Generate supply orders/prioritize shipments | N/A | Not in scope for TP1. |
| FR002 | FR | Process and broadcast alerts from sensors | ✅ | Implemented (IPC sensor → AlertManager → broadcast). |
| FR003 | FR | Client register/update inventory every 60s | ⚠️ | Partial — keepalive 60s present; periodic register/update partial. |
| FR004 | FR | Allow authorized operators to request resources | ✅ | CommandProcessor + permissions implemented. |
| FR005 | FR | Clients display emergency alerts in real time | ✅ | UDP notifications + dashboard display implemented. |
| FR007 | FR | Handle ≥10,000 concurrent connections | ⚠️ | Partial — epoll+threadpool designed for scale; full stress tests pending. |
| FR008 | FR | Store & retrieve inventory transaction history | ✅ | Storage provides history queries. |
| FR014 | FR | Log key transactions (orders/cancels/shipments) | ⚠️ | Partial — logging exists; specific order lifecycle logging partial. |
| NoFR001 | NoFR | Real-time health monitoring via Grafana | ✅ | Prometheus + Grafana integrated. |
| NoFR003 | NoFR | Structured error handling for failures | ✅ | Exception handling and validation implemented. |
| NoFR004 | NoFR | Uptime ≥ 99% | ⚠️ | Partial — design supports availability, not validated. |
| NoFR005 | NoFR | Containerized deployment flexibility | ✅ | Docker environment provided. |
| NoFR006 | NoFR | Max latency 20ms for message processing | ⚠️ | Partial — low-latency design, not tested under stress |
| NoFR007 | NoFR | Graceful failure recovery / persistence after crash | ⚠️ | Partial — backups and persistence exist, full recovery scenarios untested. |
| NoFR008 | NoFR | Implement log rotation to prevent disk usage | ✅ | Log rotation & compression implemented. |

- **Notes:**  Some functional (FR) and non-functional (NoFR) requirements that were not part of the original SRS were implemented later as a result of design decisions and best-practice improvements. Conversely, some constraints were not implemented due to initial misinterpretation; adding them at a late stage would have required large refactors of core modules, so they were deferred. Remaining or partially implemented FRs will be addressed in subsequent labs.

# 6. Testing & Validation

### CI / Automation
- A GitHub Actions pipeline runs on push to:
  - format code with clang-format,
  - build the project,
  - generate Doxygen documentation,
  - run the test suite.
- Test results and coverage reports are published in the CI artifacts and visible in the GitHub push details.

### Unit testing
- Unit tests are implemented with Unity.  
- A single test executable runs all tests for both server and client, grouping tests per module.
- Tests focus on module logic: command parsing, authentication logic, inventory operations, logger behavior, storage helpers, etc.
- Edge cases and failure scenarios are covered where meaningful.

### Coverage and scope
- Achieved approximate coverage: 70% (server + client).
- Network-dependent code (actual TCP/UDP socket interactions, address resolution, OS-level I/O) was not unit-tested. Those areas require mocks or full integration tests and were considered out of scope for unit testing in this lab.

### Integration and manual validation
- Features not easily unit-testable were validated with manual integration runs:
  - multiple clients connected to the server,
  - sending/receiving commands,
  - alert sensor and dashboard interaction,
  - verify logging, metrics and backups.
- These manual tests complement unit tests to validate end-to-end behavior.

### Static and dynamic analysis
- Static/dynamic tools integrated in CI and SonarQube:
  - clang-tidy, cppcheck, clang analyzer reports,
  - Valgrind for memory checks,
  - SonarQube for code quality dashboards.
- Reports are produced during CI and imported into Sonar for review.

### Limitations and next steps
- Missing automated integration tests for networking and end-to-end flows. Adding mocks for socket functions or a separate integration test stage is recommended.
- Coverage could increase by isolating and mocking OS/network dependencies and adding focused integration tests.
- Stress tests on multiple simultaneus clients are pending

# 7. Issues & Solutions

## 7.1 Resolved issues (summary)
- Concurrency/scalability: refactored from select + one-thread-per-connection to epoll + thread pool (default 16 threads). Result: much better resource usage and scalability.
- ClientSession redesign: centralized per-connection lifecycle, authentication and message processing moved into session object.
- Authentication: bcrypt password hashing and login lockout (3 attempts, 15 min) implemented.
- IPC integration: POSIX MQ (client → dashboard) and UNIX file (sensor → server) added and validated.
- Persistence: integrated SQLite via SQLiteCpp and implemented schema (users, inventory, logs).
- Logging & rotation: mutex-protected logging, size-based rotation, compression and backup script implemented.
- Monitoring: prometheus-cpp metrics and Grafana integration added (traffic/errors/connections).
- Error handling: try/catch validation across parsing, storage and command processing; config validation on startup.
- CI & quality: GitHub Actions for format/build/tests, clang-tidy, valgrind/cppcheck integration and SonarQube reporting.
- Unit testing: single test executable with Unity; ~70% coverage focusing on logic modules.
- Integrated the prometheuscpp library with Grafana (older versions of C prometheus not working)

## 7.2 Known issues
- The system does not support at the moment a logout function. The user must close the client to logout and re-open to login again. 
- The system does not check for duplicate sessions: two users can be loged in at the same time, and may cause unexpected behivour.
- At the moment there is no way to create a user in a fresh new server through the CLI. Admin user must be first created directly into the DB or within the code. Then a user admin can register new users one by one.

# 8. Conclusions

The first lab focused on building a solid foundation for the system, ensuring core infrastructure is in place and ready for future enhancements. The project was useful to learn and apply many aspects of software development: good GitHub practices, automated testing and CI, CMake, networking, monitoring with Prometheus/Grafana, database integration, and Linux system practices — all guided by software engineering best practices where possible.

Work took longer than initially planned because continuous improvements and robustness fixes were applied during development. Although the scope and complexity grew, an orderly approach and adherence to design principles made it possible to resolve most problems and meet the main requirements. With this base completed, the project is well positioned to implement additional features and address remaining items in the next labs.