/*
 * work.template.hh
 *
 *  Created on: Feb 28, 2011
 *      Author: max
 */
#include "first.hh"

#include <cerrno>         /* for errno, EFBIG              */
#include <cstdio>         /* for fprintf(), stdout, stderr */
#include <limits>         /* for std::numeric_limits<T>    */

#include "traits.hh"      /* for FT_TYPE_TO_UNSIGNED(T) macro            */
#include "vector.hh"      /* for ft_vector<T>                            */
#include "map.hh"         /* for ft_map<T>                               */
#include "fail.hh"        /* for ff_fail()                               */
#include "file_extent.hh" /* for ff_file_extents()                       */
#include "work.hh"        /* for ff_work_dispatch(), ft_work_ctx<T>, ft_work<T>  */


/* constructor. takes a reference to ctx.file_fds(), does NOT copy them! */
template<typename T>
ft_work_ctx<T>::ft_work_ctx(T dev_length, const ft_ctx & ctx)
    : fm_dev_length(dev_length), fm_file_fd(ctx.file_fds())
{ }

/** destructor. does nothing */
template<typename T>
ft_work_ctx<T>::~ft_work_ctx()
{ }



template<typename const_iter>
static void ff_map_show(const char * label, const_iter iter, const_iter end)
{
    if (iter != end) {
        fprintf(stdout, "# extents in %s\n# extent \t\t logical\t\tphysical\t      length\n", label);
        for (ft_size i = 0; iter != end; ++iter, ++i) {
#ifdef FT_HAVE_LONG_LONG
            fprintf(stdout, "%8lu\t%16llu\t%16llu\t%12llu\n", (unsigned long)i,
                    (unsigned long long)iter->second.fm_logical,
                    (unsigned long long)iter->first.fm_physical,
                    (unsigned long long)iter->second.fm_length);
#else
            fprintf(stdout, "%8lu\t%16lu\t%16lu\t%12lu\n", (unsigned long)i,
                    (unsigned long)iter->second.fm_logical,
                    (unsigned long)iter->first.fm_physical,
                    (unsigned long)iter->second.fm_length);
#endif /* FT_HAVE_LONG_LONG */
        }
        fprintf(stdout, "\n");
    } else {
        fprintf(stdout, "# no extents in %s\n", label);
    }
}


template<typename T>
static void ff_map_show(const char * label, const ft_vector<T> & vector)
{
    ff_map_show(label, vector.begin(), vector.end());
}

template<typename T>
static void ff_map_show(const char * label, const ft_map<T> & map)
{
    ff_map_show(label, map.begin(), map.end());
}

/**
 * default constructor.
 *
 * implementation note: compiler-generated default constructor would be OK,
 * but our (intentionally uncallable) copy constructor declaration inhibits its generation
 */
template<typename T>
ft_work<T>::ft_work()
    : loop_map(), loop_holes_map(), dev_map()
{ }


/**
 * destructor
 *
 * implementation note: compiler-generated destructor would be OK,
 * we implement it only for symmetry with default constructor
 */
template<typename T>
ft_work<T>::~ft_work()
{ }


/**
 * read extents from LOOP-FILE and ZERO-FILE and use them to fill ft_work<T>
 * return 0 if success, else error
 */
template<typename T>
int ft_work<T>::init(const ft_work_ctx<T> & ctx)
{
    T dev_length = ctx.dev_length();
    const int * file_fd = ctx.file_fds();

    /* accumulate sum in file_vector */
    ft_vector<T> file_vector;
    ft_size i;
    int err = 0;

    do {
        for (i = 0; i < ft_ctx::FC_FILE_COUNT && !err; i++) {
            /* ff_file_extents() appends into file_vector, does NOT overwrite it */
            if ((err = ff_file_extents(file_fd[i], file_vector)) == 0) {

                if (i == 0) {
                    /*
                     * sort file extents by fm_physical.
                     * needed by loop_map.append0() and loop_holes_map.complement0() below
                     */
                    file_vector.sort_by_physical();

                    /*
                     * compute LOOP-FILE map
                     * WARNING: this does not merge and does not check for merges
                     */
                    loop_map.append0(file_vector);
                    /* show LOOP-FILE extents sorted by physical */
                    ff_map_show(ft_ctx::fc_file_label[i], loop_map);

                    /*
                     * compute LOOP-FILE complement map
                     * WARNING: file_vector must be sorted by physical!
                     * WARNING: this does not merge and does not check for merges
                     */
                    loop_holes_map.complement0(file_vector, dev_length);
                    /* show LOOP-FILE complement sorted by physical */
                    ff_map_show("LOOP-FILE complement", loop_holes_map);

                }
            }
        }
        if (err != 0)
            break;

        /*
         * sort file extents by fm_physical.
         * needed by dev_map.complement0() below
         */
        file_vector.sort_by_physical();

        /*
         * compute DEVICE map
         * WARNING: assumes file_vector_sum is sorted by physical!
         * WARNING: this does not merge and does not check for merges
         *
         * we compute DEVICE map as all the extents
         * which are neither in LOOP-FILE nor in ZERO-FILE
         */
        dev_map.complement0(file_vector, dev_length);

        /* show DEVICE extents sorted by fm_physical */
        ff_map_show(ft_ctx::fc_dev_label, dev_map);


    } while(0);

    return err;
}

/** core of transformation algorithm */
template<typename T>
int ft_work<T>::run()
{
    return 0;
}


/**
 * call in sequence init() and run().
 * return 0 if success, else error.
 *
 * in case of error in init() it does not call run()
 */
template<typename T>
int ft_work<T>::work(const ft_work_ctx<T> & ctx)
{
    int err = init(ctx);
    if (err == 0)
        err = run();
    return err;
}

/**
 * instantiate and call ft_work<T>::work()
 * passing device length and file descriptors
 */
template<typename T>
static int ff_work(T dev_length, const ft_ctx & ctx)
{
    ft_work_ctx<T> worker_ctx(dev_length, ctx);
    ft_work<T> worker;
    return worker.work(worker_ctx);
}


/** return true if dev_length can be stored in type T, else false */
template<typename T>
static bool ff_check(ft_off dev_length)
{
    typedef typename FT_TYPE_TO_UNSIGNED(ft_off) ft_off_unsigned;
    typedef typename FT_TYPE_TO_UNSIGNED(T)      T_unsigned;

    /* avoid comparison between signed and unsigned */
    if ((ft_off_unsigned) dev_length > (T_unsigned) std::numeric_limits<T>::max()) {
        /* overflow! we cannot represent device length - and extents! - with current choice of 'T' */
        return false;
    }
    return true;
}

/**
 * instantiate and run ft_work<T>::work() with the smallest T that can represent device length.
 * return 0 if success, else error.
 *
 * implementation:
 * iterates on all known types T and, if ff_check<T>() succeeds,
 * calls ff_work<T>() passing to it device length and file descriptors
 */
int ff_work_dispatch(const ft_ctx & ctx)
{
    ft_off dev_length = ctx.dev_length();
    int err;

    /** use the smallest type that can represent device length */

    if (ff_check<unsigned short>(dev_length))
        // possibly narrowing cast is safe here: we just checked for overflow
        err = ff_work<unsigned short>((unsigned short) dev_length, ctx);

    else if (ff_check<unsigned int>(dev_length))
        // possibly narrowing cast is safe here: we just checked for overflow
        err = ff_work<unsigned int>((unsigned int) dev_length, ctx);

    else if (ff_check<unsigned long>(dev_length))
        // possibly narrowing cast is safe here: we just checked for overflow
        err = ff_work<unsigned long>((unsigned long) dev_length, ctx);

#ifdef FT_HAVE_LONG_LONG
    else if (ff_check<unsigned long long>(dev_length))
        // possibly narrowing cast is safe here: we just checked for overflow
        err = ff_work<unsigned long long>((unsigned long long) dev_length, ctx);
#endif /* FT_HAVE_LONG_LONG */

    else
        err = ff_fail(EFBIG, "%s length is too large! cannot represent it with any known type", ft_ctx::fc_dev_label);

    return err;
}
