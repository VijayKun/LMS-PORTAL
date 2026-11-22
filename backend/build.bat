@echo off
set SRC_DIR=src
set LIB_DIR=lib
set BUILD_DIR=build
set OBJ_DIR=build\obj

if not exist %BUILD_DIR% mkdir %BUILD_DIR%
if not exist %OBJ_DIR%\src mkdir %OBJ_DIR%\src
if not exist %OBJ_DIR%\src\utils mkdir %OBJ_DIR%\src\utils
if not exist %OBJ_DIR%\src\services mkdir %OBJ_DIR%\src\services
if not exist %OBJ_DIR%\src\controllers mkdir %OBJ_DIR%\src\controllers
if not exist %OBJ_DIR%\src\database mkdir %OBJ_DIR%\src\database
if not exist %OBJ_DIR%\src\server mkdir %OBJ_DIR%\src\server
if not exist %OBJ_DIR%\src\config mkdir %OBJ_DIR%\src\config
if not exist %OBJ_DIR%\src\auth mkdir %OBJ_DIR%\src\auth
if not exist %OBJ_DIR%\src\middleware mkdir %OBJ_DIR%\src\middleware
if not exist %OBJ_DIR%\src\security mkdir %OBJ_DIR%\src\security
if not exist %OBJ_DIR%\src\routes mkdir %OBJ_DIR%\src\routes
if not exist %OBJ_DIR%\%LIB_DIR% mkdir %OBJ_DIR%\%LIB_DIR%

set GCC=C:\msys64\ucrt64\bin\gcc.exe
set INCLUDES=-I./src -I./lib -I/C/msys64/ucrt64/include
set LIBS=-LC:/msys64/ucrt64/lib -lpq -lcrypto -lws2_32 -lwinhttp

%GCC% -std=c11 -Wall -Wextra -O0 -g %INCLUDES% -c src/main.c -o %OBJ_DIR%/src/main.o
%GCC% -std=c11 -Wall -Wextra -O0 -g %INCLUDES% -c src/config/config.c -o %OBJ_DIR%/src/config/config.o
%GCC% -std=c11 -Wall -Wextra -O0 -g %INCLUDES% -c src/database/db.c -o %OBJ_DIR%/src/database/db.o
%GCC% -std=c11 -Wall -Wextra -O0 -g %INCLUDES% -c src/server/server.c -o %OBJ_DIR%/src/server/server.o
%GCC% -std=c11 -Wall -Wextra -O0 -g %INCLUDES% -c src/routes/health_routes.c -o %OBJ_DIR%/src/routes/health_routes.o
%GCC% -std=c11 -Wall -Wextra -O0 -g %INCLUDES% -c src/routes/auth_routes.c -o %OBJ_DIR%/src/routes/auth_routes.o
%GCC% -std=c11 -Wall -Wextra -O0 -g %INCLUDES% -c src/routes/book_routes.c -o %OBJ_DIR%/src/routes/book_routes.o
%GCC% -std=c11 -Wall -Wextra -O0 -g %INCLUDES% -c src/routes/student_routes.c -o %OBJ_DIR%/src/routes/student_routes.o
%GCC% -std=c11 -Wall -Wextra -O0 -g %INCLUDES% -c src/routes/loan_routes.c -o %OBJ_DIR%/src/routes/loan_routes.o
%GCC% -std=c11 -Wall -Wextra -O0 -g %INCLUDES% -c src/routes/reservation_routes.c -o %OBJ_DIR%/src/routes/reservation_routes.o
%GCC% -std=c11 -Wall -Wextra -O0 -g %INCLUDES% -c src/routes/fine_routes.c -o %OBJ_DIR%/src/routes/fine_routes.o
%GCC% -std=c11 -Wall -Wextra -O0 -g %INCLUDES% -c src/routes/notification_routes.c -o %OBJ_DIR%/src/routes/notification_routes.o
%GCC% -std=c11 -Wall -Wextra -O0 -g %INCLUDES% -c src/routes/report_routes.c -o %OBJ_DIR%/src/routes/report_routes.o
%GCC% -std=c11 -Wall -Wextra -O0 -g %INCLUDES% -c src/routes/branch_routes.c -o %OBJ_DIR%/src/routes/branch_routes.o
%GCC% -std=c11 -Wall -Wextra -O0 -g %INCLUDES% -c src/controllers/auth_controller.c -o %OBJ_DIR%/src/controllers/auth_controller.o
%GCC% -std=c11 -Wall -Wextra -O0 -g %INCLUDES% -c src/controllers/book_controller.c -o %OBJ_DIR%/src/controllers/book_controller.o
%GCC% -std=c11 -Wall -Wextra -O0 -g %INCLUDES% -c src/controllers/student_controller.c -o %OBJ_DIR%/src/controllers/student_controller.o
%GCC% -std=c11 -Wall -Wextra -O0 -g %INCLUDES% -c src/controllers/loan_controller.c -o %OBJ_DIR%/src/controllers/loan_controller.o
%GCC% -std=c11 -Wall -Wextra -O0 -g %INCLUDES% -c src/controllers/reservation_controller.c -o %OBJ_DIR%/src/controllers/reservation_controller.o
%GCC% -std=c11 -Wall -Wextra -O0 -g %INCLUDES% -c src/controllers/fine_controller.c -o %OBJ_DIR%/src/controllers/fine_controller.o
%GCC% -std=c11 -Wall -Wextra -O0 -g %INCLUDES% -c src/controllers/notification_controller.c -o %OBJ_DIR%/src/controllers/notification_controller.o
%GCC% -std=c11 -Wall -Wextra -O0 -g %INCLUDES% -c src/controllers/report_controller.c -o %OBJ_DIR%/src/controllers/report_controller.o
%GCC% -std=c11 -Wall -Wextra -O0 -g %INCLUDES% -c src/controllers/branch_controller.c -o %OBJ_DIR%/src/controllers/branch_controller.o
%GCC% -std=c11 -Wall -Wextra -O0 -g %INCLUDES% -c src/services/list_service.c -o %OBJ_DIR%/src/services/list_service.o
%GCC% -std=c11 -Wall -Wextra -O0 -g %INCLUDES% -c src/services/auth_service.c -o %OBJ_DIR%/src/services/auth_service.o
%GCC% -std=c11 -Wall -Wextra -O0 -g %INCLUDES% -c src/services/book_service.c -o %OBJ_DIR%/src/services/book_service.o
%GCC% -std=c11 -Wall -Wextra -O0 -g %INCLUDES% -c src/services/student_service.c -o %OBJ_DIR%/src/services/student_service.o
%GCC% -std=c11 -Wall -Wextra -O0 -g %INCLUDES% -c src/services/loan_service.c -o %OBJ_DIR%/src/services/loan_service.o
%GCC% -std=c11 -Wall -Wextra -O0 -g %INCLUDES% -c src/services/reservation_service.c -o %OBJ_DIR%/src/services/reservation_service.o
%GCC% -std=c11 -Wall -Wextra -O0 -g %INCLUDES% -c src/services/fine_service.c -o %OBJ_DIR%/src/services/fine_service.o
%GCC% -std=c11 -Wall -Wextra -O0 -g %INCLUDES% -c src/services/notification_service.c -o %OBJ_DIR%/src/services/notification_service.o
%GCC% -std=c11 -Wall -Wextra -O0 -g %INCLUDES% -c src/services/report_service.c -o %OBJ_DIR%/src/services/report_service.o
%GCC% -std=c11 -Wall -Wextra -O0 -g %INCLUDES% -c src/services/branch_service.c -o %OBJ_DIR%/src/services/branch_service.o
%GCC% -std=c11 -Wall -Wextra -O0 -g %INCLUDES% -c src/services/isbn_service.c -o %OBJ_DIR%/src/services/isbn_service.o
%GCC% -std=c11 -Wall -Wextra -O0 -g %INCLUDES% -c src/auth/session_store.c -o %OBJ_DIR%/src/auth/session_store.o
%GCC% -std=c11 -Wall -Wextra -O0 -g %INCLUDES% -c src/middleware/auth_middleware.c -o %OBJ_DIR%/src/middleware/auth_middleware.o
%GCC% -std=c11 -Wall -Wextra -O0 -g %INCLUDES% -c src/middleware/cors_middleware.c -o %OBJ_DIR%/src/middleware/cors_middleware.o
%GCC% -std=c11 -Wall -Wextra -O0 -g %INCLUDES% -c src/security/password.c -o %OBJ_DIR%/src/security/password.o
%GCC% -std=c11 -Wall -Wextra -O0 -g %INCLUDES% -c src/utils/json_utils.c -o %OBJ_DIR%/src/utils/json_utils.o
%GCC% -std=c11 -Wall -Wextra -O0 -g %INCLUDES% -c src/utils/json_parse.c -o %OBJ_DIR%/src/utils/json_parse.o
%GCC% -std=c11 -Wall -Wextra -O0 -g %INCLUDES% -c src/utils/response_utils.c -o %OBJ_DIR%/src/utils/response_utils.o
%GCC% -std=c11 -Wall -Wextra -O0 -g %INCLUDES% -c %LIB_DIR%/mongoose.c -o %OBJ_DIR%/%LIB_DIR%/mongoose.o

%GCC% %OBJ_DIR%/src/*.o %OBJ_DIR%/src/services/*.o %OBJ_DIR%/src/controllers/*.o %OBJ_DIR%/src/database/*.o %OBJ_DIR%/src/server/*.o %OBJ_DIR%/src/config/*.o %OBJ_DIR%/src/auth/*.o %OBJ_DIR%/src/middleware/*.o %OBJ_DIR%/src/security/*.o %OBJ_DIR%/src/utils/*.o %OBJ_DIR%/%LIB_DIR%/mongoose.o %LIBS% -o %BUILD_DIR%/lms_backend.exe

echo Build complete: %BUILD_DIR%/lms_backend.exe