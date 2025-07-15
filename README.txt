a.Name: Mahtab Movahhedrad


c. In this project, the client prompts the user for a username and password, then sends them to ServerM over a TCP connection. ServerM encrypts the credentials and forwards them via UDP to ServerC, which checks its database and returns a confirmation message if they match. The client has up to three attempts to enter the correct username and password. Once authenticated, ServerM notifies the client, then asks for a course code and query category. The client sends this information over TCP to ServerM, which forwards the request via UDP to either ServerCS or ServerEE based on the course code. The department server looks up the requested data in its database and replies via UDP to ServerM, which then relays the response back to the client over TCP and waits for the next query.
d. (1)client.cpp: connects to the serverM via a TCP port. It inputs username and password and waits for auth response. Then after receiving the conformation, it enters the coursecode and query category and server main will respod to that via the TCP port. 


1. **Client**  
   - Connects to `ServerM` over TCP.  
   - Prompts user for **username** and **password**.  
   - Sends credentials to `ServerM`.  
   - After successful authentication, prompts for **course code** and **query category** and sends them to `ServerM`.  
   - Receives query results over TCP and displays them.  

2. **ServerM** (Main Server)  
   - Listens for TCP connections from the client.  
   - On credential receipt: encrypts them and forwards via UDP to `ServerC`.  
   - Waits for authentication confirmation, then notifies the client.  
   - On course‐query receipt: chooses the correct department server (`ServerCS` or `ServerEE`) based on course code, forwards the request via UDP, collects the response, and relays it back to the client over TCP.  

3. **ServerC** (Authentication Server)  
   - Listens for UDP messages from `ServerM`.  
   - Compares received (encrypted) credentials against its database.  
   - Sends back a confirmation (success/failure) via UDP to `ServerM`.  

4. **ServerCS** (CS Department Server)  
   - Listens for UDP queries from `ServerM`.  
   - Looks up CS‐related course data in its local database and replies via UDP.  

5. **ServerEE** (EE Department Server)  
   - Listens for UDP queries from `ServerM`.  
   - Looks up EE‐related course data in its local database and replies via UDP.  

## Components

| File           | Description                                                                                     |
| -------------- | ----------------------------------------------------------------------------------------------- |
| **client.cpp** | Connects to `ServerM` (TCP), handles user I/O (credentials & queries), and displays server replies. |
| **serverM.cpp**| TCP interface to clients; encrypts credentials, communicates with `ServerC`, `ServerCS`, `ServerEE` via UDP, and relays responses. |
| **serverC.cpp**| UDP‐only authentication backend; validates encrypted credentials against its database.           |
| **serverCS.cpp**| UDP‐only CS department backend; responds to CS course queries.                                 |
| **serverEE.cpp**| UDP‐only EE department backend; responds to EE course queries.                                 |

## Data Formats

- **All inter‐server messages** are exchanged as plain **strings**.  
- Only when reading from text files (e.g., loading databases) are C++ `std::vector` structures used internally.

## Limitations

- **Username input** is limited to **50 characters**. Any input longer than this will cause the program to fail.
- No SSL/TLS—credentials encryption is custom/illustrative only.
- No persistent connections for department servers; each request is one UDP datagram.

## Building & Running

1. **Prerequisites**  
   - C++11 (or later) compiler  
   - POSIX‐compliant OS (Linux/macOS)  
   - [Beej’s Guide to Network Programming](https://beej.us/guide/bgnet/html/)  

2. **Compile**  
   ```bash
   g++ -std=c++11 -o client client.cpp
   g++ -std=c++11 -o serverM serverM.cpp
   g++ -std=c++11 -o serverC serverC.cpp
   g++ -std=c++11 -o serverCS serverCS.cpp
   g++ -std=c++11 -o serverEE serverEE.cpp


h. I used https://beej.us/guide/bgnet/html/ for socket programming and https://stackoverflow.com/questions/1474790/how-to-read-write-into-from-text-file-with-comma-separated-values for reading the txt files