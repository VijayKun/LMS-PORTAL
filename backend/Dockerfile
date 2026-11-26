# syntax=docker/dockerfile:1
#
# LMS backend - production image for Render (Docker runtime).
#
# Stage 1 builds the C server with the same Makefile used locally; the OS
# detection there picks the Linux binary name and links -lpq/-lcrypto only.
# Stage 2 keeps just the runtime libraries, so the final image stays small.

FROM debian:bookworm-slim AS build

RUN apt-get update \
 && apt-get install -y --no-install-recommends \
      build-essential libpq-dev libssl-dev ca-certificates \
 && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . .
RUN make clean && make

FROM debian:bookworm-slim AS runtime

RUN apt-get update \
 && apt-get install -y --no-install-recommends \
      libpq5 libssl3 ca-certificates \
 && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY --from=build /app/build/lms_backend /app/lms_backend

# Render expects the service to listen on 0.0.0.0 and the port in $PORT.
# config.c reads PORT automatically; this just covers the bind address.
ENV LMS_SERVER_HOST=0.0.0.0

EXPOSE 8080

CMD ["/app/lms_backend"]
