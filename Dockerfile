FROM gcc:latest

WORKDIR /app

COPY . .

RUN apt-get update && apt-get install -y libpq-dev libssl-dev build-essential

RUN cd backend && make PQ_DIR=

EXPOSE 8080

CMD ["./backend/build/lms_backend"]