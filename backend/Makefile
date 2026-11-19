# LMS backend - build system
#
# Targets:
#   make        build the server        -> build/lms_backend.exe
#   make run    build and run the server
#   make test   build and run the unit tests
#   make clean  remove build artifacts
#   make help   list targets
#
# Requires: MSYS2 UCRT64 toolchain (gcc, make) and libpq.
#   pacman -S --needed make mingw-w64-ucrt-x86_64-postgresql
#
# At runtime the libpq/OpenSSL DLLs must be on PATH, e.g.:
#   $env:Path += ";C:\msys64\ucrt64\bin"

CC       ?= gcc
BIN      := build/lms_backend.exe

SRCDIR := src
LIBDIR := lib

# libpq (PostgreSQL client) from the MSYS2 UCRT64 prefix. Override if needed:
#   make PQ_DIR=/c/other/prefix
PQ_DIR ?= C:/msys64/ucrt64

CPPFLAGS += -I$(SRCDIR) -I$(LIBDIR) -I$(PQ_DIR)/include
CFLAGS   ?= -std=c11 -Wall -Wextra -O2 -g
CFLAGS   += -MMD -MP
LDFLAGS  += -L$(PQ_DIR)/lib
             LDLIBS   += -lpq -lcrypto -lws2_32 -lwinhttp

SRCS := \
  $(SRCDIR)/main.c \
  $(SRCDIR)/config/config.c \
  $(SRCDIR)/database/db.c \
  $(SRCDIR)/server/server.c \
  $(SRCDIR)/routes/health_routes.c \
  $(SRCDIR)/routes/auth_routes.c \
  $(SRCDIR)/routes/book_routes.c \
  $(SRCDIR)/routes/student_routes.c \
  $(SRCDIR)/routes/loan_routes.c \
  $(SRCDIR)/routes/reservation_routes.c \
  $(SRCDIR)/routes/fine_routes.c \
  $(SRCDIR)/routes/notification_routes.c \
  $(SRCDIR)/routes/report_routes.c \
  $(SRCDIR)/routes/branch_routes.c \
  $(SRCDIR)/controllers/auth_controller.c \
  $(SRCDIR)/controllers/book_controller.c \
  $(SRCDIR)/controllers/student_controller.c \
  $(SRCDIR)/controllers/loan_controller.c \
  $(SRCDIR)/controllers/reservation_controller.c \
  $(SRCDIR)/controllers/fine_controller.c \
  $(SRCDIR)/controllers/notification_controller.c \
  $(SRCDIR)/controllers/report_controller.c \
  $(SRCDIR)/controllers/branch_controller.c \
  $(SRCDIR)/services/list_service.c \
  $(SRCDIR)/services/auth_service.c \
  $(SRCDIR)/services/book_service.c \
  $(SRCDIR)/services/student_service.c \
  $(SRCDIR)/services/loan_service.c \
  $(SRCDIR)/services/reservation_service.c \
  $(SRCDIR)/services/fine_service.c \
  $(SRCDIR)/services/notification_service.c \
  $(SRCDIR)/services/report_service.c \
  $(SRCDIR)/services/branch_service.c \
  $(SRCDIR)/services/isbn_service.c \
  $(SRCDIR)/auth/session_store.c \
  $(SRCDIR)/middleware/auth_middleware.c \
  $(SRCDIR)/middleware/cors_middleware.c \
  $(SRCDIR)/security/password.c \
  $(SRCDIR)/utils/json_utils.c \
  $(SRCDIR)/utils/json_parse.c \
  $(SRCDIR)/utils/response_utils.c \
  $(LIBDIR)/mongoose.c

OBJS := $(patsubst %.c,build/obj/%.o,$(SRCS))
DEPS := $(OBJS:.o=.d)

# Unit tests (no live database required).
#   * test_json_utils / test_books_json link only json_utils.c.
#   * test_book_service links the service + db layer (and therefore libpq) but
#     only exercises the "no database" paths, so it needs no credentials.
TEST_SRCS := tests/test_json_utils.c tests/test_books_json.c tests/test_book_service.c \
  tests/test_student_service.c tests/test_list_service.c tests/test_session_store.c \
  tests/test_json_parse.c tests/test_password.c tests/test_auth_service.c \
  tests/test_loan_service.c tests/test_reservation_service.c tests/test_fine_service.c \
  tests/test_report_service.c tests/test_isbn_service.c
TEST_BINS := $(patsubst tests/%.c,build/%.exe,$(TEST_SRCS))

.PHONY: all run test clean help

all: $(BIN)

$(BIN): $(OBJS)
	@mkdir -p $(dir $@)
	$(CC) $(OBJS) -o $@ $(LDFLAGS) $(LDLIBS)

build/obj/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

build/test_json_utils.exe build/test_books_json.exe: build/%.exe: tests/%.c $(SRCDIR)/utils/json_utils.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ -o $@

build/test_book_service.exe: tests/test_book_service.c \
  $(SRCDIR)/services/book_service.c \
  $(SRCDIR)/database/db.c \
  $(SRCDIR)/utils/json_utils.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ -o $@ $(LDFLAGS) $(LDLIBS)

build/test_student_service.exe: tests/test_student_service.c \
  $(SRCDIR)/services/student_service.c \
  $(SRCDIR)/security/password.c \
  $(SRCDIR)/database/db.c \
  $(SRCDIR)/utils/json_utils.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ -o $@ $(LDFLAGS) $(LDLIBS)

build/test_list_service.exe: tests/test_list_service.c \
  $(SRCDIR)/services/list_service.c \
  $(SRCDIR)/database/db.c \
  $(SRCDIR)/utils/json_utils.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ -o $@ $(LDFLAGS) $(LDLIBS)

build/test_session_store.exe: tests/test_session_store.c \
  $(SRCDIR)/auth/session_store.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ -o $@

build/test_json_parse.exe: tests/test_json_parse.c \
  $(SRCDIR)/utils/json_parse.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ -o $@

build/test_password.exe: tests/test_password.c \
  $(SRCDIR)/security/password.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ -o $@ $(LDFLAGS) -lcrypto

build/test_auth_service.exe: tests/test_auth_service.c \
  $(SRCDIR)/services/auth_service.c \
  $(SRCDIR)/database/db.c \
  $(SRCDIR)/security/password.c \
  $(SRCDIR)/utils/json_utils.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ -o $@ $(LDFLAGS) $(LDLIBS)

build/test_loan_service.exe: tests/test_loan_service.c \
  $(SRCDIR)/services/loan_service.c \
  $(SRCDIR)/services/list_service.c \
  $(SRCDIR)/database/db.c \
  $(SRCDIR)/utils/json_utils.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ -o $@ $(LDFLAGS) $(LDLIBS)

build/test_reservation_service.exe: tests/test_reservation_service.c \
  $(SRCDIR)/services/reservation_service.c \
  $(SRCDIR)/services/list_service.c \
  $(SRCDIR)/database/db.c \
  $(SRCDIR)/utils/json_utils.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ -o $@ $(LDFLAGS) $(LDLIBS)

build/test_fine_service.exe: tests/test_fine_service.c \
  $(SRCDIR)/services/fine_service.c \
  $(SRCDIR)/services/list_service.c \
  $(SRCDIR)/database/db.c \
  $(SRCDIR)/utils/json_utils.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ -o $@ $(LDFLAGS) $(LDLIBS)

build/test_report_service.exe: tests/test_report_service.c \
  $(SRCDIR)/services/report_service.c \
  $(SRCDIR)/services/branch_service.c \
  $(SRCDIR)/services/list_service.c \
  $(SRCDIR)/database/db.c \
  $(SRCDIR)/utils/json_utils.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ -o $@ $(LDFLAGS) $(LDLIBS)

build/test_isbn_service.exe: tests/test_isbn_service.c \
  $(SRCDIR)/services/isbn_service.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ -o $@ $(LDFLAGS) $(LDLIBS)

test: $(TEST_BINS)
	@echo "Running unit tests..."
	@for t in $(TEST_BINS); do echo "== $$t =="; ./$$t || exit 1; done

run: $(BIN)
	./$(BIN)

clean:
	rm -rf build

help:
	@echo "LMS backend - available targets:"
	@echo "  make        build the server ($(BIN))"
	@echo "  make run    build and run the server"
	@echo "  make test   build and run unit tests"
	@echo "  make clean  remove build artifacts"

-include $(DEPS)
