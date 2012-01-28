#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.65])
AC_INIT([fstransform], [0.3.8], [BUG-REPORT-ADDRESS])
AC_CONFIG_SRCDIR([fsremap/src/remap.cc])
AC_CONFIG_HEADERS([fsremap/src/config.hh])
AC_CONFIG_AUX_DIR([tools])
AM_INIT_AUTOMAKE

			      
			      
# Checks for programs.
AC_PROG_CXX
AC_PROG_INSTALL

# Checks for libraries.

# Checks for header files.
AC_LANG(C++)
AC_CHECK_HEADERS([cerrno  climits  cstdarg  cstdio  cstdlib  cstring  ctime \
                  errno.h limits.h stdarg.h stdio.h stdlib.h string.h time.h \
                  dirent.h fcntl.h features.h stddef.h stdint.h \
                  linux/fiemap.h linux/fs.h sys/ioctl.h sys/mman.h sys/stat.h \
                  sys/statvfs.h sys/time.h sys/types.h sys/wait.h \
                  termios.h time.h unistd.h utime.h])
AC_LANG(C)

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_MODE_T
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_CHECK_MEMBERS([struct stat.st_rdev])
AC_CHECK_MEMBERS([struct stat.st_atim.tv_nsec])
AC_CHECK_MEMBERS([struct stat.st_mtim.tv_nsec])
AC_CHECK_MEMBERS([struct stat.st_atimensec])
AC_CHECK_MEMBERS([struct stat.st_mtimensec])
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_CHECK_TYPES([long long])
AC_CHECK_TYPES([unsigned long long])

# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_LSTAT_FOLLOWS_SLASHED_SYMLINK
AC_FUNC_MALLOC
AC_FUNC_MMAP
AC_CHECK_FUNCS([execvp ftruncate gettimeofday getuid lchown chown localtime localtime_r \
               memmove memset mkdir mkfifo mlock msync munmap posix_fallocate \
               random remove srandom strerror strftime time tzset \
               utimes utimensat waitpid])

FT_NEED_ALL_FUNCS([execvp ftruncate memmove memset mkdir mkfifo msync munmap \
                   remove strerror strftime time waitpid])
FT_NEED_ANY_FUNC([localtime localtime_r])
FT_NEED_ANY_FUNC([lchown chown])

AC_CONFIG_FILES([Makefile fsremap/build/Makefile fsmove/build/Makefile fstransform/build/Makefile])
AC_OUTPUT

FT_OUTPUT

