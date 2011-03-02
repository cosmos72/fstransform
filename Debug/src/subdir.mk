################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CC_SRCS += \
../src/ctx.cc \
../src/fail.cc \
../src/file_extent.cc \
../src/file_util.cc \
../src/main.cc \
../src/map.cc \
../src/vector.cc \
../src/work.cc 

OBJS += \
./src/ctx.o \
./src/fail.o \
./src/file_extent.o \
./src/file_util.o \
./src/main.o \
./src/map.o \
./src/vector.o \
./src/work.o 

CC_DEPS += \
./src/ctx.d \
./src/fail.d \
./src/file_extent.d \
./src/file_util.d \
./src/main.d \
./src/map.d \
./src/vector.d \
./src/work.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cc
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


