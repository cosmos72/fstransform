################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CC_SRCS += \
../src/dispatch.cc \
../src/job.cc \
../src/log.cc \
../src/main.cc \
../src/map.cc \
../src/map_stat.cc \
../src/pool.cc \
../src/transform.cc \
../src/util.cc \
../src/vector.cc \
../src/work.cc 

OBJS += \
./src/dispatch.o \
./src/job.o \
./src/log.o \
./src/main.o \
./src/map.o \
./src/map_stat.o \
./src/pool.o \
./src/transform.o \
./src/util.o \
./src/vector.o \
./src/work.o 

CC_DEPS += \
./src/dispatch.d \
./src/job.d \
./src/log.d \
./src/main.d \
./src/map.d \
./src/map_stat.d \
./src/pool.d \
./src/transform.d \
./src/util.d \
./src/vector.d \
./src/work.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cc
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


