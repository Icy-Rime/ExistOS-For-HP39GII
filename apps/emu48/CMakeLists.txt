




 
SET(APP_NAME  emu48)
 
   
include_directories(.)
include_directories(..)
aux_source_directory(. ${APP_NAME}_SRCS)
add_executable(${APP_NAME}.elf ../llapi.c ${${APP_NAME}_SRCS})


include_directories(.)
include_directories(./core)

AUX_SOURCE_DIRECTORY(. DIR_EMU48_SRCS)
AUX_SOURCE_DIRECTORY(./core/ DIR_EMU48_SRCS)

ADD_LIBRARY(emu48_lib ${DIR_EMU48_SRCS})
target_compile_options(emu48_lib PRIVATE -mtune=arm926ej-s -mcpu=arm926ej-s -mlittle-endian -mfloat-abi=soft -marm -O3  ) 

target_compile_options(${APP_NAME}.elf PRIVATE -mtune=arm926ej-s -mcpu=arm926ej-s -mlittle-endian -mfloat-abi=soft -marm -Os  ) 
  
 
target_link_libraries(${APP_NAME}.elf PRIVATE 
emu48_lib
-Wl,--wrap=_malloc_r 
-Wl,--wrap=_free_r 
-lnosys -lc -lm -lgcc -lstdc++ -nostartfiles   
-T ${CMAKE_SOURCE_DIR}/apps/script/ld.lds)

add_custom_command(TARGET ${APP_NAME}.elf POST_BUILD
    COMMAND arm-none-eabi-size ${CMAKE_CURRENT_BINARY_DIR}/${APP_NAME}.elf 
    )

add_custom_target(
  ${APP_NAME}.exp ALL
  COMMAND ${CMAKE_OBJCOPY} -I elf32-little -O binary -R .note -R .comment -S ${CMAKE_CURRENT_BINARY_DIR}/${APP_NAME}.elf ${CMAKE_CURRENT_BINARY_DIR}/${APP_NAME}.exp 
  DEPENDS ${APP_NAME}.elf 
  )


