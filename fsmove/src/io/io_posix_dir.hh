/*
 * io_posix_dir.hh
 *
 *  Created on: Sep 22, 2011
 *      Author: max
 */

#ifndef FSMOVE_IO_IO_POSIX_DIR_HH
#define FSMOVE_IO_IO_POSIX_DIR_HH

#include "../types.hh"    // for ft_string

#ifdef FT_HAVE_DIRENT_H
# include <dirent.h>       // for DIR
#endif

FT_IO_NAMESPACE_BEGIN


typedef struct dirent fm_io_posix_dirent;


class fm_io_posix_dir
{
private:
    ft_string this_path;
    DIR * this_dir;

    /** cannot call copy constructor */
    fm_io_posix_dir(const fm_io_posix_dir &);

    /** cannot call assignment operator */
    const fm_io_posix_dir & operator=(const fm_io_posix_dir &);

public:
    /** default constructor */
    fm_io_posix_dir();

    /** destructor, calls close() */
    ~fm_io_posix_dir();

    /** open a directory */
    int open(const ft_string & path);

    /**
     *  get next directory entry.
     *  returns 0 if success (NULL result indicates EOF),
     *  else returns error code
     */
    int next(fm_io_posix_dirent * & result);

    /** close the currently open directory */
    int close();
};

FT_IO_NAMESPACE_END

#endif /* FSMOVE_IO_IO_POSIX_DIR_HH */
