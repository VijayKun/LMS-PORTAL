FROM buildpack-deps:bullseye

RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
    build-essential libpq-dev libssl-dev \
    && update-ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY backend/Makefile backend/
COPY backend/src backend/
COPY backend/lib backend/

RUN make -C backend

# Debug - verify binary
RUN file backend/build/lms_backend

ENV LMS_SERVER_HOST=0.0.0.0
ENV LMS_SERVER_PORT=8080

EXPOSE 8080

CMD ["/app/backend/build/lms_backend"]