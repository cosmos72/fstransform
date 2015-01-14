m4_define([FT_CXX_FEATURES],
  [
  AC_CACHE_CHECK([whether $CXX supports explicit template instantiation],
    [ac_cv_cxx_have_template_instantiation],
    [AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
      template<typename T>
        class ft_my_class {
          public:
            void apply(T *);
        };
      
      template<typename T>
        void ft_my_class<T>::apply(T * arg)
        { }
      
      template class ft_my_class<int>;
    ]], [[
      ft_my_class<int> dummy;
    ]])],
    [ac_cv_cxx_have_template_instantiation=yes],
    [ac_cv_cxx_have_template_instantiation=no]
    )
  ])
  
  AC_CACHE_CHECK([whether $CXX supports inhibiting template instantiation],
    [ac_cv_cxx_have_template_inhibition],
    [AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
      template<typename T>
        class ft_my_class {
          public:
            void apply(T *);
        };
      extern template class ft_my_class<int>;
    ]], [[
      ft_my_class<int> dummy;
    ]])],
    [ac_cv_cxx_have_template_inhibition=yes],
    [ac_cv_cxx_have_template_inhibition=no]
   )
 ])
 
 if test "$ac_cv_cxx_have_template_instantiation" = yes -a "$ac_cv_cxx_have_template_inhibition" = yes; then
   AC_DEFINE([HAVE_EXTERN_TEMPLATE], [1],
     [define if C++ compiler supports forcing and inhibiting template instantiation])
 fi
])
