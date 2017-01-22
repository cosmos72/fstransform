AUTOMAKE_OPTIONS = subdir-objects

sbin_PROGRAMS = fsmove

fsmove_SOURCES = \
  ../src/args.cc \
  ../src/assert.cc \
  ../src/copy.cc \
  ../src/eta.cc \
  ../src/cache/cache_symlink.cc \
  ../src/io/disk_stat.cc \
  ../src/io/io.cc \
  ../src/io/io_posix.cc \
  ../src/io/io_posix_dir.cc \
  ../src/io/io_prealloc.cc \
  ../src/io/util_dir.cc \
  ../src/io/util_posix.cc \
  ../src/log.cc \
  ../src/main.cc \
  ../src/misc.cc \
  ../src/move.cc \
  ../src/mstring.cc \
  ../src/zmem.cc \
  ../src/zpage.cc

# ../src/io/util.cc

fsmove_LDADD = $(LD_LIBZ)
