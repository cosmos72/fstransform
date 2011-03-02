/*
 * work.template.hh
 *
 *  Created on: Feb 28, 2011
 *      Author: max
 */
#include "first.hh"

#include <cerrno>         // for errno, EFBIG              */
#include <cstdio>         // for fprintf(), stdout, stderr */
#include <limits>         // for std::numeric_limits<T>    */

#include "traits.hh"      // for FT_TYPE_TO_UNSIGNED(T) macro
#include "vector.hh"      // for ft_vector<T>
#include "map.hh"         // for ft_map<T>
#include "fail.hh"        // for ff_fail()
#include "io/io.hh"       // for ft_io
#include "work.hh"        // for ff_work_dispatch(), ft_work_ctx<T>, ft_work<T>

FT_NAMESPACE_BEGIN

/** constructor. stores a reference to ft_io in this ft_work_ctx */
template<typename T>
ft_work_ctx<T>::ft_work_ctx(T dev_length, FT_IO_NS ft_io & io)
    : fm_dev_length(dev_length), fm_io(io)
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



/** default constructor */
template<typename T>
ft_work<T>::ft_work()
    : dev_map(), loop_map(), ctx(NULL)
{ }


/** destructor. calls quit() */
template<typename T>
ft_work<T>::~ft_work()
{
    quit();
}


/**
 * high-level do-everything method. calls in sequence init(), run() and quit().
 * return 0 if success, else error.
 */
template<typename T>
int ft_work<T>::main(ft_work_ctx<T> & work_ctx)
{
    ft_work<T> worker;
    int err = worker.init(work_ctx);
    if (err == 0)
        err = worker.run();

    // worker.quit() is called automatically by destructor, no need to call explicitly

    return err;
}

/** return true if this ft_work is currently (and correctly) initialized */
template<typename T>
bool ft_work<T>::is_initialized()
{
    return ctx != NULL && ctx->is_open();
}


/**
 * read extents from LOOP-FILE and ZERO-FILE and use them to fill ft_work<T>
 * return 0 if success, else error
 */
template<typename T>
int ft_work<T>::init(ft_work_ctx<T> & work_ctx)
{
    if (is_initialized()) {
        ff_fail(0, "I/O already initialized");
        return EISCONN;
    }
    // cleanup in case ctx != NULL or dev_map, loop_map are not empty
    quit();

    FT_IO_NS ft_io & io = work_ctx.io();

    int err = 0;

    do {
        if ((err = io.loop_file_extents(loop_map)) != 0)
            break;
        /* show LOOP-FILE extents sorted by physical */
        ff_map_show(FT_IO_NS ft_io::label[FT_IO_NS ft_io::FC_LOOP_FILE], loop_map);


        if ((err = io.device_extents(loop_map, dev_map)) != 0)
            break;
        /* show DEVICE extents sorted by fm_physical */
        ff_map_show(FT_IO_NS ft_io::label[FT_IO_NS ft_io::FC_DEVICE], dev_map);

    } while(0);

    if (err == 0)
        ctx = & work_ctx;
    else
        quit(); // clear dev_map and loop_map

    return err;
}

/** core of transformation algorithm */
template<typename T>
int ft_work<T>::run()
{
    return 0;
}


/** performs cleanup. called by destructor, you can also call it explicitly after (or instead of) run()  */
template<typename T>
void ft_work<T>::quit()
{
    dev_map.clear();
    loop_map.clear();
    ctx = NULL; // do not delete work_ctx<T>, we did not create it!
}


/**
 * create ft_work_ctx<T>
 * and call ft_work<T>::main()
 * passing created ft_work_ctx
 */
template<typename T>
static int ff_work(T dev_length, FT_IO_NS ft_io & io)
{
    ft_work_ctx<T> work_ctx(dev_length, io);

    return ft_work<T>::main(work_ctx);
}


/** return true if dev_length can be stored in type T, else false */
template<typename T>
static bool ff_check(ft_uoff dev_length)
{
    if ((T) dev_length < 0 || dev_length != (ft_uoff)(T) dev_length) {
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
int ff_work_dispatch(FT_IO_NS ft_io & io)
{
    ft_off dev_length = io.dev_length();
    int err;

    /** use the smallest type that can represent device length */

    if (ff_check<ft_u32>(dev_length))
        // possibly narrowing cast is safe here: we just checked for overflow
        err = ff_work<ft_u32>((ft_u32) dev_length, io);

    else if (ff_check<ft_u64>(dev_length))
        // possibly narrowing cast is safe here: we just checked for overflow
        err = ff_work<ft_u64>((ft_u64) dev_length, io);

    else
        err = ff_fail(EFBIG, "device length is too large! cannot represent it with any known type");

    return err;
}

FT_NAMESPACE_END
