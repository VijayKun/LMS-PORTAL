FROM gcc:latest

WORKDIR /app

COPY . .

RUN apt-get update && apt-get install -y libpq-dev libssl-dev

RUN cd backend && make

EXPOSE 8080

CMD ["./backend/build/lms_backend.exe"]