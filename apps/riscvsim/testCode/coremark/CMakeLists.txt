cmake_minimum_required(VERSION 3.1)


SET(CMAKE_SYSTEM_NAME Generic)
SET(CMAKE_SYSTEM_PROCESSOR riscv)
 
set(CMAKE_C_COMPILER riscv-none-embed-gcc)
set(CMAKE_CXX_COMPILER riscv-none-embed-g++)
set(CMAKE_ASM_COMPILER riscv-none-embed-gcc)
set(CMAKE_OBJCOPY riscv-none-embed-objcopy)
set(CMAKE_SIZE riscv-none-embed-size)  

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY) 
set(CMAKE_C_COMPILER_WORKS 1)
set(CMAKE_CXX_COMPILER_WORKS 1) 

set(CMAKE_C_COMPILER_FORCED TRUE)
set(CMAKE_CXX_COMPILER_FORCED TRUE)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")
set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")

project(cm) 

SET(APP_NAME  cm)
   
include_directories(.) 
include_directories(./cm) 
aux_source_directory(./cm ${APP_NAME}_SRCS)
aux_source_directory(. ${APP_NAME}_SRCS)
add_executable(${APP_NAME}.elf  ${${APP_NAME}_SRCS})

target_compile_options(${APP_NAME}.elf PRIVATE -march=rv32ima -mabi=ilp32 -O3  ) 
target_link_libraries(${APP_NAME}.elf PRIVATE  
-lnosys -lc -lm -lgcc  -nostartfiles   
-T ${CMAKE_SOURCE_DIR}/ld.lds)


add_custom_command(TARGET ${APP_NAME}.elf POST_BUILD
    COMMAND arm-none-eabi-size ${CMAKE_CURRENT_BINARY_DIR}/${APP_NAME}.elf 
    )

add_custom_target(
  ${APP_NAME}.bin ALL
  COMMAND ${CMAKE_OBJCOPY} -I elf32-little -O binary -R .note -R .comment -S ${CMAKE_CURRENT_BINARY_DIR}/${APP_NAME}.elf ${CMAKE_CURRENT_BINARY_DIR}/${APP_NAME}.bin 
  DEPENDS ${APP_NAME}.elf 
  )

