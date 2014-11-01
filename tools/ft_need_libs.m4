m4_define([FT_NEED_ALL_LIBS], [
  ft_libs_missing=
  for ft_lib in $@
  do :
    if test "`eval echo '$ac_cv_lib_'$ft_lib`" != "yes"
    then
      ft_libs_missing="${ft_libs_missing}lib$(echo $ft_lib | sed 's/_/:/')() "
    fi
  done
  if test "x$ft_libs_missing" != "x"
  then
    as_fn_error "missing required library functions: $ft_libs_missing"
  fi
])
