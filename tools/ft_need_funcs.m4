m4_define([FT_NEED_ALL_FUNCS], [
  ft_funcs_missing=
  for ft_func in $@
  do :
    if test "`eval echo '$ac_cv_func_'$ft_func`" != "yes"
    then
      ft_funcs_missing="$ft_funcs_missing$ft_func "
    fi
  done
  if test "x$ft_funcs_missing" != "x"
  then
    as_fn_error $? "missing required functions: $ft_funcs_missing"
  fi
])

m4_define([FT_NEED_ANY_FUNC], [
  ft_funcs_missing=
  ft_funcs_found=
  for ft_func in $@
  do :
    ft_funcs_missing="$ft_funcs_missing$ft_func "

    if test "`eval echo '$ac_cv_func_'$ft_func`" = "yes"
    then
      ft_funcs_found=1
    fi
  done
  if test "x$ft_funcs_found" = "x"
  then
    as_fn_error $? "least one of the following functions is required: $ft_funcs_missing"
  fi
])

