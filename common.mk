ifeq ($(ODP_SDK),)
$(error "Please define ODP_SDK environment variable")
endif

# Compiler debug level
#CFLAGS += -O3
#CFLAGS +=  -g -rdynamic -O0
CFLAGS += -O0

#Turn warnings into errors
#CFLAGS += -Werror 
CFLAGS += -Wall 
#CFLAGS += -w
CFLAGS += -Wno-unused-function
CFLAGS += -Wno-unused-variable
CFLAGS += -Wno-unused-value
CFLAGS += -Wmaybe-uninitialized

CFLAGS += -std=gnu99

#To define the backend used: odp/dpdk
CFLAGS += -D ODP_BK

#CFLAGS += -D NDEBUG
#CFLAGS += -D NINFO
#CFLAGS += -D NSIGG

#CFLAGS += -fno-stack-protector
