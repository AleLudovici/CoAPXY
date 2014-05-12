################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
O_SRCS += \
../proxy/connection.o \
../proxy/main.o \
../proxy/resource.o \
../proxy/sockset.o \
../proxy/websocket.o 

C_SRCS += \
../proxy/connection.c \
../proxy/main.c \
../proxy/resource.c \
../proxy/sockset.c \
../proxy/websocket.c 

OBJS += \
./proxy/connection.o \
./proxy/main.o \
./proxy/resource.o \
./proxy/sockset.o \
./proxy/websocket.o 

C_DEPS += \
./proxy/connection.d \
./proxy/main.d \
./proxy/resource.d \
./proxy/sockset.d \
./proxy/websocket.d 


# Each subdirectory must supply rules for building sources it contributes
proxy/%.o: ../proxy/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


