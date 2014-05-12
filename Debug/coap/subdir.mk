################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
O_SRCS += \
../coap/coap_cache.o \
../coap/coap_client.o \
../coap/coap_context.o \
../coap/coap_debug.o \
../coap/coap_list.o \
../coap/coap_md5.o \
../coap/coap_net.o \
../coap/coap_option.o \
../coap/coap_pdu.o \
../coap/coap_rd.o \
../coap/coap_resource.o \
../coap/coap_server.o 

C_SRCS += \
../coap/coap_cache.c \
../coap/coap_client.c \
../coap/coap_context.c \
../coap/coap_debug.c \
../coap/coap_list.c \
../coap/coap_md5.c \
../coap/coap_net.c \
../coap/coap_option.c \
../coap/coap_pdu.c \
../coap/coap_rd.c \
../coap/coap_resource.c \
../coap/coap_server.c 

OBJS += \
./coap/coap_cache.o \
./coap/coap_client.o \
./coap/coap_context.o \
./coap/coap_debug.o \
./coap/coap_list.o \
./coap/coap_md5.o \
./coap/coap_net.o \
./coap/coap_option.o \
./coap/coap_pdu.o \
./coap/coap_rd.o \
./coap/coap_resource.o \
./coap/coap_server.o 

C_DEPS += \
./coap/coap_cache.d \
./coap/coap_client.d \
./coap/coap_context.d \
./coap/coap_debug.d \
./coap/coap_list.d \
./coap/coap_md5.d \
./coap/coap_net.d \
./coap/coap_option.d \
./coap/coap_pdu.d \
./coap/coap_rd.d \
./coap/coap_resource.d \
./coap/coap_server.d 


# Each subdirectory must supply rules for building sources it contributes
coap/%.o: ../coap/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


