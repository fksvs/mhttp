MHTTP - Minimal HTTPS web server
================================

mhttp is a epoll based minimal HTTPS web server.

## Table of Contents

1. [Features](#features)
2. [Requirements](#requirements)
3. [Installation](#installation)
4. [Usage](#usage)
5. [Caveats](#caveats)
6. [License](#license)

## Features

- Support for both IPv4 and IPv6.
- Multi-client architecture.
- GET and HEAD methods.
- Detailed error codes.
- Custom logging.

## Requirements

- OpenSSL

## Installation

- Clone the repository from [GitHub][] or [GitLab][]:

```console
git clone https://github.com/fksvs/mhttp
git clone https://gitlab.com/fksvs/mhttp
```
- navigate to the `mhttp` directory and build the source code:

```console
cd mhttp/
make
```

- to enable debug features (console logging), build with `debug` flag:

```console
make debug
```

## Usage

Run `mhttp` with the following command:

```console
./mhttp [options]
```

`mhttp` will then start as daemon.

The server will listen on addresses specified as macro in the `src/types.h` file. Make sure to configure these macros before compiling or use `-a`, `-p` and `-6` options according to your requirements.

To enable TLS, generate a certificate and key file. Default file names are `cert.pem` and `key.pem`

#### Options:

- -a [listen address] : listen address for incoming connection
- -p [listen port] : listen port for incoming connections
- -6 : use IPv6
- -d [directory] : main directory to serve
- -s : use TLS
- -c [certificate file] : TLS certificate file
- -k [key file] : TLS private key file
- -f [log file] : set log file
- -h : display this help

## Caveats

I developed this web server for fun, many lines of code in this project is probably vulnerable.

## License

This project is free software; you can redistribute it and/or modify it under the terms of the GPLv3 license. See [LICENSE][] for details.

[GitHub]: https://github.com/fksvs/mhttp
[GitLab]: https://gitlab.com/fksvs/mhttp
[LICENSE]: https://www.gnu.org/licenses/gpl-3.0.en.html
