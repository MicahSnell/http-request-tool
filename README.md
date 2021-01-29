# HTTP Request Tool
## Description
This is a tool that sends one or more http requests to a user specified URL, and then prints the response that was received. The http request is handled using TCP sockets rather than an http library.

## How to Build
To build the program using the provided `makefile` simply navigate inside of the /src directory and type:
```bash
make
```
Which will create the executable `request`

## How to Run
### The Help Menu
To see detailed usage of the program use the `--help` parameter:
```bash
./request --help
```
Which will print a short description of the program and the parameters that it will accept.

### One Request w/ Response Output
To send an http GET request use the `--url` parameter followed by the URL of the server to send your request:
```bash
./request --url example.com
```
Which will output the number of bytes received, followed by the servers response.

### Multiple Requests w/ Statistics
To send multiple requests to a URL and track the statistics, add the
`--profile` parameter followed by a positive number:
```bash
./request --url example.com --profile 10
```
Which will send 10 GET requests to example.com/

## References
1. The linux man page for [getaddrinfo](https://man7.org/linux/man-pages/man3/getaddrinfo.3.html) 
2. The linux man page for [gettimeofday](https://man7.org/linux/man-pages/man2/gettimeofday.2.html)
