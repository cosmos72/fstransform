m4_define([FT_CXX_FLAGS],
  [
  AC_CACHE_CHECK([whether $CXX accepts -Wall],
    [ac_cv_cxx_flag_Wall],
    [save_CXXFLAGS="$CXXFLAGS"
     CXXFLAGS="$CXXFLAGS -Wall"
     AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
    ]], [[
      return 0;
    ]])],
    [ac_cv_cxx_flag_Wall=yes],
    [ac_cv_cxx_flag_Wall=no
     CXXFLAGS="$save_CXXFLAGS"]
    )
  ])
  
  AC_CACHE_CHECK([whether $CXX accepts -Wextra],
    [ac_cv_cxx_flag_Wextra],
    [save_CXXFLAGS="$CXXFLAGS"
     CXXFLAGS="$CXXFLAGS -Wextra"
     AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
    ]], [[
      return 0;
    ]])],
    [ac_cv_cxx_flag_Wextra=yes],
    [ac_cv_cxx_flag_Wextra=no
     CXXFLAGS="$save_CXXFLAGS"]
   )
 ])

  AC_CACHE_CHECK([whether $CXX accepts -W],
    [ac_cv_cxx_flag_W],
    [save_CXXFLAGS="$CXXFLAGS"
     CXXFLAGS="$CXXFLAGS -W"
     AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
    ]], [[
      return 0;
    ]])],
    [ac_cv_cxx_flag_W=yes],
    [ac_cv_cxx_flag_W=no
     CXXFLAGS="$save_CXXFLAGS"]
   )
 ])
])
