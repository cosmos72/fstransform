/*
 * fsattr - modify file-system internal data structures
 *
 * Copyright (C) 2012 Massimiliano Ghilardi
 *
 *     This program is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation, version 2 of the License.
 *
 *     This program is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 *
 *     You should have received a copy of the GNU General Public License
 *     along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * e4attr.cc
 *
 *  Created on: Apr 14, 2012
 *      Author: max
 */
#include "first.hh"
#include "types.hh"

#if defined(FT_HAVE_ERRNO_H)
# include <errno.h>        // for errno
#elif defined(FT_HAVE_CERRNO)
# include <cerrno>         // for errno
#endif
#if defined(FT_HAVE_STDLIB_H)
# include <stdlib.h>       // for malloc(), free()
#elif defined(FT_HAVE_CSTDLIB)
# include <cstdlib>        // for malloc(), free()
#endif
#if defined(FT_HAVE_STRING_H)
# include <string.h>       // for memcmp(), strncmp()
#elif defined(FT_HAVE_CSTRING)
# include <cstring>        // for memcmp(), strncmp()
#endif

#if defined (FT_HAVE_EXT2FS_EXT2FS_H)
# include <ext2fs/ext2fs.h>
#endif

# include "log.hh"

FT_NAMESPACE_BEGIN

#if defined(FT_HAVE_LIBEXT2FS) && defined(FT_HAVE_LIBCOM_ERR)

typedef errcode_t e4_err;
typedef ext2_filsys e4_fs;
typedef ext2_ino_t e4_inum;
typedef ext2_inode e4_inode;
typedef ext2_dir_entry e4_dir_iter_;
typedef ext2_dir_entry_2 e4_dir_iter;
typedef ext2_extent_handle_t e4_extnum;
typedef struct ext2fs_extent e4_extent;

enum e4attr_extent_op { E4_EXTENT_UNKNOWN_OP, E4_EXTENT_SET_UNINITIALIZED, E4_EXTENT_SET_INITIALIZED };

struct e4attr_ctx
{
    const char * dev_path;
    e4_fs fs;
    e4_err err;
    e4attr_extent_op apply_op;
};

#define e4_fail(err, fmt, ...) ff_log(FC_ERROR, 0, fmt ": %s", __VA_ARGS__, error_message(err))

static e4_err e4attr_extents_apply(e4_fs fs, e4_inum inum, e4attr_extent_op apply_op)
{
    e4_extnum extnum;
    e4_extent extent;
    e4_err err = ext2fs_extent_open(fs, inum, &extnum);
    if (err != 0) {
        e4_fail(err, "ext2fs_extent_open(inode = #%" FT_ULL ") failed", (ft_ull) inum);
        return err;
    }
    int op = EXT2_EXTENT_ROOT;
    bool init = apply_op == E4_EXTENT_SET_INITIALIZED;
    const ft_ull max_len = init ? EXT_INIT_MAX_LEN : EXT_UNINIT_MAX_LEN;

    for (;;) {
        if ((err = ext2fs_extent_get(extnum, op, &extent)) != 0) {
            if (err == EXT2_ET_EXTENT_NO_NEXT)
                err = 0;
            else
                e4_fail(err, "ext2fs_extent_get(inode = #%" FT_ULL ", extent = #%" FT_ULL ") failed", (ft_ull) inum, (ft_ull) extnum);
            break;
        }
        op = EXT2_EXTENT_NEXT;

        if ((extent.e_flags & EXT2_EXTENT_FLAGS_SECOND_VISIT) || !(extent.e_flags & EXT2_EXTENT_FLAGS_LEAF))
            continue;
        if (init && !(extent.e_flags & EXT2_EXTENT_FLAGS_UNINIT))
            continue;
        if (!init && (extent.e_flags & EXT2_EXTENT_FLAGS_UNINIT))
            continue;

        if (extent.e_len > max_len) {
            ff_log(FC_ERROR, 0, "cannot %sinitialize extent %" FT_ULL " - %" FT_ULL " (%" FT_ULL " blocks) in inode #%" FT_ULL
                   ": maximum %sinitialized extent length is %" FT_ULL " blocks", (init ? "" : "un"),
                   (ft_ull) extent.e_lblk, (ft_ull) (extent.e_lblk + (extent.e_len - 1)),
                   (ft_ull) extent.e_len, (ft_ull) inum, (init ? "" : "un"), (ft_ull) max_len);
            err = -EOVERFLOW;
            break;
        }

        extent.e_flags ^= EXT2_EXTENT_FLAGS_UNINIT;
        if ((err = ext2fs_extent_replace(extnum, 0, &extent)) != 0) {
            e4_fail(err, "ext2fs_extent_replace(inode = #%" FT_ULL ", extent = #%" FT_ULL ") failed", (ft_ull) inum, (ft_ull) extnum);
            break;
        }
    }
    ext2fs_extent_free(extnum);

    return err;
}

static int e4attr_dir_iterate(e4_inum dir, int entry, e4_dir_iter_ * iter_, int offset, int blocksize, char * buf, void * priv_data)
{
    e4attr_ctx * ctx = static_cast<e4attr_ctx *>(priv_data);
    e4_fs fs = ctx->fs;
    e4_err & err = ctx->err;
    e4_dir_iter * iter = reinterpret_cast<e4_dir_iter *>(iter_);
    e4_inum inum = iter->inode;
    int name_len = (int)(unsigned char) iter->name_len;
    const char * descr;
    bool is_dir = false, is_file = false;

    // suppress "unused parameter" warnings
    (void)entry, (void)offset, (void)blocksize, (void)buf;

    switch (iter->file_type) {
    default:
    case EXT2_FT_UNKNOWN:
        if ((is_dir = (ext2fs_check_directory(fs, inum) == 0)))
            descr = "(dir)";
        else
            // avoid trigraph warning
            descr = "(" "???" ")";
        break;
    case EXT2_FT_REG_FILE: descr = "(file)"; is_file = true; break;
    case EXT2_FT_DIR:      descr = "(dir)";  is_dir = true; break;
    case EXT2_FT_CHRDEV:   descr = "(chr)";  break;
    case EXT2_FT_BLKDEV:   descr = "(blk)";  break;
    case EXT2_FT_FIFO:     descr = "(fifo)"; break;
    case EXT2_FT_SOCK:     descr = "(sock)"; break;
    case EXT2_FT_SYMLINK:  descr = "(slnk)"; break;
    }
    ff_log(FC_DEBUG, 0, "%" FT_ULL "\t->%" FT_ULL "\t%s\t%.*s", (ft_ull) dir, (ft_ull) inum, descr, name_len, iter->name);

    if (is_file) {
       err = e4attr_extents_apply(fs, inum, ctx->apply_op);

       // skip "." and ".."
    } else if (is_dir && inum != dir && (name_len != 2 || memcmp(iter->name, "..", 2))) {
        char * buf2 = (char *) malloc(fs->blocksize);
        if (buf2 == 0) {
            err = ff_log(FC_ERROR, errno, "malloc(%" FT_ULL ") failed", (ft_ull) fs->blocksize);
            return 1;
        }
        if ((err = ext2fs_dir_iterate2(fs, inum, 0, buf2, e4attr_dir_iterate, priv_data)) != 0)
            e4_fail(err, "ext2fs_dir_iterate2(..., dir_inum = #%" FT_ULL ") failed", (ft_ull) inum);
        free(buf2);
    }
    return err ? 1 : 0;
}


static e4_err e4attr_run(const char * dev_path, e4attr_extent_op apply_op) {
    e4attr_ctx ctx;
    char * buf = NULL;
    // harcoded?
    const e4_inum root_inum = 2;
    bool fs_is_open = false;

    ctx.dev_path = dev_path;
    ctx.apply_op = apply_op;

    e4_fs & fs = ctx.fs;
    e4_err & err = ctx.err = 0;

    initialize_ext2_error_table();

    do {
        if ((err = ext2fs_open(dev_path, EXT2_FLAG_RW, 0, 0, unix_io_manager, & fs)) != 0) {
            e4_fail(err, "ext2fs_open(%s) failed", dev_path);
            break;
        }
        fs_is_open = true;

        if ((err = ext2fs_check_directory(fs, root_inum)) != 0) {
            e4_fail(err, "inode #%" FT_ULL " inside device '%s' is not a directory - but it is supposed to be the root directory",
                    dev_path, (ft_ull) root_inum);
            break;
        }
        buf = (char *) malloc(fs->blocksize);
        if (buf == 0) {
            err = ff_log(FC_ERROR, errno, "malloc(%" FT_ULL ") failed", (ft_ull) fs->blocksize);
            break;
        }
        if ((err = ext2fs_dir_iterate2(fs, root_inum, 0, buf, e4attr_dir_iterate, static_cast<void *>(& ctx))) != 0)
            e4_fail(err, "ext2fs_dir_iterate2(%s, root_inum = #%" FT_ULL ") failed", dev_path, (ft_ull) root_inum);
    } while (0);

    if (buf)
        free(buf);
    if (fs_is_open && (err = ext2fs_close(fs)) != 0)
        e4_fail(err, "ext2fs_close(%s) failed", dev_path);

    return err;
}


/** print command-line usage to stdout and return 0 */
static int e4attr_help(const char * program_name) {
    ff_log(FC_NOTICE, 0, "Usage: %s --files={normal|prealloc} DEVICE", program_name);
    ff_log(FC_NOTICE, 0, "Modify file-system internal data structures");

    return ff_log(FC_NOTICE, 0,
     "Options:\n"
     "  --               end of options. treat subsequent parameters as arguments\n"
     "                     even if they start with '-'\n"
     "  --files=normal   set files to normal mode, i.e. show their contents.\n"
     "  --files=prealloc set files to 'preallocated', i.e. clear their contents.\n"
     "  --fstype=FSTYPE  specify file system inside device.\n"
     "  --help           display this help and exit\n"
     "  --version        output version information and exit");
}


static int e4attr_version() {
    return ff_log(FC_NOTICE, 0,
        "fsattr (fstransform utilities) " FT_VERSION "\n"
        "Copyright (C) 2012-2017 Massimiliano Ghilardi\n"
        "\n"
        "License GPLv2: GNU GPL version 2\n"
        "<http://www.gnu.org/licenses/old-licenses/gpl-2.0.html>.\n"
        "This is free software: you are free to change and redistribute it.\n"
        "There is NO WARRANTY, to the extent permitted by law.");
}

static int e4attr_usage(const char * program_name) {
    ff_log(FC_NOTICE, 0, "Try %s --help' for more information.", program_name);
    return 1;
}

int e4attr_main(int argc, char ** argv) {
    const char * program_name = argv[0], * dev_path = NULL;
    e4attr_extent_op apply_op = E4_EXTENT_UNKNOWN_OP;
    bool allow_opts = true;

    while (--argc) {
        char * arg = * ++argv;
        if (allow_opts && arg[0] == '-') {
            if (!strcmp(arg, "--"))
                allow_opts = false;
            else if (!strcmp(arg, "--help"))
                return e4attr_help(program_name);
            else if (!strcmp(arg, "--version"))
                return e4attr_version();
            else if (!strcmp(arg, "--files=prealloc"))
                apply_op = E4_EXTENT_SET_UNINITIALIZED;
            else if (!strcmp(arg, "--files=normal"))
                apply_op = E4_EXTENT_SET_INITIALIZED;
            else if (!strncmp(arg, "--fstype=", 9)) {
                if (strcmp(arg + 9, "ext4")) {
                    ff_log(FC_ERROR, 0, "%s: option '%s' specified, but only supported FSTYPE value is 'ext4'", program_name, arg);
                    return 1;
                }
            } else {
                ff_log(FC_ERROR, 0, "%s: invalid option '%s'", program_name, arg);
                return e4attr_usage(program_name);
            }
        } else
            dev_path = arg;
    }
    if (dev_path == NULL) {
        ff_log(FC_ERROR, 0, "%s: missing argument DEVICE", program_name);
        return e4attr_usage(program_name);
    }
    if (apply_op == E4_EXTENT_UNKNOWN_OP) {
        ff_log(FC_ERROR, 0, "%s: missing operation, please specify one of --files=normal or --files=prealloc", program_name);
        return e4attr_usage(program_name);
    }

    e4_err err = ft::e4attr_run(dev_path, apply_op);
    return err != 0 ? 1 : 0;
}

#else /* !(defined(FT_HAVE_LIBEXT2FS) && defined(FT_HAVE_LIBCOM_ERR)) */

int e4attr_main(int argc, char ** argv) {
    const char * program_name = argv[0];

    ff_log(FC_ERROR, 0, "%s: this program is a NOT functional because it was compiled without -lext2fs -lcom_err", program_name);
    return 1;
}

#endif /* defined(FT_HAVE_LIBEXT2FS) && defined(FT_HAVE_LIBCOM_ERR) */

FT_NAMESPACE_END

