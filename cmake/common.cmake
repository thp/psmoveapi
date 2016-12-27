# Some platform-specific but target-agnostic settings

if (NOT MSVC)
    if (${CMAKE_VERSION} VERSION_LESS "3.1")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
    else()
        set(CMAKE_C_STANDARD 11)
        set(CMAKE_CXX_STANDARD 11)
    endif()
endif()

# We recommend using MinGW-w64 for the Windows builds which generates
# position-independent code by default, so skip this for Windows builds.
IF(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    # https://github.com/thp/psmoveapi/issues/29
    add_definitions(-fPIC)
ENDIF()

# Required by pthread, must be defined before include of any standard header.
# It's easier to do this here once instead of multiple times in different source/header files.
IF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    add_definitions(-D_GNU_SOURCE)
ENDIF()

# Prevent windows.h from automatically including a bunch of header files
IF(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    add_definitions(-DWIN32_LEAN_AND_MEAN)
ENDIF()

IF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    # Build Universal Binaries for OS X
    set(CMAKE_OSX_ARCHITECTURES "x86_64")
ENDIF()

# Windows' math include does not define constants by default.
# Set this definition so it does.
# Also set NOMINMAX so the min and max functions are not overwritten with macros.
IF(MSVC)
    add_definitions(-D_USE_MATH_DEFINES)
    add_definitions(-DNOMINMAX)
ENDIF()

macro(AddFlag flags warning)
	if (NOT ${${flags}} MATCHES ${warning})
		set(${flags} "${${flags}} ${warning}")
	endif()
endmacro()

# Fix compiler warnings
if(MSVC)
  # Force to always compile with W4 (C++)
  if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
    string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  elseif(CMAKE_CXX_FLAGS MATCHES "/Wall")
    string(REGEX REPLACE "/Wall" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
  endif()
  
  # Force to always compile with W4 (C)
  if(CMAKE_C_FLAGS MATCHES "/W[0-4]")
    string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
  elseif(CMAKE_C_FLAGS MATCHES "/Wall")
    string(REGEX REPLACE "/Wall" "/W4" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
  else()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W4")
  endif()
  
  # Enable debug (symbol) information for all builds
  AddFlag(CMAKE_CXX_FLAGS "/Zi")
  AddFlag(CMAKE_C_FLAGS "/Zi")
  AddFlag(CMAKE_EXE_LINKER_FLAGS "/debug")
  AddFlag(CMAKE_MODULE_LINKER_FLAGS "/debug")  
  AddFlag(CMAKE_SHARED_LINKER_FLAGS "/debug")  
  
  # Disable some warnings that are not important (C++)
  AddFlag(CMAKE_CXX_FLAGS "/wd4100") #unreferenced formal parameter
  AddFlag(CMAKE_CXX_FLAGS "/wd4115") #named type definition in parentheses
  AddFlag(CMAKE_CXX_FLAGS "/wd4201") #nonstandard extension used : nameless struct/union
  AddFlag(CMAKE_CXX_FLAGS "/wd4131") #uses old-style declarator
  AddFlag(CMAKE_CXX_FLAGS "/wd4204") #nonstandard extension used : non-constant aggregate initializer
  AddFlag(CMAKE_CXX_FLAGS "/wd4206") #nonstandard extension used : translation unit is empty
  AddFlag(CMAKE_CXX_FLAGS "/wd4512") #assignment operator could not be generated
  AddFlag(CMAKE_CXX_FLAGS "/wd4200") #nonstandard extension used : zero-sized array in struct/union
  AddFlag(CMAKE_CXX_FLAGS "/wd4510") #default constructor could not be generated
  AddFlag(CMAKE_CXX_FLAGS "/wd4610") #struct 'libusb_version' can never be instantiated - user defined constructor required
  
  # Disable some warnings that are not important (C)
  AddFlag(CMAKE_C_FLAGS "/wd4100") #unreferenced formal parameter
  AddFlag(CMAKE_C_FLAGS "/wd4115") #named type definition in parentheses
  AddFlag(CMAKE_C_FLAGS "/wd4201") #nonstandard extension used : nameless struct/union
  AddFlag(CMAKE_C_FLAGS "/wd4131") #uses old-style declarator
  AddFlag(CMAKE_C_FLAGS "/wd4204") #nonstandard extension used : non-constant aggregate initializer
  AddFlag(CMAKE_C_FLAGS "/wd4206") #nonstandard extension used : translation unit is empty
  AddFlag(CMAKE_C_FLAGS "/wd4512") #assignment operator could not be generated
  AddFlag(CMAKE_C_FLAGS "/wd4200") #nonstandard extension used : zero-sized array in struct/union
  AddFlag(CMAKE_C_FLAGS "/wd4510") #default constructor could not be generated
  AddFlag(CMAKE_C_FLAGS "/wd4610") #struct 'libusb_version' can never be instantiated - user defined constructor required
  
  add_definitions(-D_CRT_SECURE_NO_DEPRECATE -D_CRT_NONSTDC_NO_DEPRECATE)
elseif(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
  # Update if necessary
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-long-long")
endif()

# Enable solution folders for msvc
IF(MSVC)
	set_property(GLOBAL PROPERTY USE_FOLDERS ON)
ENDIF()

# Pretty-print if a "use" feature has been enabled
macro(feature_use_info CAPTION FEATURE)
    if(${FEATURE})
        message("    " ${CAPTION} "Yes")
    else()
        message("    " ${CAPTION} "No")
    endif()
endmacro()

LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}")
