# ESP8266 Virtual WiFi & NET C Library Architecture

**Source Files:**
- Core Virtual Module: [`src/Hardware/Comm/ESP8266/ESP8266.h`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Hardware/Comm/ESP8266/ESP8266.h) / [`src/Hardware/Comm/ESP8266/ESP8266.cpp`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Hardware/Comm/ESP8266/ESP8266.cpp)
- AT Command Parser: [`src/Hardware/Comm/ESP8266/ATCommandParser.h`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Hardware/Comm/ESP8266/ATCommandParser.h)
- AT Protocol Handler: [`src/Hardware/Comm/ESP8266/ATHandler.h`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Hardware/Comm/ESP8266/ATHandler.h) / [`src/Hardware/Comm/ESP8266/ATHandler.cpp`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Hardware/Comm/ESP8266/ATHandler.cpp)
- Asynchronous Engine: [`src/Hardware/Comm/ESP8266/NetworkLoop.h`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Hardware/Comm/ESP8266/NetworkLoop.h) / [`src/Hardware/Comm/ESP8266/NetworkLoop.cpp`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Hardware/Comm/ESP8266/NetworkLoop.cpp)
- Socket Manager: [`src/Hardware/Comm/ESP8266/ATSocketManager.h`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Hardware/Comm/ESP8266/ATSocketManager.h) / [`src/Hardware/Comm/ESP8266/ATSocketManager.cpp`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Hardware/Comm/ESP8266/ATSocketManager.cpp)
- HTTP/REST Client: [`src/Hardware/Comm/ESP8266/ATHttpClient.h`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Hardware/Comm/ESP8266/ATHttpClient.h) / [`src/Hardware/Comm/ESP8266/ATHttpClient.cpp`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Hardware/Comm/ESP8266/ATHttpClient.cpp)
- Ping Service: [`src/Hardware/Comm/ESP8266/ATPingService.h`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Hardware/Comm/ESP8266/ATPingService.h) / [`src/Hardware/Comm/ESP8266/ATPingService.cpp`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Hardware/Comm/ESP8266/ATPingService.cpp)
- Network Sniffer / Logger: [`src/Hardware/Comm/ESP8266/NetworkTrafficLogger.h`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Hardware/Comm/ESP8266/NetworkTrafficLogger.h)
- SDK Headers: [`sdk/src/Libs/NET.h`](file:///home/manu/Documents/GitHub/65c02-SIM/sdk/src/Libs/NET.h), [`sdk/microdos/Libs/app-bios.h`](file:///home/manu/Documents/GitHub/65c02-SIM/sdk/microdos/Libs/app-bios.h)
- Test Suite: [`src/UnitTests/ESP8266_Test.cpp`](file:///home/manu/Documents/GitHub/65c02-SIM/src/UnitTests/ESP8266_Test.cpp)

---

## 1. Overview

The 65c02-SIM platform includes an advanced virtual emulation of the **Espressif ESP8266 Wi-Fi SoC**, providing 65c02 applications with real internet connectivity, asynchronous TCP/UDP/SSL sockets, HTTP/REST requests, ICMP pings, and local server capabilities using industry-standard **Hayes AT commands**.

The virtual modem is decoupled from the 6502 clock domain via a multithreaded networking core powered by **Asio** and **OpenSSL**, buffering incoming data packets in an internal FIFO queue and feeding bytes to the 65c02 through a dedicated Memory-Mapped I/O (MMIO) register interface.

```
       65c02 CPU Bus
             │
      [ MMIO: 0x5004 / 0x7FC4 ]
             │
    +--------v--------+       Worker Threads       +----------------------+
    |  ESP8266 FIFO   |<───────────────────────────|   ATSocketManager    |
    |  RX/TX Queues   |                            |   (Asio & OpenSSL)   |
    +--------+--------+                            +----------+-----------+
             │                                                │
    +--------v--------+                                       v
    |    ATHandler    |────────────────────────────> Real Internet (TCP/UDP/SSL)
    | Command Parser  |
    +-----------------+
```

---

## 2. MMIO Interface & Registers

The ESP8266 module occupies a 4-byte address block in the system address space:

| Default Address | microDOS / Cartridge Address | Register | Access | Hardware Functionality |
|:---:|:---:|:---|:---:|:---|
| **`0x5004`** | **`0x7FC4`** | `ESP8266_DATA` | R/W | **Read:** Dequeues one byte from the RX FIFO queue. When the queue is emptied, Bit 7 of `STATUS` is automatically cleared.<br>**Write:** Transmits one byte to the AT command buffer or raw payload stream. |
| **`0x5005`** | **`0x7FC5`** | `ESP8266_STATUS` | R | **Status Register:**<br>• **Bit 7 (`0x80`):** `1` = Data available in RX queue; `0` = RX queue empty.<br>• Bits 0–6: Reserved. |
| **`0x5006`** | **`0x7FC6`** | `ESP8266_CTRL` | R/W | **Control Register:**<br>• **Read:** Returns current control register state (`ctrlReg`).<br>• **Write:** Bit 0 (`data & 0x01 != 0`) triggers an immediate hardware reset of the module and clears all socket FIFO queues (`Reset()`). |
| `0x5007` | `0x7FC7` | *Reserved* | R | Returns `0x00`. Writes are ignored. |

### 2.1 Direct MMIO vs. ACIA Serial Bridge Modes

The emulator supports two distinct connection topologies between the 65c02 CPU and the virtual ESP8266:

1. **Direct MMIO Mode (Default & microDOS):**
   - Software reads and writes directly to `ESP8266_DATA` (`0x5004` or `0x7FC4`) and polls `ESP8266_STATUS` (`0x5005` or `0x7FC5`).
   - Offers near-instantaneous throughput without artificial baud rate throttling, ideal for network file transfers and HTTP downloads in `NET.h`.
2. **ACIA Serial UART Bridge Mode (`SerialIOBridge`):**
   - In legacy or retro-modem configurations, the 6502 communicates with the ESP8266 using standard MOS 6551 ACIA registers (`0x5000` / `0x7FC0`).
   - `SerialIOBridge::ForwardOutputIfBridged()` intercepts bytes transmitted over the ACIA serial line, logs them in `NetworkTrafficLogger`, and pipes them into the ESP8266 at `0x5004`.
   - Incoming responses from the ESP8266 are shifted back into the ACIA receiver buffer paced by the configured baud rate delay (`baudDelay = (ips * 10) / baudRate`).

---

## 3. Supported AT Command Set

All AT commands are sent as ASCII strings terminated with `\r\n` (CRLF) or `\n`. The module supports case-insensitive commands, optional echo mode (`ATE0`/`ATE1`), and standard response status codes (`OK`, `ERROR`, `FAIL`).

### 3.1 Device Management & Configuration

| Command | Parameters | Response | Description |
|:---|:---|:---|:---|
| **`AT`** | — | `\r\nOK\r\n` | Tests AT communication; verifies modem is responsive. |
| **`AT+RST`** | — | `\r\nOK\r\n` | Performs a software reset of all network state machines and socket queues. |
| **`AT+GMR`** | — | Firmware Info + `\r\nOK\r\n` | Returns simulated AT version and SDK firmware build revision. |

---

### 3.2 Wi-Fi Subsystem (`VirtualWifiState`)

| Command | Parameters | Response | Description |
|:---|:---|:---|:---|
| **`AT+CWMODE=<mode>`** | `1` = Station<br>`2` = SoftAP<br>`3` = Station+SoftAP | `\r\nOK\r\n` | Configures the virtual Wi-Fi operating mode. |
| **`AT+CWMODE?`** | — | `+CWMODE:<mode>` | Queries the current Wi-Fi operating mode. |
| **`AT+CWJAP="ssid","pwd"`**| `ssid`, `password` | `WIFI CONNECTED\r\nWIFI GOT IP\r\n\r\nOK\r\n` | Connects the virtual station to a Wi-Fi access point. |
| **`AT+CWJAP?`** | — | `+CWJAP:"<ssid>"` | Queries the currently connected SSID. |
| **`AT+CWLAP`** | — | List of APs + `\r\nOK\r\n` | Scans and lists nearby virtual and simulated access points. |
| **`AT+CWQAP`** | — | `\r\nOK\r\n` | Disconnects the station from the access point. |
| **`AT+CIFSR`** | — | IP addresses + `\r\nOK\r\n` | Returns local Station and SoftAP IPv4 addresses (`+CIFSR:STAIP,"192.168.1.100"`). |

---

### 3.3 TCP/IP & Socket Management (`ATSocketManager`)

The socket engine manages up to **5 concurrent sockets** (`kMaxConnections = 5`, Link IDs $0 \dots 4$).

#### `AT+CIPMUX` — Connection Mode
- `AT+CIPMUX=0`: Single connection mode (default).
- `AT+CIPMUX=1`: Multiple connection mode (required for TCP server).

#### `AT+CIPSTART` — Establishing Sockets
- **Single Connection (`CIPMUX=0`):**
  - TCP: `AT+CIPSTART="TCP","hostname",port`
  - UDP: `AT+CIPSTART="UDP","hostname",port[,local_port,mode]`
  - SSL/TLS: `AT+CIPSTART="SSL","hostname",port`
- **Multiple Connection (`CIPMUX=1`):**
  - TCP: `AT+CIPSTART=<id>,"TCP","hostname",port`
  - UDP: `AT+CIPSTART=<id>,"UDP","hostname",port[,local_port,mode]`
  - SSL/TLS: `AT+CIPSTART=<id>,"SSL","hostname",port`

#### `AT+CIPSEND` — Data Transmission State Machine
1. Software transmits:
   - Single mode: `AT+CIPSEND=<length>`
   - Multi mode: `AT+CIPSEND=<id>,<length>`
2. The module responds with the `>` prompt and enters `ATState::WaitingCIPSENDData`.
3. Software writes exactly `<length>` raw bytes into `ESP8266_DATA`.
4. As soon as all bytes are received, the engine delivers the payload to the socket worker thread and responds with:
   ```
   \r\nSEND OK\r\n
   ```

#### `+IPD` — Unsolicited Incoming Data
When incoming network traffic arrives from a remote server, the worker thread enqueues an unsolicited `+IPD` header directly into the RX queue:
- **Single Mode:** `+IPD,<len>:<data>`
- **Multi Mode:** `+IPD,<id>,<len>:<data>`

#### `AT+CIPCLOSE` — Terminating Connections
- `AT+CIPCLOSE` (Single Mode) or `AT+CIPCLOSE=<id>` (Multi Mode).
- When a remote host terminates a connection, the module enqueues:
  - Single Mode: `CLOSED\r\n`
  - Multi Mode: `<id>,CLOSED\r\n`

#### `AT+CIPSERVER` — TCP Server
- `AT+CIPSERVER=1,<port>`: Starts an asynchronous listening TCP server on the specified port.
- Incoming client connections are automatically assigned the next available Link ID ($0 \dots 4$).
- `AT+CIPSERVER=0`: Shuts down the listening server.

---

### 3.4 High-Level Services: HTTP Client & ICMP Ping

#### `AT+HTTPCLIENT` — REST / Web Request Client
[`ATHttpClient`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Hardware/Comm/ESP8266/ATHttpClient.h) performs complete asynchronous HTTP and HTTPS transfers without requiring manual socket and header parsing in 6502 code:

$$\text{AT+HTTPCLIENT}=\text{opt},\text{contentType},"\text{url}"[,"\text{host}","\text{path}","\text{postData}"]$$

- **`opt` (Method):** `1` = GET, `2` = POST, `3` = PUT, `4` = DELETE.
- **`contentType`:** `0` = `application/x-www-form-urlencoded`, `1` = `text/plain`, `2` = `application/json`.
- The engine executes the request on a background thread and streams the HTTP response body back through `+IPD` packets!

#### `AT+PING` — Network Ping Service
[`ATPingService`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Hardware/Comm/ESP8266/ATPingService.h) resolves hostnames and verifies remote connectivity:
```
AT+PING="8.8.8.8"
+PING:14
OK
```

---

## 4. Network Traffic Sniffer (`NetworkTrafficLogger`)

The virtual ESP8266 integrates a non-intrusive traffic logger:
- **Channels:** `ATCommand`, `ATResponse`, `DataPayload`, `TLS_HTTP`, `System`.
- **Packet Attributes:** Timestamp (ms), direction (`TX`, `RX`, `NetTX`, `NetRX`), link ID, and payload.
- Software developers can inspect raw network packets in real time via the **Network Sniffer Window** in the GUI frontend.

---

## 5. The `NET.h` C Library Reference

The SDK header [`sdk/src/Libs/NET.h`](file:///home/manu/Documents/GitHub/65c02-SIM/sdk/src/Libs/NET.h) provides lightweight macros and C bindings:

### 5.1 Low-Level MMIO Macros

```c
#if defined(MICRODOS) || defined(__MICRODOS__)
#define ESP8266_DATA   (*(volatile uint8_t*)0x7FC4)
#define ESP8266_STATUS (*(volatile uint8_t*)0x7FC5)
#else
#define ESP8266_DATA   (*(volatile uint8_t*)0x5004)
#define ESP8266_STATUS (*(volatile uint8_t*)0x5005)
#endif

// Returns true if unread data is waiting in the RX buffer
static inline bool net_has_data(void) {
    return (ESP8266_STATUS & 0x80U) != 0U;
}

// Writes a single byte to the ESP8266
static inline void net_putc(uint8_t c) {
    ESP8266_DATA = c;
}

// Reads a single byte from the ESP8266
static inline uint8_t net_getc(void) {
    return ESP8266_DATA;
}
```

### 5.2 High-Level API Functions

- `void net_send(const char* s)`: Transmits a null-terminated string character by character.
- `void net_send_num(uint16_t n)`: Formats and transmits an unsigned integer as an ASCII decimal string.
- `void net_cmd(const char* s)`: Sends an AT command string, automatically appending `\r\n`.
- `void net_wifi(const char* ssid, const char* pass)`: Sets `CWMODE=1` and connects to an AP using `AT+CWJAP`.
- `void net_start(const char* type, const char* host, const char* port)`: Issues `AT+CIPSTART`.
- `void net_cipsend(const char* data, uint16_t len)`: Sends `AT+CIPSEND`, waits for the `>` prompt, and transmits `len` raw bytes.

---

## 6. Programming Examples

### 6.1 65c02 Assembly: Sending an AT Command and Reading Response

```assembly
; =====================================================================
; Send AT Command and Wait for 'OK'
; MMIO Base: $5004 (Default) or $7FC4 (microDOS)
; =====================================================================
ESP_DATA    = $5004
ESP_STATUS  = $5005

SendAT:
    ; Send 'A', 'T', '\r', '\n'
    LDA #'A'
    STA ESP_DATA
    LDA #'T'
    STA ESP_DATA
    LDA #$0D
    STA ESP_DATA
    LDA #$0A
    STA ESP_DATA

    ; Read back response
WaitResponse:
    LDA ESP_STATUS
    AND #$80            ; Check Bit 7 (Data Ready)
    BEQ WaitResponse
    LDA ESP_DATA        ; Read byte from RX queue
    ; Process byte in Accumulator...
    RTS
```

### 6.2 C Language: Performing an HTTP GET Request

```c
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "Libs/NET.h"
#include "Libs/app-bios.h"

void http_get_example(void) {
    char c;
    println("Initializing WiFi...");

    // 1. Join Access Point
    net_wifi("SimulatedNetwork", "password123");

    // 2. Open TCP connection to HTTP server
    println("Connecting to example.com:80...");
    net_start("TCP", "example.com", "80");

    // 3. Prepare HTTP GET payload
    const char* request = "GET / HTTP/1.1\r\nHost: example.com\r\nConnection: close\r\n\r\n";
    uint16_t req_len = strlen(request);

    // 4. Send request via CIPSEND
    net_cipsend(request, req_len);
    println("Request sent. Awaiting response:\n");

    // 5. Stream response bytes to console
    while (true) {
        if (net_has_data()) {
            c = (char)net_getc();
            bios_putchar(c);
        }
    }
}
```
