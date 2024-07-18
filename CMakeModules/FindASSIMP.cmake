include(FindPackageHandleStandardArgs)

# The assimp library is linked to zlib
FIND_PACKAGE(ZLIB)
FIND_LIBRARY(ASSIMP_LIBRARIES assimp)
FIND_PATH(ASSIMP_INCLUDE_DIRS assimp/mesh.h)

FIND_LIBRARY(ASSIMP_LIBRARIES NAMES assimp assimp-vc143-mt assimp-vc143-mtd assimp-vc142-mt assimp-vc142-mtd assimp-vc141-mt assimp-vc141-mtd assimp-vc140-mt assimp-vc140-mtd assimp-vc130-mt assimp-vc130-mtd)
FIND_PATH(ASSIMP_INCLUDE_DIRS assimp/mesh.h)

# We don't need to find zlib if we are on windows...
if(WIN32)
find_package_handle_standard_args(ASSIMP DEFAULT_MSG
    ASSIMP_LIBRARIES
    ASSIMP_INCLUDE_DIRS
)
else()
FIND_PACKAGE(ZLIB)
find_package_handle_standard_args(ASSIMP DEFAULT_MSG
    ASSIMP_LIBRARIES
    ASSIMP_INCLUDE_DIRS
   ZLIB_LIBRARIES
)
endif()

