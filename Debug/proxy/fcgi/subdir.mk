################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
O_SRCS += \
../proxy/fcgi/fcgi_stdio.o \
../proxy/fcgi/fcgiapp.o \
../proxy/fcgi/os_unix.o 

C_SRCS += \
../proxy/fcgi/fcgi_stdio.c \
../proxy/fcgi/fcgiapp.c \
../proxy/fcgi/os_unix.c 

OBJS += \
./proxy/fcgi/fcgi_stdio.o \
./proxy/fcgi/fcgiapp.o \
./proxy/fcgi/os_unix.o 

C_DEPS += \
./proxy/fcgi/fcgi_stdio.d \
./proxy/fcgi/fcgiapp.d \
./proxy/fcgi/os_unix.d 


# Each subdirectory must supply rules for building sources it contributes
proxy/fcgi/%.o: ../proxy/fcgi/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


