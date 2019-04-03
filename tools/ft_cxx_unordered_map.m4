m4_define([FT_CXX_STD_UNORDERED_MAP],
  [AC_CACHE_CHECK([whether C++ library implements std::unordered_map<K,V>],
    [ac_cv_cxx_have_std_unordered_map],
    [AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#ifdef HAVE_UNORDERED_MAP
#include <unordered_map>
#endif
      ]], [[
        std::unordered_map<int,int> my_map;
      ]])],
      [ac_cv_cxx_have_std_unordered_map=yes],
      [ac_cv_cxx_have_std_unordered_map=no])])

  if test "$ac_cv_cxx_have_std_unordered_map" = yes; then
    AC_DEFINE([HAVE_STD_UNORDERED_MAP], [1],
    [define if C++ library implements std::unordered_map<K,V>])
  fi]
)


m4_define([FT_CXX_STD_TR1_UNORDERED_MAP],
  [AC_CACHE_CHECK([whether C++ library implements std::tr1::unordered_map<K,V>],
    [ac_cv_cxx_have_std_tr1_unordered_map],
    [AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#ifdef HAVE_TR1_UNORDERED_MAP
#include <tr1/unordered_map>
#endif
      ]], [[
        std::tr1::unordered_map<int,int> my_map;
      ]])],
      [ac_cv_cxx_have_std_tr1_unordered_map=yes],
      [ac_cv_cxx_have_std_tr1_unordered_map=no])])

  if test "$ac_cv_cxx_have_std_tr1_unordered_map" = yes; then
    AC_DEFINE([HAVE_STD_TR1_UNORDERED_MAP], [1],
    [define if C++ library implements std::tr1::unordered_map<K,V>])
  fi]
)
