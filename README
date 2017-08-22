###############################################################
###############################################################
###
###                        DISCLAIMER
###
###             THIS DOCUMENT EXPLAINS HOW TO USE
###               RISKY PROGRAMS AND PROCEDURES
###        THAT MAY COMPLETELY AND IRREVERSIBLY DELETE
###                ALL THE DATA ON YOUR DISKS
###
###         THE AUTHOR DECLINES ALL RESPONSIBILITIES
###               FOR ANY DAMAGE THAT MAY DERIVE
###           FROM USING THE PROGRAMS AND PROCEDURES
###               DESCRIBED IN THIS DOCUMENT
###
###############################################################
###############################################################


This document explains how use the programs 'fstransform', 'fsmove',
'fsmount_kernel' and 'fsremap' to transform the contents of a Linux
device - usually a disk partition - from a filesystem type to another
while preserving its contents.

For example, a disk partition can be transformed from 'jfs' to 'ext4',
or from 'ext2' to 'xfs', or many other combinations.

Currently, the programs mentioned above have been tested on Linux
with the following filesystems, both as source and as target:
ext2, ext3, ext4, jfs, ntfs, reiserfs, xfs.

Do NOT use these programs with other filesystems
unless you are willing to LOSE your data.

In particular, they do NOT (yet) support msdos, vfat and exfat file systems.

Common sense and experience tell that you should ALWAYS have a backup
of your valuable data: while the programs do NOT need to backup your data
to operate, YOU need a backup in case something goes wrong.

The programs have been tested carefully, yet there is ALWAYS a possibility
that they will irreversibly delete ALL the data on the device you run them on,
even if you use a tested combination of filesystems.

All this foreword means only one thing:

      IF YOU LOSE YOUR DATA, IT IS YOUR PROBLEM.

The author declines ALL responsibilities for ANY damage that may derive
from using the programs and procedures described in this document.

###############################################################
###############################################################

### Introduction

Enough legalese... now let's get to the interesting part.

The program 'fstransform' does the following:

it takes a device with a filesystem on it (even if almost full)
and transforms the device to a different filesystem type,
in-place (i.e. without backup) and non-destructively
(i.e. it preserves all your data).

It works even if the filesystem is almost full
- several empyrical tests have succeeded even with a 95% full filesystem -
and even if it contains very large files, for example if some files
are larger than half the device or larger than the available space.

### REQUIREMENTS

There are five requirements for fstransform to have a chance to succeed:

1. the device must be unmountable, i.e. `umount DEVICE` must work.
   In particular, if some running programs are using the device,
   you must first close or kill them.

   Transforming the current root directory does not work. For that, you should
   boot from a different installation (for example a live CD, DVD or USB).

2. the device must have a little free space, typically at least 5%

   WARNING: transforming an almost full device to 'xfs' file-system
   can be tricky:
   * you need either slightly more free space, typically at least 10%,
   * or you must be VERY quick at suspending fstransform
     when either the source or the target (or both) file-system is almost full
     and run 'xfs_fsr' on the source or target device (or both)
     before resuming fstransform.
     A future fstransform version may automate this operation.

3. the filesystem on the device must support SPARSE FILES, i.e. files with holes
   (see for example http://en.wikipedia.org/wiki/Sparse_file for an explanation of what they are)
   and at least one of the two system calls "ioctl(FS_IOC_FIEMAP)" or "ioctl(FIBMAP)"
   (see the file Documentation/filesystems/fiemap.txt in any recent Linux kernel
   for an explanation, or search for the same file on Internet)

   ioctl(FIBMAP) is limited by design to 2G-1 blocks, which typically translates to 8TB - 4kB.
   To transform file systems equal or larger than 8TB, ioctl(FIEMAP) is required.

4. the initial and final filesystems must be supported by the Linux kernel
   (i.e. it must be able to mount them)
   and by the tools 'mkfs' and 'fsck'
   (i.e. it must be possible to create them and check them for errors).

   Support through FUSE (userspace) drivers is acceptable, as long as
   there is also a kernel driver that can mount the same file system
   at least read-only. For example, this is the case for ntfs.

5. the following programs must be available:
   the three custom-made programs 'fsmove', 'fsmount_kernel' 'fsremap'
   (distributed with the script) and several common Linux tools:
      which, expr, id, blockdev, losetup, mount, umount,
      mkdir, rmdir, rm, mkfifo, dd, sync, fsck, mkfs


### KNOWN LIMITATIONS

1) As stated above, at a certain step during the conversion, fstransform needs
   to unmount the device being transformed.
   For this reason, running fstransform on the device currently mounted as /
   (i.e. the root directory) fails.
   For the same reason, running fstransform on the device currently mounted
   as /usr, /home or /var or similar heavily-used directories is difficult,
   because quite often there are programs using those, which prevents
   them from being unmounted.

2) If the device contains a HUGE number of files with multiple hard links,
   fstransform will be very slow and consume a LOT of memory.
   Devices with more than one million files with multiple hard links
   can cause fstransform to crash with "out of memory" errors.

3) JFS and NTFS file systems equal or larger than 8TB cannot be converted
   due to missing support for ioctl(FIEMAP) in the kernel:
   the fallback ioctl(FIBMAP) is limited by design to < 8TB (assuming 4k blocks)

   Also, ioctl(FIBMAP) must be called for _each_ block so the conversion
   will be a bit slower.

4) REISERFS file systems using format "3.5" (the default) and equal or larger than 2TB
   cannot be converted due to their maximum file size = 2TB - 4k:
   fstransform needs to create a sparse file as large as the device itself.

   REISERFS file systems using format "3.6" are immune to this problem.

5) for the same reason, a device cannot be converted _to_ REISERFS format "3.5"
   if it contains some files larger than 2TB - 4k.


### DETAILS TO KNOW

If the original device is almost full, the program 'fsremap'
will create a relatively small backup file ("secondary storage")
inside the directory /var/tmp/fstransform.
This secondary storage file will be at most as large as half your free RAM.

You can pass the option '-s <size>[k|M|G|T|P|E|Y|Z]' to the tool 'fsremap'
in order manually set the secondary storage size,
but please understand that using a too small secondary storage
can slow down the procedure.

To pass the same option to 'fstransform', you must execute something like
  fstransform --opts-fsremap='-s <size>' <other-options-and-arguments>


### PROCEDURE

0. compile fsmove, fsmount_kernel and fsremap.
   Running "./configure" then "make" should suffice on any recent Linux machine,
   as long as g++ is installed.

   You will get three executables, fsmove and fsremap.
   They will be located at
     ./fsmove/build/fsmove
     ./fsmount_kernel/build/fsmount_kernel
     ./fsremap/build/fsremap

   You are suggested to either run "make install" or to copy them to a simpler path.
   Below, they will be referred as {fsmove}, {fsmount_kernel} and {fsremap}


1. OPTIONAL - CAN BE SKIPPED
   mount read-write the device you want to remap to a new file-system type

   mount {device} {device-mount-point} [your-options]

   if the device is already mounted, check that it is mounted read-write
   and that no process is using it.

2. decide the target file-system type.

   For some combinations of the initial and final filesystems
   it is not necessary to use 'fstransform',
   as the same result can be obtained with much simpler - and SAFER - tools.

   For example, an 'ext2' or 'ext3' filesystem can be transformed into 'ext3'
   or 'ext4' using the program 'tune2fs'.

   Explaining how to use 'tune2fs' is beyond the scope of this document,
   just read its man-page or search on the Internet for one of
   "convert Linux File System ext2 to ext3"
   "convert Linux File System ext2 to ext3"
   "convert Linux File System ext3 to ext4"

   But for most combinations, the only way is either to do a full backup +
   format + restore the data, or use 'fstransform'

3. execute the program

     fstransform {device} {target-file-system-type}

   when converting _from_ NTFS, you must execute a slightly more verbose command:

     fstransform {device} {target-file-system-type} --current-fstype=ntfs

   because fstransform currently cannot autodetect FUSE-based file systems.

4. follow the instructions - the program will tell you what it is doing,
   and will also call 'fsmove' and 'fsremap' which show progress percentage
   and estimated time left.

   Note that 'fsmove' and 'fsremap' need approximately the same time to run,
   so if 'fsmove' tells you that it will need 2 hours, 'fsmove' will
   likely need a similar amount of time, for a total of 4 hours.

   In case there are errors, you can even try to fix them instead of
   aborting the execution (if you know what you are doing).

5. be PATIENT. Transforming a large device takes a LONG time...
   On a fairly fast disk, it takes about one minute per gigabyte.
   It means transforming 1000GB takes about 16 hours.
   Raid disks can be somewhat faster, and solid state disks (SSD)
   can be _much_ faster.

6) if something goes really wrong, check in /var/tmp/fstransform
   for the log files
   fstransform.log.<NNN> and fsremap.job.<MMM>/fsremap.log
   they are ABSOLUTELY necessary if you want someone to analyze the problem
   - but unless you are very lucky you can forget about recovering your data...

7) if for some reason the execution is interrupted while 'fsremap' is running,
   for example due to a power failure, it is possible to resume it
   by running 'fsremap --resume-job=<MMM> {device}'.
   Also, 'fsremap' will show at its startup the exact command line
   needed to resume its execution.

   The loop file created by fstransform must NEVER be  as argument to
   'fsremap --resume-job=<MMM> {...}'. You would IRREVERSIBLY LOSE YOUR DATA!

SOME REAL-WORLD TESTS

1) 1000GB encrypted disk, 52% full, ext2->ext4: SUCCESS, took 12 hours
   despite one system crash and two manual interruptions (CTRL+C)
   (Yes, I know ext2->ext4 can be done with tune2s, but I wanted to
   test fstransform)

2) 1540GB encrypted raid0 (3 disks), 56% full, ext2->ext4: SUCCESS, took 8 hours

3) 213GB disk, 85% full, ntfs->xfs: SUCCESS, took 12 hours
   (Yes, that's slow. But it works)


Good luck!

Massimiliano Ghilardi
