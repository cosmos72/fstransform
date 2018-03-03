#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([fstransform],[0.9.3.work13],[BUG-REPORT-ADDRESS])
AC_CONFIG_SRCDIR([fsremap/src/remap.cc])
AC_CONFIG_HEADERS([fsremap/src/config.hh])
AC_CONFIG_AUX_DIR([tools])
AM_INIT_AUTOMAKE



# Checks for programs.
AC_PROG_CXX
AC_PROG_INSTALL

AC_LANG(C++)

# Checks for compiler characteristics
FT_CXX_FLAGS
FT_CXX_FEATURES

# Checks for header files.
AC_CHECK_HEADERS([cerrno  climits  cmath  cstdarg  cstdio  cstdlib  cstring  ctime \
                  errno.h limits.h math.h stdarg.h stdio.h stdlib.h string.h time.h \
                  dirent.h fcntl.h features.h stddef.h stdint.h \
                  ext2fs/ext2fs.h linux/fiemap.h linux/fs.h \
                  sys/disklabel.h sys/ioctl.h sys/mman.h sys/mount.h sys/stat.h \
                  sys/statvfs.h sys/time.h sys/types.h sys/wait.h \
                  termios.h time.h unistd.h utime.h \
                  tr1/unordered_map unordered_map zlib.h])

# Checks for typedefs, and structures
AC_TYPE_MODE_T
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_CHECK_TYPES([long long])
AC_CHECK_TYPES([unsigned long long])

AC_CHECK_MEMBERS([struct disklabel.d_secsize, struct disklabel.d_secperunit], [], [], [AC_INCLUDES_DEFAULT [
#ifdef HAVE_SYS_DISKLABEL_H
#include <sys/disklabel.h>
#endif]])

AC_CHECK_MEMBERS([struct stat.st_rdev, struct stat.st_atim.tv_nsec, struct stat.st_mtim.tv_nsec,
                  struct stat.st_atimensec, struct stat.st_mtimensec])






# Checks for C library functions.
AC_FUNC_FORK
AC_FUNC_LSTAT_FOLLOWS_SLASHED_SYMLINK
AC_FUNC_MALLOC
AC_FUNC_MMAP
AC_CHECK_FUNCS([execvp fallocate posix_fallocate fdatasync fileno fsync ftruncate \
               getpagesize gettimeofday getuid lchown chown isatty localtime_r localtime \
               memmove memset mkdir mkfifo mlock mount msync munmap random remove \
               srandom strerror strftime sync sysconf time tzset utimes utimensat \
               waitpid])


# Checks for C++ library features.
FT_CXX_STD_UNORDERED_MAP
FT_CXX_STD_TR1_UNORDERED_MAP


# Checks for libraries.
AC_CHECK_LIB(com_err, error_message,        [AC_DEFINE(HAVE_LIBCOM_ERR, 1, [Define to 1 if you have the com_err library.])
                                             AC_SUBST(LD_LIBCOM_ERR, [-lcom_err])])
AC_CHECK_LIB(ext2fs, ext2fs_extent_replace, [AC_DEFINE(HAVE_LIBEXT2FS, 1, [Define to 1 if you have the ext2fs library.])
                                             AC_SUBST(LD_LIBEXT2FS, [-lext2fs])])
dnl AC_CHECK_LIB(z,      deflate,               [AC_DEFINE(HAVE_Z_DEFLATE, 1, [Define to 1 if you have the z library.])
dnl                                              AC_SUBST(LD_LIBZ, [-lz])])

FT_NEED_ALL_FUNCS([execvp fileno ftruncate memmove memset mkdir mkfifo mount msync munmap \
                   remove strerror strftime sync time waitpid])
FT_NEED_ANY_FUNC([localtime_r localtime])
FT_NEED_ANY_FUNC([lchown chown])

dnl FT_NEED_ALL_LIBS([com_err_error_message ext2fs_ext2fs_extent_replace])

dnl FT_NEED_ALL_LIBS([z_deflate])

if test "$ac_cv_func_fallocate" != "yes"
then
  AC_MSG_WARN([missing function fallocate(), experimental files preallocation will NOT be compiled])

elif test "$ac_cv_lib_com_err_error_message" != "yes"
then
  AC_MSG_WARN([missing function error_message() in -lcom_err, experimental files preallocation will NOT be compiled])

elif test "$ac_cv_lib_ext2fs_ext2fs_extent_replace" != "yes"
then
  AC_MSG_WARN([missing function ext2fs_extent_replace() in -lext2fs, experimental files preallocation will NOT be compiled])

else
  AC_DEFINE(HAVE_IO_PREALLOC, 1, [Define to 1 if "prealloc" I/O is supported"])
  AC_MSG_NOTICE([support for experimental files preallocation will be compiled])
fi


AC_CONFIG_FILES([Makefile fsattr/build/Makefile fsremap/build/Makefile fsmove/build/Makefile fsmount_kernel/build/Makefile fstransform/build/Makefile])
AC_OUTPUT

FT_OUTPUT

