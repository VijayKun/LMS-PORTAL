FROM buildpack-deps:bullseye

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    libpq-dev \
    libssl-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY backend/Makefile backend/
COPY backend/src backend/src
COPY backend/lib backend/lib

RUN cd backend && make

ENV LMS_SERVER_HOST=0.0.0.0
ENV LMS_SERVER_PORT=8080

EXPOSE 8080

CMD ["/app/backend/build/lms_backend"]