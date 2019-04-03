#
# This simple script is a helper to run torture tests on 'fstransform.sh'
#
# It is designed assuming the availability of certain files and commands
# in certain hard-coded paths,
# and it will NOT be useful for the general public until customized.
#

DEVICE_FILE_SAVE=0.device
DEVICE_FILE=1.device
LOOP_FILE=random
DEVICE_MOUNT_POINT=device
ZERO_FILE=zero

FSREMAP=fsremap
CAT_RANDOM=cat-random

set -e
set -x

if [ "$1" = "" ]; then
  TESTDIR=torture-test.dir.0
else
  TESTDIR="$1"
  shift
fi

if [ "$1" = "" ]; then
  # this is a race condition if multiple torture-test.sh are executed simultaneously:
  # in such case you must specify explicitly which loop device to use
  DEVICE=`losetup -f`
else
  DEVICE="$1"
  shift
fi

if [ "$1" = "" ]; then
  FILE_SIZE=16777216
else
  FILE_SIZE=`expr $1 / 4096 \* 4096`
  shift
fi

if [ "$#" = 0 ]; then
  MKFS="mkfs -t ext2"
else
  MKFS="$@"
fi

DEVICE_SIZE=`expr $FILE_SIZE \* 11 / 10 / 4096 \* 4096`

mkdir "$TESTDIR" || true
mount -t tmpfs tmpfs "$TESTDIR"
mkdir "$TESTDIR"/device
pushd "$TESTDIR"
exec >& log.txt

cleanup() {
  set +e
  exec >& /dev/tty
  trap - EXIT SIGHUP SIGINT SIGQUIT SIGILL SIGTRAP SIGABRT SIGBUS SIGFPE SIGSEGV SIGALRM SIGTERM SIGSTKFLT
  echo 'press ENTER to delete test files and quit.'
  read dummy
  popd
  umount "$TESTDIR"/$DEVICE_MOUNT_POINT
  losetup -d "$DEVICE"
  umount "$TESTDIR"
  rmdir "$TESTDIR"
  exit 1
}

trap cleanup EXIT SIGHUP SIGINT SIGQUIT SIGILL SIGTRAP SIGABRT SIGBUS SIGFPE SIGSEGV SIGALRM SIGTERM SIGSTKFLT

$CAT_RANDOM $FILE_SIZE > $LOOP_FILE
truncate -s $DEVICE_SIZE $LOOP_FILE

while true; do
  rm -f $DEVICE_FILE

  truncate -s $DEVICE_SIZE $DEVICE_FILE

  losetup -d "$DEVICE" 2>/dev/null || true
  losetup "$DEVICE" $DEVICE_FILE
  $MKFS "$DEVICE"
  mount "$DEVICE" $DEVICE_MOUNT_POINT
  cp -avf --sparse=always $LOOP_FILE $DEVICE_MOUNT_POINT/
  dd bs=512 if=/dev/zero of=$DEVICE_MOUNT_POINT/$ZERO_FILE 2>/dev/null || true
  # show $ZERO_FILE length
  stat --format=%s $DEVICE_MOUNT_POINT/$ZERO_FILE

  mount -o remount,ro "$DEVICE"
  sync
  cp -vf --sparse=always "$DEVICE" $DEVICE_FILE_SAVE

  $FSREMAP -t . -q "$DEVICE" $DEVICE_MOUNT_POINT/$LOOP_FILE $DEVICE_MOUNT_POINT/$ZERO_FILE < /dev/null
  umount "$DEVICE" || true

  if false; then
    if ! cmp "$DEVICE" $DEVICE_FILE; then
      echo "$DEVICE and $DEVICE_FILE differ!!! bug in Linux loop devices?"
      cmp -l "$DEVICE" $DEVICE_FILE | less
      break
    fi
  fi || true
  sync
  if ! losetup -d "$DEVICE"; then
    fuser -vm "$DEVICE" || true
    losetup -d "$DEVICE"
  fi
  if ! cmp $LOOP_FILE $DEVICE_FILE; then
    echo "$LOOP_FILE and $DEVICE_FILE differ!!! bug in fsremap?"
    cmp -l $LOOP_FILE $DEVICE_FILE | less
    break
  fi
done

exit 1

