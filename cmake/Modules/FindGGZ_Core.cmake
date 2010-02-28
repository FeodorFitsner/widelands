FIND_PATH(GGZ_CORE_INCLUDE_DIR ggzmod.h)

FIND_LIBRARY(GGZ_CORE_LIBRARY NAMES ggzmod) 

IF (GGZ_CORE_INCLUDE_DIR AND GGZ_CORE_LIBRARY)
   SET(GGZ_CORE_FOUND TRUE)
ENDIF (GGZ_CORE_INCLUDE_DIR AND GGZ_CORE_LIBRARY)

IF (GGZ_CORE_FOUND)
   IF (NOT GGZ_Core_FIND_QUIETLY)
      MESSAGE(STATUS "Found GGZ_Core: ${GGZ_CORE_LIBRARY}")
   ENDIF (NOT GGZ_Core_FIND_QUIETLY)
ELSE (GGZ_CORE_FOUND)
   IF (GGZ_Core_FIND_REQUIRED)
      MESSAGE(FATAL_ERROR "Could not find GGZ_Core")
   ENDIF (GGZ_Core_FIND_REQUIRED)
ENDIF (GGZ_CORE_FOUND)