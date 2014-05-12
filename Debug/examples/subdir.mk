################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
O_SRCS += \
../examples/coap_client.o \
../examples/coap_server.o 

C_SRCS += \
../examples/coap_client.c \
../examples/coap_client_benchmark.c \
../examples/coap_server.c 

OBJS += \
./examples/coap_client.o \
./examples/coap_client_benchmark.o \
./examples/coap_server.o 

C_DEPS += \
./examples/coap_client.d \
./examples/coap_client_benchmark.d \
./examples/coap_server.d 


# Each subdirectory must supply rules for building sources it contributes
examples/%.o: ../examples/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


