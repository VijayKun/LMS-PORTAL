FROM buildpack-deps:bullseye AS builder

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential libpq-dev libssl-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY backend/Makefile backend/
COPY backend/src backend/
COPY backend/lib backend/

RUN make clean && make

FROM buildpack-deps:bullseye-slim AS runtime

RUN apt-get update && apt-get install -y --no-install-recommends \
    libpq5 libssl3 \
    && rm -rf /var/lib/apt/lists/*

COPY --from=builder /app/backend/build/lms_backend /app/lms_backend

ENV LMS_SERVER_HOST=0.0.0.0
ENV LMS_SERVER_PORT=8080

EXPOSE 8080

CMD ["/app/lms_backend"]