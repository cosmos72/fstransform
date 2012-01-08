#!/bin/dash

BOOT_CMD_which=which

CMDS_bootstrap="which expr id"
CMDS="blockdev losetup mount umount mkdir rmdir dd sync fsck mkfs fsmove fsremap"

echo "fstransform 0.3.3 starting, checking environment..."

# start with a clean environment
ERR=0
DEVICE=
FSTYPE=
DEVICE_SIZE_IN_BYTES=
DEVICE_MOUNT_POINT=
DEVICE_FSTYPE=
LOOP_FILE=
LOOP_DEVICE=
LOOP_MOUNT_POINT=
ZERO_FILE=
FSMOVE_OPTS=
FSREMAP_OPTS=

for cmd in $CMDS; do
  eval "CMD_$cmd="
done

# parse command line arguments and set USER_CMD_* variables accordingly
parse_args() {
  echo "parsing command line arguments"
  for arg in "$@"; do
    case "$arg" in
      --cmd-*=* )
        cmd="`$CMD_expr match \"$arg\" '--cmd-\(.*\)=.*'`"
        user_cmd="`$CMD_expr match \"$arg\" '--cmd-.*=\(.*\)'`"
        eval "USER_CMD_$cmd=\"$user_cmd\""
        ;;
      --loop-file=*)
        LOOP_FILE="`$CMD_expr match \"$arg\" '--loop-file=\(.*\)'`"
	echo "loop file '$LOOP_FILE' specified on command line"
        ;;
      --loop-mount-point=*)
        LOOP_MOUNT_POINT="`$CMD_expr match \"$arg\" '--loop-mount-point=\(.*\)'`"
	echo "loop file mount point '$LOOP_MOUNT_POINT' specified on command line"
        ;;
      --zero-file=*)
        ZERO_FILE="`$CMD_expr match \"$arg\" '--zero-file=\(.*\)'`"
	echo "zero file '$ZERO_FILE' specified on command line"
        ;;
      --fsmove-opts=*)
        FSMOVE_OPTS="$CMD_expr match \"$arg\" '--fsmove-opts=\(.*\)'"
	;;
      --fsremap-opts=*)
        FSREMAP_OPTS="$CMD_expr match \"$arg\" '--fsremap-opts=\(.*\)'"
	;;
      --*)
        echo "ignoring unknown option '$arg'"
        ;;
      *)
        if [ "$DEVICE" = "" ]; then
	  DEVICE="$arg"
	elif [ "$FSTYPE" = "" ]; then
	  FSTYPE="$arg"
	else
          echo "ignoring extra argument '$arg'"
	fi
	;;
    esac
  done
}

detect_cmd() {
  local my_cmd_which="$CMD_which"
  if test "$my_cmd_which" = ""; then
    my_cmd_which="$BOOT_CMD_which"
  fi

  local cmd="$1"
  echo -n "checking for $cmd...	"
  local user_cmd="`eval echo '$USER_CMD_'\"$cmd\"`"
  local my_cmd=

  if test "$user_cmd" != "" ; then
    my_cmd="`$my_cmd_which \"$user_cmd\"`" >/dev/null 2>&1
    if test "$my_cmd" != "" ; then
      if test -x "$my_cmd" ; then
        echo "found '$my_cmd' ('$user_cmd' was specified)"
        eval "CMD_$cmd=\"$my_cmd\""
        return 0
      fi
    fi
  fi

  my_cmd="`$my_cmd_which \"$cmd\"`" >/dev/null 2>&1
  if test "$my_cmd" != "" ; then
    if test -x "$my_cmd" ; then
      echo "found '$my_cmd'"
      eval "CMD_$cmd=\"$my_cmd\""
      return 0
   else
      echo "found '$my_cmd', but is not executable by you!"
    fi
  else
    echo "not found!"
  fi
  return 1
}

fail() {
  echo "environment check failed."
  echo "please fix your \$PATH and/or install missing commands before running fstransform"
  exit "$ERR"
}



# bootstrap command detection (command 'which') and argument parsing (command 'expr')
for cmd in $CMDS_bootstrap; do
  detect_cmd "$cmd" || ERR="$?"
done
if [ "$ERR" != 0 ]; then
  echo
  fail
fi

check_uid_0() {
  UID="`$CMD_id -u`"
  if [ "$UID" != 0 ]; then
    echo "FATAL ERROR: this script must be executed as root (uid 0)"
    echo "             instead it is currently running as (uid $UID)"
    exit 1
  fi
}
check_uid_0


parse_args "$@"

for cmd in $CMDS; do
  detect_cmd "$cmd" || ERR="$?"
done

if [ "$ERR" != 0 ]; then
  echo
  fail
fi

echo "environment check passed."

check_command_line_args() {
  if [ "$DEVICE" = "" ]; then
    echo "missing command-line argument DEVICE!"
    exit 1
  fi
  if [ "$FSTYPE" = "" ]; then
    echo "missing command-line argument FSTYPE!"
    exit 1
  fi
}
check_command_line_args

exec_cmd() {
  "$@"
  ERR="$?"
  if [ "$ERR" != 0 ]; then
    echo "FATAL ERROR! command $@ failed (exit status $ERR)"
    echo
    echo "      this is potentially a problem."
    echo "      you can either quit now by pressing CTRL+C,"
    echo
    echo "      or, if you know what went wrong, you can fix it yourself,"
    echo "      manually run the command '$@'"
    echo "      (or something equivalent)"
    echo -n "      and finally resume this script by pressing ENTER"
    read dummy
  fi
}

capture_cmd() {
  local ret_var="$1"
  shift
  local ret=`"$@"`
  local my_err="$?"
  if [ "$my_err" != 0 ]; then
    ERR="$my_err"
    echo "FATAL ERROR! command $@ failed (exit status $ERR)"
    exit "$ERR"
  elif [ "$ret" = "" ]; then
    echo "FATAL ERROR! command $@ failed (no output)"
    exit 1
  fi
  eval "$ret_var=\"$ret\""
}




echo "preparing to transform device '$DEVICE' to filesystem type '$FSTYPE'"



capture_cmd DEVICE_SIZE_IN_BYTES "$CMD_blockdev" --getsize64 "$DEVICE"
echo "detected '$DEVICE' size: $DEVICE_SIZE_IN_BYTES bytes"

echo_device_mount_point_and_fstype() {
  local my_dev="$1"
  "$CMD_mount" | while read dev _on_ mount_point _type_ fstype opts; do
    if [ "$dev" = "$my_dev" ]; then
      echo "$mount_point $fstype"
      break
    fi
  done
}

find_device_mount_point_and_fstype() {
  local my_dev="$DEVICE"
  local ret="`echo_device_mount_point_and_fstype \"$my_dev\"`"
  if [ "$ret" = "" ]; then
    echo "FATAL ERROR! device '$my_dev' not found in the output of command $CMD_mount"
    echo "             maybe device '$my_dev' is not mounted?"
    exit 1
  fi
  local my_mount_point=
  local my_fstype=
  for i in $ret; do
    if [ "$my_mount_point" = "" ]; then
      my_mount_point="$i"
    else
      my_fstype="$i"
    fi
  done
  echo "detected '$my_dev' mount point '$my_mount_point' with filesystem type '$my_fstype'"
  if [ ! -e "$my_mount_point" ]; then
    echo "FATAL ERROR! mount point '$my_mount_point' does not exist."
    echo "             maybe device '$my_dev' is mounted on a path containing spaces?"
    echo "             fstransform.sh does not support mount points containing spaces in their path"
    exit 1
  fi
  if [ ! -d "$my_mount_point" ]; then
    echo "FATAL ERROR! mount point '$my_mount_point' is not a directory"
    exit 1
  fi
  DEVICE_MOUNT_POINT="$my_mount_point"
  DEVICE_FSTYPE="$my_fstype"
}
find_device_mount_point_and_fstype


create_aux_file() {
  local my_kind="$1" # loop or zero
  local ret_var="$2"
  local my_file="$3"
  local my_pattern="$DEVICE_MOUNT_POINT/.fstransform.$my_kind.*"
  local my_files="`echo $my_pattern`"
  if [ "$my_files" != "$my_pattern" ]; then
    echo
    echo "WARNING: possibly stale fstransform $my_kind files found inside device '$DEVICE', maybe they can be removed?"
    echo "         files found:"
    echo "$my_files"
    echo
  fi
  if [ "$my_file" = "" ]; then
    my_file="$DEVICE_MOUNT_POINT/.fstransform.$my_kind.$$"
    echo "creating sparse $my_kind file '$my_file' inside device '$DEVICE'..."
    if [ -e "$my_file" ]; then
      echo "FATAL ERROR! $my_kind file '$my_file' already exists! please remove it"
      exit 1
    fi
  else
    # check that user-specified file is actually inside DEVICE_MOUNT_POINT
    "$CMD_expr" match "$my_file" "$DEVICE_MOUNT_POINT/.*" >/dev/null 2>/dev/null || ERR="$?"
    if [ "$ERR" != 0 ]; then
      echo "FATAL ERROR! user-specified $my_kind file '$my_file' does not seem to be inside device mount point '$DEVICE_MOUNT_POINT'"
      echo "             please use a $my_kind file path that starts with '$DEVICE_MOUNT_POINT/'"
      exit "$ERR"
    fi
    "$CMD_expr" match "$my_file" '.*/\.\./.*' >/dev/null 2>/dev/null
    if [ "$?" = 0 ]; then
      echo "FATAL ERROR! user-specified $my_kind file '$my_var' contains '/../' in path"
      echo "             somebody is trying to cheat?"
      echo "             I give up"
      exit "$ERR"
    fi
    echo "overwriting $my_kind file '$my_file' inside device '$DEVICE'..."
  fi
  > "$my_file"
  ERR="$?"
  if [ "$ERR" != 0 ]; then
    echo "FATAL ERROR! failed to create or truncate '$my_file' to zero bytes"
    echo "             maybe device '$DEVICE' is full or mounted read-only?"
    exit "$ERR"
  fi
  eval "$ret_var=\"$my_file\""
}
create_loop_file() {
  create_aux_file loop LOOP_FILE "$LOOP_FILE"
  exec_cmd "$CMD_dd" if=/dev/zero of="$LOOP_FILE" bs=1 count=1 seek="`\"$CMD_expr\" \"$DEVICE_SIZE_IN_BYTES\" - 1`"
}
create_loop_file


connect_loop_device() {
  capture_cmd LOOP_DEVICE "$CMD_losetup" -f
  exec_cmd "$CMD_losetup" "$LOOP_DEVICE" "$LOOP_FILE"
  echo "connected loop device '$LOOP_DEVICE' to file '$LOOP_FILE'"
}
connect_loop_device

disconnect_loop_device() {
  local my_iter=0
  # loop device sometimes needs a little time to become free...
  for my_iter in 1 2 3 4; do
    exec_cmd "$CMD_sync"
    if [ "$my_iter" -le 3 ]; then
      "$CMD_losetup" -d "$LOOP_DEVICE" && break
    else
      exec_cmd "$CMD_losetup" -d "$LOOP_DEVICE"
    fi
  done
  echo "disconnected loop device '$LOOP_DEVICE' from file '$LOOP_FILE'"
}


format_file_or_device() {
  local my_file_or_device="$1"
  local my_fstype="$2"
  echo "formatting '$my_file_or_device' with filesystem type '$my_fstype'..."
  exec_cmd "$CMD_mkfs" -t "$my_fstype" "$my_file_or_device"
}
format_file_or_device "$LOOP_DEVICE" "$FSTYPE" 


mount_loop_file() {
  if [ "$LOOP_MOUNT_POINT" = "" ]; then
    LOOP_MOUNT_POINT="/tmp/fstransform.loop.mount_point.$$"
    exec_cmd "$CMD_mkdir" "$LOOP_MOUNT_POINT"    
  else
    "$CMD_expr" match "$LOOP_MOUNT_POINT" "/.*" >/dev/null 2>/dev/null
    if [ "$?" != 0 ]; then
      echo
      echo "WARNING: user-specified loop file mount point '$LOOP_MOUNT_POINT' should start with '/'"
      echo "         i.e. it should be an absolute path"
      echo "         fstransform cannot ensure that '$LOOP_MOUNT_POINT' is outside '$DEVICE_MOUNT_POINT'"
      echo "         continue at your own risk"
      echo
      echo -n "         press ENTER to continue, or CTRL+C to quit."
      read dummy
    else
      "$CMD_expr" match "$LOOP_MOUNT_POINT" "$DEVICE_MOUNT_POINT/.*" >/dev/null 2>/dev/null
      if [ "$?" = 0 ]; then
        echo "FATAL ERROR! user-specified loop file mount point '$LOOP_MOUNT_POINT' seems to be inside '$DEVICE_MOUNT_POINT'"
	echo "             somebody is trying to break fstransform and lose data?"
	echo "             I give up"
	exit 1
      fi
    fi
  fi
  echo "mounting loop device '$LOOP_DEVICE' on '$LOOP_MOUNT_POINT' ..."
  exec_cmd "$CMD_mount" "$LOOP_DEVICE" "$LOOP_MOUNT_POINT"
  echo "loop device mounted successfully."
}
mount_loop_file


move_device_contents_into_loop_file() {
  echo "preliminary steps completed, now comes the delicate part:"
  echo "fstransform will move '$DEVICE' contents into the loop file."
  echo
  echo "WARNING: you will LOSE YOUR DATA if loop file becomes full!"
  echo "         please open another terminal, type"
  echo "         'watch df $DEVICE $LOOP_DEVICE'"
  echo "         and check that both the original device '$DEVICE'"
  echo "         and the loop device '$LOOP_DEVICE'"
  echo "         are NOT becoming full."
  echo
  echo "         if one of them is almost full,"
  echo "         you MUST stop fstransform.sh with CTRL+C or equivalent."
  echo
  echo "THIS IS IMPORTANT! you will LOSE YOUR DATA if the original device '$DEVICE'"
  echo "                   or the loop device '$LOOP_DEVICE'"
  echo "                   becomes full."
  echo
  echo "         this is your last chance to quit."
  echo -n "         press ENTER to continue, or CTRL+C to quit."
  read dummy
  echo "moving '$DEVICE' contents into the loop file."
  echo "    this may take some time, please be patient..."
  exec_cmd "$CMD_fsmove" $FSMOVE_OPTS -- "$DEVICE_MOUNT_POINT" "$LOOP_MOUNT_POINT" --exclude "$LOOP_FILE"
}
move_device_contents_into_loop_file

umount_and_fsck_loop_file() {
  exec_cmd "$CMD_umount" "$LOOP_MOUNT_POINT"
  # ignore errors if removing "$LOOP_MOUNT_POINT" fails
  "$CMD_rmdir" "$LOOP_MOUNT_POINT"
  exec_cmd "$CMD_sync"
  exec_cmd "$CMD_fsck" -f "$LOOP_DEVICE"
}
umount_and_fsck_loop_file

disconnect_loop_device

create_zero_file() {
  create_aux_file zero ZERO_FILE "$ZERO_FILE"
  echo "creating a file '$ZERO_FILE' full of zeroes inside device '$DEVICE'"
  echo "    to help '$CMD_fsremap' locating unused space."
  echo "    this may take a while, please be patient..."
  # next command will fail with "no space left on device".
  # this is normal and expected.
  "$CMD_dd" if=/dev/zero of="$ZERO_FILE" bs=64k 2>/dev/null
  echo "file full of zeroes created successfully."
}
create_zero_file

remount_device_ro_and_fsck() {
  echo "remounting device '$DEVICE' read-only"
  exec_cmd "$CMD_sync"
  exec_cmd "$CMD_mount" "$DEVICE" -o remount,ro
  #echo "running '$CMD_fsck' (disk check) on device '$DEVICE'"
  #exec_cmd "$CMD_fsck" -f "$DEVICE"
}
remount_device_ro_and_fsck


remap_device() {
  echo "launching '$CMD_fsremap' in SIMULATED mode"
  exec_cmd "$CMD_fsremap" $FSREMAP_OPTS -n -q -- "$DEVICE" "$LOOP_FILE" "$ZERO_FILE"
  echo "launching $CMD_fsremap' in real mode to perform in-place remapping."
  exec_cmd "$CMD_fsremap" $FSREMAP_OPTS -q -- "$DEVICE" "$LOOP_FILE" "$ZERO_FILE"
}
remap_device

sync_and_fsck_device() {
  echo "running again '$CMD_fsck' (disk check) on device '$DEVICE'"
  exec_cmd "$CMD_sync"
  "$CMD_fsck" -f "$DEVICE"
  ERR="$?"
  if [ "$ERR" != 0 ]; then
    echo "ERROR! command $CMD_fsck -f $DEVICE failed! (exit status $ERR)"
    echo "       this can happen if $DEVICE was actually a loop-device /dev/loop*"
    echo "       or it can indicate a real problem happened while remapping your data"
    echo "       please manually run a filesystem check on device '$DEVICE'"
  fi
}
sync_and_fsck_device

mount_device() {
  echo "mounting transformed device '$DEVICE' on mount point '$DEVICE_MOUNT_POINT'"
  "$CMD_mount" "$DEVICE" "$DEVICE_MOUNT_POINT"
  ERR="$?"
  if [ "$ERR" != 0 ]; then
    echo "ERROR! command $CMD_mount $DEVICE $DEVICE_MOUNT_POINT failed! (exit status $ERR)"
    echo "       this can happen if $DEVICE was actually a loop-device /dev/loop*"
    echo "       or it can indicate a real problem happened while remapping your data"
    echo "       please manually run a filesystem check on device '$DEVICE'"
  fi
}
mount_device
