################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CC_SRCS += \
../src/io/extent_file.cc \
../src/io/extent_posix.cc \
../src/io/io.cc \
../src/io/io_emul.cc \
../src/io/io_posix.cc \
../src/io/persist.cc \
../src/io/util.cc \
../src/io/util_posix.cc 

OBJS += \
./src/io/extent_file.o \
./src/io/extent_posix.o \
./src/io/io.o \
./src/io/io_emul.o \
./src/io/io_posix.o \
./src/io/persist.o \
./src/io/util.o \
./src/io/util_posix.o 

CC_DEPS += \
./src/io/extent_file.d \
./src/io/extent_posix.d \
./src/io/io.d \
./src/io/io_emul.d \
./src/io/io_posix.d \
./src/io/persist.d \
./src/io/util.d \
./src/io/util_posix.d 


# Each subdirectory must supply rules for building sources it contributes
src/io/%.o: ../src/io/%.cc
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


