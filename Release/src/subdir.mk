################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CC_SRCS += \
../src/args.cc \
../src/assert.cc \
../src/dispatch.cc \
../src/eta.cc \
../src/job.cc \
../src/log.cc \
../src/main.cc \
../src/map.cc \
../src/map_stat.cc \
../src/pool.cc \
../src/tmp_zero.cc \
../src/transform.cc \
../src/util.cc \
../src/vector.cc \
../src/work.cc 

OBJS += \
./src/args.o \
./src/assert.o \
./src/dispatch.o \
./src/eta.o \
./src/job.o \
./src/log.o \
./src/main.o \
./src/map.o \
./src/map_stat.o \
./src/pool.o \
./src/tmp_zero.o \
./src/transform.o \
./src/util.o \
./src/vector.o \
./src/work.o 

CC_DEPS += \
./src/args.d \
./src/assert.d \
./src/dispatch.d \
./src/eta.d \
./src/job.d \
./src/log.d \
./src/main.d \
./src/map.d \
./src/map_stat.d \
./src/pool.d \
./src/tmp_zero.d \
./src/transform.d \
./src/util.d \
./src/vector.d \
./src/work.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cc
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


