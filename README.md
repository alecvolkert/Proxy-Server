
# Caching HTTP Proxy Server

A lightweight HTTP proxy server written in C that sits between a client and the web, caching responses to speed up repeated requests and filtering out blocked domains. Built as a systems programming project at CU Boulder — one of my favorite things I've built. The website filtering in particular was satisfying to get working.

---

## How It Works

- Intercepts HTTP GET requests from the client
- Checks if the requested URL is in the **blocklist** — if so, returns a 403 and drops the connection
- Checks if the response is already **cached** locally via MD5-hashed URL — if so, serves it directly without hitting the network
- If not cached, forwards the request to the target server, streams the response back to the client, and **writes it to cache** for future requests
- Handles multiple simultaneous clients via **fork()**

---

## Features

- **MD5-based caching** — each URL is hashed to a unique filename for fast cache lookup
- **Domain/IP blocklist** — blocks requests to specified hosts, returns HTTP 403
- **Cookie stripping** — removes Cookie headers from forwarded requests
- **User-Agent spoofing** — replaces the client's User-Agent with `Proxy/1.0`
- **Concurrent clients** — fork-based multi-client handling
- **Configurable timeout** — optional cache expiry via command line argument

---

## Files

| File | Description |
|------|-------------|
| `proxy.c` | Main proxy server implementation |
| `cache/` | Directory where cached responses are stored |
| `blocklist` | Plaintext list of blocked domains/IPs |

---

## Build

```bash
gcc proxy.c -o proxy -lssl -lcrypto
```

---

## Usage

```bash
./proxy <port>
./proxy <port> <timeout>
```

**Examples:**
```bash
./proxy 8080        # Run on port 8080, no cache timeout
./proxy 8080 60     # Run with 60 second cache timeout
```

Then configure your browser or HTTP client to use `127.0.0.1:<port>` as a proxy.

---

## Blocklist

Add domains or IPs to the `blocklist` file, one per line:

```
facebook.com
ads.doubleclick.net
192.168.1.100
```

Any request to a listed host returns `HTTP/1.1 403 Forbidden` and closes the connection.

---

## Technical Details

- Written in **C** using POSIX sockets (TCP)
- MD5 hashing via **OpenSSL** (`libssl`, `libcrypto`)
- Cache stored as flat files named by URL hash
- Parses raw HTTP headers manually — no external HTTP libraries
- Supports HTTP/1.0 and HTTP/1.1
