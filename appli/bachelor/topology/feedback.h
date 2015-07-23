#include "iotlab_uid.h"
#include "printf.h"

#define MESSAGE(fmt, ...) printf("%04x;MSG;" fmt, iotlab_uid(), ##__VA_ARGS__)

#define ERROR(fmt, ...) printf("%04x;ERR;" fmt, iotlab_uid(), ##__VA_ARGS__)
