################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CC_SRCS += \
../src/arch/mem.cc \
../src/arch/mem_linux.cc 

OBJS += \
./src/arch/mem.o \
./src/arch/mem_linux.o 

CC_DEPS += \
./src/arch/mem.d \
./src/arch/mem_linux.d 


# Each subdirectory must supply rules for building sources it contributes
src/arch/%.o: ../src/arch/%.cc
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


