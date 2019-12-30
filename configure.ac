AC_PREREQ([2.68])
AC_INIT([renogySolar], [1.0], [stevecoul@icloud.com])
AM_INIT_AUTOMAKE([subdir-objects])
AC_CONFIG_HEADERS([config.h])

AC_PROG_CXX
AC_PROG_RANLIB

AC_CHECK_LIB(sqlite3,sqlite3_prepare)
LIBS=

AC_CHECK_HEADERS([arpa/inet.h fcntl.h netinet/in.h stdlib.h string.h sys/ioctl.h sys/time.h syslog.h termios.h unistd.h])

#AC_CHECK_HEADER_STDBOOL
AC_TYPE_PID_T
AC_TYPE_SIZE_T

AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([gettimeofday memset socket strdup strerror])

AC_CONFIG_FILES(Makefile)
AC_OUTPUT

