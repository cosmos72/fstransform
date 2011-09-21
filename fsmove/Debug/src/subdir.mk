################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CC_SRCS += \
../src/args.cc \
../src/assert.cc \
../src/eta.cc \
../src/inode_cache.cc \
../src/log.cc \
../src/main.cc \
../src/move.cc \
../src/util.cc 

OBJS += \
./src/args.o \
./src/assert.o \
./src/eta.o \
./src/inode_cache.o \
./src/log.o \
./src/main.o \
./src/move.o \
./src/util.o 

CC_DEPS += \
./src/args.d \
./src/assert.d \
./src/eta.d \
./src/inode_cache.d \
./src/log.d \
./src/main.d \
./src/move.d \
./src/util.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cc
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


