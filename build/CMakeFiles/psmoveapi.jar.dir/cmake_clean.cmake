FILE(REMOVE_RECURSE
  "psmovePYTHON_wrap.c"
  "psmove.py"
  "psmoveJAVA_wrap.c"
  "CMakeFiles/psmoveapi.jar"
)

# Per-language clean rules from dependency scanning.
FOREACH(lang)
  INCLUDE(CMakeFiles/psmoveapi.jar.dir/cmake_clean_${lang}.cmake OPTIONAL)
ENDFOREACH(lang)
