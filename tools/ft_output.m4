m4_define([FT_OUTPUT], [[
  sed -e 's/define \([A-Z]\)/define FT_\1/g' -e 's/undef \([A-Z]\)/undef FT_\1/g' < fsremap/src/config.hh > fsremap/src/ft_config.hh
]])
