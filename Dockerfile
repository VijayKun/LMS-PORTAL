# Build stage for LMS Backend
FROM alpine:latest

RUN apk add --no-cache make gcc musl-dev postgresql-dev openssl-dev linux-headers

WORKDIR /app

COPY . .

RUN cd backend && make

EXPOSE 8080

CMD ["./backend/build/lms_backend.exe"]