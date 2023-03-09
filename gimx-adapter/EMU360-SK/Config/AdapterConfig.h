#ifndef _ADAPTER_CONFIG_H_
#define _ADAPTER_CONFIG_H_

#define ADAPTER_TYPE         BYTE_TYPE_X360SK
#define ADAPTER_IN_NUM       (ENDPOINT_DIR_IN | 1)
#define ADAPTER_IN_SIZE      32
#define ADAPTER_IN_INTERVAL  4                       // Was: 1
#define ADAPTER_OUT_NUM      (ENDPOINT_DIR_OUT | 2)
#define ADAPTER_OUT_SIZE     32
#define ADAPTER_OUT_INTERVAL 8

#endif
