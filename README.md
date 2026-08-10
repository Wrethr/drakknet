# drakknet_c
 
A cross-platform C networking library — a thin, unified layer over BSD sockets (POSIX) and Winsock2 (Windows).
 
`drakknet_c` is the low-level C ABI foundation of the larger **drakknet** project. It wraps the raw socket APIs of Linux and Windows behind a single, consistent interface, so the rest of the library (and any application built on top of it) doesn't need to care which platform it's running on.
 
## Why
 
Writing cross-platform networking code in raw C means dealing with two different worlds at once: POSIX sockets on Linux/macOS, and Winsock2 on Windows. The types don't match (`int` vs `SOCKET`), the error codes don't match (`errno` vs `WSAGetLastError`), even the "invalid socket" and "socket error" sentinels are different values. `drakknet_c` hides all of that behind one API, so you write your networking code once.
 
## Features
 
- **Cross-platform**: one API, works on Linux (POSIX sockets) and Windows (Winsock2).
- **IPv4 and IPv6**: addresses are stored in a `sockaddr_storage`-based type that transparently supports both.
- **Unified error handling**: every function returns a `drakknet_error_t` enum instead of raw, platform-specific error codes.
- **Zero-overhead style**: no exceptions, no hidden allocations, error codes returned directly, results passed back through out-parameters.

## Roadmap
 
1. **Event loop** — `poll`/`select` (cross-platform), with `epoll` as a Linux-specific fast path.
2. **HTTP layer** — a request/response parser built on top of the TCP layer, turning `drakknet_c` into the foundation for a small HTTP server.
