# Socket-Based Course Registration System

A multi-process client–server application written in C++ that demonstrates layered network communication using both **TCP** and **UDP** sockets. A client authenticates with a central server, which delegates credential verification and course lookups to dedicated backend servers—all communicating over a local network.

---

## Architecture

```
                        ┌──────────┐
                        │  Client  │
                        └────┬─────┘
                          TCP│(port 25289)
                        ┌────▼─────┐
                        │ ServerM  │  ← Main / central server
                        └──┬───┬───┘
               UDP          │   │          UDP
        (port 21289) ┌──────┘   └──────┐ (port 22289 / 23289)
                  ┌──▼───┐        ┌────▼──────────┐
                  │ServerC│        │ ServerCS / EE  │
                  │ Auth  │        │ Course Lookup  │
                  └───────┘        └───────────────┘
```

| Component    | Protocol | Port  | Role                                                  |
|--------------|----------|-------|-------------------------------------------------------|
| `client`     | TCP      | —     | User-facing interface; handles credentials and queries |
| `serverM`    | TCP+UDP  | 25289 / 24289 | Central hub; routes all traffic              |
| `serverC`    | UDP      | 21289 | Validates encrypted credentials against `cred.txt`    |
| `serverCS`   | UDP      | 22289 | Answers CS department course queries from `cs.txt`    |
| `serverEE`   | UDP      | 23289 | Answers EE department course queries from `ee.txt`    |

---

## How It Works

### 1 — Authentication
1. The client connects to `serverM` over TCP and enters a **username** and **password**.
2. `serverM` applies a Caesar-style character shift to encrypt both fields.
3. The encrypted credentials are forwarded via UDP to `serverC`.
4. `serverC` looks up the credentials in `cred.txt` and replies with one of three tokens:

   | Token | Meaning                        |
   |-------|--------------------------------|
   | `TT`  | Username and password match    |
   | `T`   | Password matches, unknown user |
   | `F`   | No match found                 |

5. `serverM` maps the token to a client-facing result (`TT` / `FP` / `FU`) and sends it back over TCP.
6. The client has **up to 3 attempts** before being disconnected.

### 2 — Course Query
Once authenticated, the client enters a **course code** and **query category**:

- **Course code** prefix determines routing: `CS` → `serverCS`, `EE` → `serverEE`.
- **Category** options: `Credit`, `Professor`, `Days`, `CourseName`.
- `serverM` forwards the request via UDP to the appropriate department server, which looks up the data in its local `.txt` file and replies.
- `serverM` relays the result back to the client over TCP.
- This loop continues until the client disconnects.

---

## File Structure

```
.
├── client.cpp      # Client: user I/O, TCP connection to serverM
├── serverM.cpp     # Main server: TCP ↔ UDP bridge, credential encryption, query routing
├── serverC.cpp     # Auth server: UDP-only, validates credentials from cred.txt
├── serverCS.cpp    # CS dept server: UDP-only, course lookups from cs.txt
├── serverEE.cpp    # EE dept server: UDP-only, course lookups from ee.txt
├── cred.txt        # Credential database (username,password per line)
├── cs.txt          # CS course database (code,credit,professor,days,name per line)
└── ee.txt          # EE course database (code,credit,professor,days,name per line)
```

---

## Data File Format

**`cred.txt`** — one entry per line:
```
username,password
alice,pass123
bob,secure456
```

**`cs.txt` / `ee.txt`** — one course per line:
```
code,credit,professor,days,courseName
CS101,3,Dr. Smith,MWF,Intro to Programming
EE201,4,Dr. Jones,TTh,Circuit Analysis
```

---

## Prerequisites

- C++17-compatible compiler (`g++` recommended)
- POSIX-compliant OS (Linux or macOS)

---

## Build

```bash
g++ -std=c++17 -Wall -Wextra -o client    client.cpp
g++ -std=c++17 -Wall -Wextra -o serverM   serverM.cpp
g++ -std=c++17 -Wall -Wextra -o serverC   serverC.cpp
g++ -std=c++17 -Wall -Wextra -o serverCS  serverCS.cpp
g++ -std=c++17 -Wall -Wextra -o serverEE  serverEE.cpp
```

---

## Running

Open five terminal windows and start each process in this order (backend servers first):

```bash
# Terminal 1
./serverC

# Terminal 2
./serverCS

# Terminal 3
./serverEE

# Terminal 4
./serverM

# Terminal 5
./client
```

All processes communicate over `127.0.0.1` (localhost) using the ports listed in the architecture table above.

---

## Limitations

- All communication is over **localhost** — not intended for use across real networks.
- **Username and password input is capped at 50 characters.**
- Credential encryption is a simple Caesar-style character shift for illustrative purposes — it is **not cryptographically secure**.
- No TLS/SSL; do not use with real credentials.
- `serverM` handles **one client connection at a time** (no concurrency).

---

## References

- [Beej's Guide to Network Programming](https://beej.us/guide/bgnet/html/) — socket programming patterns used throughout this project.
- [Reading CSV files in C++](https://stackoverflow.com/questions/1474790/how-to-read-write-into-from-text-file-with-comma-separated-values) — approach used for parsing `.txt` database files.
