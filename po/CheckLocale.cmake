file (GLOB_RECURSE pofiles ${pobasedir}/${lang}.po)

foreach (pofile ${pofiles})
  get_filename_component(popath ${pofile} PATH)
  string(REPLACE "${pobasedir}/" "" templatename ${popath})
  set (mofile ${localebasedir}/${lang}/LC_MESSAGES/${templatename}.mo)
  find_path(trans ${templatename}.mo ${localebasedir}/${lang}/LC_MESSAGES)
  if (trans STREQUAL "trans-NOTFOUND")
    message(FATAL_ERROR "The translation for locale ${lang} is missing for the template ${templatename}. You need to run cmake again to pick up this translation.")
  endif (trans STREQUAL "trans-NOTFOUND")
endforeach (pofile ${pofiles})