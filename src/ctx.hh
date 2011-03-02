/*
 * ctx.hh
 *
 *  Created on: Feb 28, 2011
 *      Author: max
 */

#ifndef FSTRANSFORM_CTX_HH
#define FSTRANSFORM_CTX_HH

#include "types.hh"    /* for ft_off */

class ft_ctx
{
public:
    enum { FC_FILE_COUNT = 2 };

    static const char * fc_dev_label;
    static const char * const fc_file_label[FC_FILE_COUNT];

private:
    /* cannot call copy constructor */
    ft_ctx(const ft_ctx &);

    /* cannot call assignment operator */
    const ft_ctx & operator=(const ft_ctx &);

protected:
    ft_off fm_dev_length;
    int fm_file_fd[FC_FILE_COUNT];

public:
    /** default constructor */
    ft_ctx();

    /** destructor. calls close() */
    ~ft_ctx();

    /** return true if this ft_ctx is currently (and correctly) open */
    bool is_open() const;

    /** close file descriptors */
    void close();

    /** return device length */
    FT_INLINE const ft_off dev_length() const { return fm_dev_length; }

    /** return file descriptors */
    FT_INLINE const int * file_fds() const { return fm_file_fd; }
};



#endif /* FSTRANSFORM_CTX_HH */
