################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CC_SRCS += \
../src/ui/ui.cc \
../src/ui/ui_tty.cc 

OBJS += \
./src/ui/ui.o \
./src/ui/ui_tty.o 

CC_DEPS += \
./src/ui/ui.d \
./src/ui/ui_tty.d 


# Each subdirectory must supply rules for building sources it contributes
src/ui/%.o: ../src/ui/%.cc
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


