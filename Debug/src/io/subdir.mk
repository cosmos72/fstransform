################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CC_SRCS += \
../src/io/io.cc \
../src/io/io_posix.cc \
../src/io/posix_extent.cc \
../src/io/posix_util.cc 

OBJS += \
./src/io/io.o \
./src/io/io_posix.o \
./src/io/posix_extent.o \
./src/io/posix_util.o 

CC_DEPS += \
./src/io/io.d \
./src/io/io_posix.d \
./src/io/posix_extent.d \
./src/io/posix_util.d 


# Each subdirectory must supply rules for building sources it contributes
src/io/%.o: ../src/io/%.cc
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


