# Vendored third-party code

## Mongoose (HTTP server)

- Files: `mongoose.c`, `mongoose.h`
- Version: **7.23**
- Source: https://github.com/cesanta/mongoose
- License: **GPLv2** (Mongoose also offers a commercial license).

Mongoose is a single-file (amalgamated) embedded web server for C. It is
compiled directly into the backend from this folder - do not edit these files.

### Updating

Download a new `mongoose.c` / `mongoose.h` pair from the Mongoose releases page
and replace both files, then run `make clean && make`.

### Link requirements (Windows/MinGW)

Mongoose needs Winsock, hence `-lws2_32` is set in the project `Makefile`.
