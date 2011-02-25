################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/fail.c \
../src/filemap.c \
../src/fileutil.c \
../src/main.c \
../src/map.c \
../src/translate.c 

OBJS += \
./src/fail.o \
./src/filemap.o \
./src/fileutil.o \
./src/main.o \
./src/map.o \
./src/translate.o 

C_DEPS += \
./src/fail.d \
./src/filemap.d \
./src/fileutil.d \
./src/main.d \
./src/map.d \
./src/translate.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


