ifeq ($(ODP_SDK),)
$(error "Please define ODP_SDK environment variable")
endif

#CC = clang 
$(info CC = $(CC))

# Compiler debug level
CFLAGS += -O3
#CFLAGS +=  -g 

#Turn warnings into errors
#CFLAGS += -Werror 
CFLAGS += -Wall 
CFLAGS += -Wno-unused-function
CFLAGS += -Wno-unused-variable
CFLAGS += -Wno-unused-value
CFLAGS += -std=gnu99

ifeq ($(CC),gcc)
CFLAGS += -Wmaybe-uninitialized 
else
CFLAGS += -Wuninitialized
endif

#Comment the flags to enable different level of debug
CFLAGS += -D NDEBUG
CFLAGS += -D NINFO
CFLAGS += -D NSIGG

CFLAGS += -D CUCKOO

#CFLAGS += -fno-stack-protector
