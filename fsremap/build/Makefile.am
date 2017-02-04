AUTOMAKE_OPTIONS = subdir-objects

sbin_PROGRAMS = fsremap

fsremap_LDADD = $(LD_LIBZ)

fsremap_SOURCES =           \
  ../src/arch/mem.cc        \
  ../src/arch/mem_linux.cc  \
  ../src/arch/mem_posix.cc  \
  ../src/args.cc            \
  ../src/assert.cc          \
  ../src/dispatch.cc        \
  ../src/eta.cc             \
  ../src/io/extent_file.cc  \
  ../src/io/extent_posix.cc \
  ../src/io/io.cc           \
  ../src/io/io_null.cc      \
  ../src/io/io_posix.cc     \
  ../src/io/io_posix_dir.cc \
  ../src/io/io_prealloc.cc  \
  ../src/io/io_self_test.cc \
  ../src/io/io_test.cc      \
  ../src/io/persist.cc      \
  ../src/io/util_dir.cc     \
  ../src/io/util_posix.cc   \
  ../src/job.cc             \
  ../src/log.cc             \
  ../src/main.cc            \
  ../src/map.cc             \
  ../src/map_stat.cc        \
  ../src/misc.cc            \
  ../src/mstring.cc         \
  ../src/pool.cc            \
  ../src/remap.cc           \
  ../src/tmp_zero.cc        \
  ../src/ui/ui.cc           \
  ../src/ui/ui_tty.cc       \
  ../src/vector.cc          \
  ../src/work.cc            \
  ../src/zmem/zmem.cc       \
  ../src/zmem/zpage.cc      \
  ../src/zmem/zpool.cc      \
  ../src/zmem/zptr.cc       \
  ../src/zmem/ztree.cc      \
  ../src/zmem/ztest.cc
