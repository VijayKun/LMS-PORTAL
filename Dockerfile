# syntax=docker/dockerfile:1
#
# LMS backend - production image for Render (Docker runtime).

FROM debian:bookworm-slim AS build

RUN apt-get update \
 && apt-get install -y --no-install-recommends \
     build-essential libpq-dev libssl-dev ca-certificates \
 && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY backend/Makefile ./backend/
COPY backend/src ./backend/src/
COPY backend/lib ./backend/lib/
COPY backend/include ./backend/include/

RUN cd backend && make

FROM debian:bookworm-slim AS runtime

RUN apt-get update \
 && apt-get install -y --no-install-recommends \
     libpq5 libssl3 ca-certificates \
 && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY --from=build /app/backend/build/lms_backend /app/lms_backend

ENV LMS_SERVER_HOST=0.0.0.0
ENV LMS_SERVER_PORT=8080

EXPOSE 8080

CMD ["/app/lms_backend"]