#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>

#define SERVER_IP "192.168.58.128"
#define SERVER_PORT 1233
#define LOCAL_PORT 1235  // 固定使用1235端口
#define TEST_DURATION 30  // 测试时长，单位秒

// CSV文件名
#define CSV_FILENAME "udp_perf_results.csv"

// 自定义发送数据
// #define DATA_SIZE 16   
// char send_data[DATA_SIZE] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef, 
//                              0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10};

// #define DATA_SIZE 32  
// char send_data[DATA_SIZE] = {0xdf, 0x88, 0x5d, 0x58, 0x0e, 0x65, 0x4a, 0x92, 0x71, 0xee, 0xb1, 0x00, 0xb5, 0x2d, 0x7d, 0x7a, 0xa6, 0x76, 0x9b, 0xa6, 0xb4, 0xf6, 0x7f, 0xe3, 0x73, 0x88, 0xd0, 0xbe, 0x9b, 0x57, 0xa4, 0x0d
// };

// #define DATA_SIZE 64
// char send_data[DATA_SIZE] = {
//     0x0d, 0xf0, 0x0e, 0xe2, 0xa4, 0xe5, 0x08, 0xf3, 0xe2, 0x75, 0x03, 0xc5, 0x17, 0x21, 0xe0, 0x5e, 0x1a, 0xe6, 0xb2, 0xf3, 0xcd, 0x54, 0x53, 0x5a, 0x20, 0x25, 0xe5, 0x0c, 0x93, 0xc3, 0x0d, 0xde, 0x3e, 0xc3, 0xbd, 0x63, 0xf3, 0x4f, 0xf1, 0xb1, 0x19, 0xcc, 0x3a, 0x87, 0x25, 0x79, 0x9e, 0xad, 0x9d, 0x84, 0xc2, 0xe7, 0xea, 0x4f, 0x4f, 0xdf, 0x0b, 0x78, 0x6d, 0x76, 0x42, 0xd7, 0xe2, 0x13
//    };

// #define DATA_SIZE 128
// char send_data[DATA_SIZE]= {
// 0x62, 0xad, 0xb7, 0xa4, 0xa3, 0xed, 0x1b, 0xf7, 0x21, 0xc4, 0x68, 0x57, 0xdd, 0x58, 0x00, 0x2a, 0xcb, 0x18, 0x95, 0x89, 0x62, 0x4e, 0x60, 0x89, 0x0f, 0x82, 0x35, 0xc9, 0xdb, 0x52, 0xac, 0xb5, 0x5a, 0x56, 0xc3, 0x13, 0x99, 0x4b, 0xb5, 0x5e, 0x78, 0x34, 0xb1, 0x46, 0x9c, 0x1c, 0xbc, 0xd3, 0x57, 0xe9, 0xd5, 0x5f, 0xdf, 0xad, 0xbe, 0x9f, 0x07, 0x86, 0xa7, 0x46, 0x41, 0xaf, 0x2d, 0xe2, 0x6f, 0xb1, 0x86, 0x83, 0x69, 0x4a, 0x8c, 0xa0, 0xf6, 0x23, 0x4a, 0x53, 0x4d, 0x0d, 0x2a, 0xf4, 0x61, 0x4c, 0x2a, 0xb4, 0x15, 0x38, 0x99, 0xef, 0x6f, 0xfe, 0xfe, 0x46, 0x7b, 0x70, 0x20, 0x98, 0xb4, 0x82, 0x7a, 0x0c, 0x92, 0x28, 0xb2, 0x02, 0x3c, 0x60, 0xed, 0x55, 0xfb, 0xe8, 0x36, 0x26, 0xbb, 0x76, 0x94, 0xc0, 0x6a, 0x5a, 0x4e, 0xce, 0x00, 0xd3, 0x31, 0x03, 0xc3, 0xc8, 0x0f, 0x00
// };

// #define DATA_SIZE 256   
// char send_data[DATA_SIZE]= {
//     0xb1, 0x14, 0x38, 0xed, 0x30, 0x23, 0x67, 0x44, 0xfd, 0x6d, 0x83, 0xc0, 0x2b, 0x15, 0xed, 0x2e, 0x9c, 0x15, 0x50, 0xf3, 0x07, 0x01, 0xd7, 0x58, 0xd2, 0x33, 0x3f, 0xf3, 0x69, 0x1d, 0x5e, 0x76, 0xd2, 0x85, 0xb4, 0x47, 0x1c, 0x4f, 0x73, 0x43, 0x20, 0x8d, 0x15, 0x5b, 0x3e, 0x60, 0x58, 0xa4, 0x5a, 0xb1, 0xa8, 0x0c, 0x5a, 0x7f, 0xad, 0x24, 0xce, 0xb7, 0x0b, 0x58, 0x60, 0x13, 0x34, 0xbc, 0xa3, 0x0c, 0xd5, 0xe2, 0x96, 0x2a, 0x14, 0x7b, 0x70, 0xea, 0xe3, 0x85, 0xa7, 0xc9, 0xd1, 0x84, 0x72, 0x2e, 0x37, 0x46, 0x70, 0xc8, 0xeb, 0xaf, 0xcb, 0xe9, 0xc0, 0x02, 0x2a, 0xda, 0x91, 0x3d, 0xf1, 0x84, 0x5a, 0xd1, 0x39, 0x0c, 0x01, 0x42, 0x55, 0x7e, 0x60, 0x66, 0xd9, 0x1f, 0xbc, 0x16, 0x42, 0x8b, 0x10, 0x8c, 0x9d, 0xa6, 0xfb, 0x58, 0xa2, 0x4a, 0xa5, 0xdf, 0x91, 0x9f, 0xd9, 0x6b, 0xce, 0xbb, 0x25, 0x25, 0x38, 0x95, 0x21, 0xff, 0xc3, 0x59, 0x85, 0xa4, 0xce, 0x52, 0xb9, 0x32, 0x36, 0x8b, 0x6a, 0xa1, 0x3e, 0x49, 0xe7, 0x90, 0x8a, 0xe3, 0x1b, 0x9e, 0x86, 0x56, 0x6a, 0xe7, 0x35, 0xfc, 0x4e, 0xef, 0xaf, 0x0c, 0xef, 0xa9, 0x85, 0x51, 0xbc, 0xdd, 0x8b, 0xa3, 0x4a, 0x01, 0x36, 0x84, 0x38, 0x28, 0x01, 0xa5, 0x3e, 0x64, 0x91, 0x9b, 0x11, 0xab, 0x40, 0x6d, 0xee, 0x82, 0x5d, 0xa1, 0x3a, 0x36, 0x96, 0x59, 0xa3, 0x38, 0xff, 0xb8, 0x15, 0xcc, 0x86, 0x2a, 0xda, 0x05, 0x85, 0xbd, 0xb4, 0x3c, 0xc9, 0xf8, 0x42, 0x68, 0x0d, 0x56, 0x31, 0xa9, 0x1a, 0x3f, 0xc3, 0xe5, 0x88, 0x5b, 0xa9, 0xfe, 0xa9, 0x7c, 0xe3, 0x5d, 0x84, 0x0b, 0x59, 0xe7, 0x3e, 0xb5, 0xe8, 0x41, 0x5c, 0x81, 0x51, 0xb3, 0x84, 0x1d, 0x3c, 0x6b, 0x9d, 0x78, 0x84, 0x7d, 0xcf, 0x38, 0x58, 0x52
//        };

 
// #define DATA_SIZE 512   
// char send_data[DATA_SIZE]= {
//           0x2c, 0xb9, 0xf5, 0x02, 0x70, 0x40, 0xa0, 0xcd, 0x39, 0xb1, 0x2e, 0xda, 0x42, 0x14, 0x16, 0x2c, 0x51, 0xfd, 0x8c, 0x20, 0x8c, 0x48, 0x25, 0xdd, 0xe6, 0x58, 0x5f, 0x86, 0x87, 0x0a, 0xf8, 0x17, 0x3d, 0xe0, 0xe2, 0xae, 0x25, 0x82, 0x1b, 0x78, 0x0f, 0x92, 0x0c, 0x98, 0xe9, 0x0c, 0x45, 0x08, 0x6c, 0xb5, 0x1a, 0xfc, 0x2a, 0x26, 0xae, 0x87, 0x30, 0xa9, 0xce, 0x03, 0xea, 0x7d, 0x16, 0xcc, 0xb4, 0xca, 0x1e, 0xc5, 0x05, 0xbc, 0x40, 0x14, 0x59, 0x78, 0xa1, 0xd7, 0x61, 0x7e, 0x6a, 0x24, 0x83, 0x31, 0x0d, 0x91, 0x79, 0x5c, 0x27, 0x47, 0xaf, 0x01, 0xdd, 0x20, 0x1e, 0x35, 0x3a, 0x81, 0x87, 0x20, 0xd3, 0x46, 0x1c, 0xda, 0x99, 0x5e, 0x17, 0xdd, 0xc8, 0xae, 0x5e, 0x4a, 0xd6, 0xd6, 0x55, 0xea, 0x5c, 0x48, 0xbc, 0xa9, 0x5e, 0xc5, 0x96, 0x77, 0xfe, 0xd5, 0xaf, 0x27, 0xbd, 0x23, 0xe7, 0x7d, 0x7e, 0x5c, 0x66, 0x05, 0x1c, 0x10, 0x82, 0x37, 0xe5, 0x2a, 0xe7, 0x60, 0xfb, 0x30, 0xad, 0x55, 0xd0, 0xda, 0x0f, 0x67, 0x3f, 0x80, 0xee, 0xd2, 0xac, 0x3d, 0xa7, 0x36, 0x95, 0xd3, 0xa5, 0xe9, 0x34, 0x25, 0x79, 0xd5, 0x75, 0xe6, 0x31, 0x6c, 0x13, 0x6a, 0x0d, 0xf9, 0x45, 0xed, 0xe3, 0x07, 0x6c, 0x75, 0x6f, 0x09, 0x63, 0x42, 0x54, 0xd0, 0xf5, 0x20, 0xce, 0x19, 0x33, 0xd8, 0x44, 0x42, 0xf2, 0x44, 0x99, 0x29, 0xe0, 0x01, 0xb6, 0xb2, 0x51, 0x04, 0x48, 0x50, 0xf1, 0xef, 0x97, 0xc6, 0xea, 0xe8, 0x3f, 0x00, 0x8d, 0xbf, 0xe9, 0x4e, 0x88, 0x7c, 0x72, 0x33, 0x4b, 0xf8, 0x08, 0xdb, 0x99, 0x1c, 0x1e, 0x7e, 0x2a, 0xa3, 0xb4, 0xe0, 0x3f, 0xa5, 0x3e, 0x93, 0xbe, 0xbb, 0x1d, 0x11, 0xb4, 0xd7, 0x30, 0x99, 0xce, 0xad, 0x73, 0x1b, 0xa8, 0xbc, 0x48, 0x56, 0xeb, 0xc6, 0x36, 0x00, 0xae, 0x79, 0x52, 0xf8, 0x48, 0x30, 0xd8, 0xc4, 0xac, 0xde, 0x8e, 0xd9, 0xae, 0x5d, 0xe0, 0x58, 0xe1, 0xd9, 0x80, 0x7c, 0xa9, 0x4e, 0x4e, 0xaa, 0xcc, 0xe5, 0x8c, 0xb7, 0x58, 0x59, 0x73, 0x5a, 0x1b, 0x96, 0x75, 0x69, 0x1e, 0xcc, 0x13, 0x94, 0xf6, 0xd7, 0x73, 0xd0, 0x33, 0x42, 0x51, 0xa8, 0x15, 0x71, 0xa9, 0xb5, 0x44, 0xb9, 0xac, 0xa4, 0x66, 0x6a, 0x94, 0xb8, 0x65, 0x6e, 0x7c, 0x52, 0x94, 0xd3, 0x92, 0xab, 0xde, 0xf7, 0xa1, 0x6b, 0x41, 0xf2, 0x96, 0x8a, 0x4d, 0xf0, 0x01, 0x03, 0xaf, 0x4c, 0xd5, 0xd1, 0xd1, 0xae, 0x42, 0x09, 0x40, 0x83, 0x23, 0x4d, 0x6f, 0x92, 0x41, 0x27, 0xbc, 0x97, 0x33, 0x33, 0xe5, 0x06, 0x06, 0xe6, 0x73, 0x6f, 0x5f, 0x9a, 0x28, 0xdb, 0xb7, 0xcb, 0xff, 0x24, 0x39, 0x5e, 0xd6, 0xdc, 0x93, 0x62, 0x2a, 0xb3, 0x2e, 0x89, 0x5d, 0x0d, 0xa3, 0x08, 0xcd, 0x82, 0x5d, 0x67, 0x54, 0xaf, 0xb2, 0xa3, 0xcb, 0x20, 0x0c, 0xb7, 0xec, 0xec, 0x9d, 0x74, 0x2c, 0xbc, 0xec, 0xa5, 0xf9, 0x52, 0xdd, 0x89, 0x90, 0xb5, 0xfd, 0xb3, 0xc8, 0xe2, 0x8b, 0x6c, 0xab, 0x84, 0x68, 0xd1, 0xf6, 0xd9, 0x88, 0x4c, 0x34, 0x9b, 0x4c, 0xbf, 0x7e, 0x9c, 0xcb, 0x82, 0xef, 0x4f, 0x69, 0xac, 0x44, 0x64, 0x8c, 0x6f, 0xe1, 0x8a, 0xee, 0x12, 0x82, 0x0b, 0x23, 0x57, 0xaa, 0xcc, 0x7d, 0x68, 0xba, 0x57, 0xbc, 0xe6, 0x1b, 0x01, 0x3d, 0xe1, 0x1c, 0xee, 0xcf, 0xa1, 0xb4, 0xb7, 0x1f, 0xe2, 0xa3, 0xe4, 0x56, 0xa2, 0x3b, 0xb9, 0x97, 0x0f, 0xba, 0x2e, 0xed, 0x88, 0x50, 0x32, 0x7a, 0x13, 0x85, 0xcc, 0xb7, 0x19, 0xfb, 0x54, 0x02, 0x6b, 0xb0, 0xd7, 0xb8, 0x87, 0xa1, 0xb3, 0xf9, 0x82, 0xe6, 0x6f, 0x1c, 0xb5, 0x1f, 0xfc, 0x5e, 0x20, 0xc5, 0x5a
//                     };

// #define DATA_SIZE 1024   
// char send_data[DATA_SIZE]= {
//               0x62, 0x08, 0x58, 0xe4, 0xbc, 0x7a, 0x36, 0x60, 0x29, 0x32, 0x26, 0xf7, 0x0c, 0x3b, 0x6b, 0x1a, 0xdf, 0x85, 0xf6, 0xb3, 0x23, 0x24, 0xe7, 0x18, 0x24, 0x63, 0x2b, 0x78, 0x2a, 0xc5, 0x68, 0xbf, 0xe7, 0x8f, 0x34, 0x23, 0xbe, 0x89, 0xa4, 0x31, 0x81, 0x38, 0xfc, 0x4d, 0x71, 0x66, 0xce, 0xe5, 0xbc, 0x85, 0x92, 0x2b, 0x77, 0x45, 0x02, 0x05, 0x33, 0x1f, 0x32, 0xb9, 0xf6, 0x16, 0x4a, 0x65, 0xd8, 0xd7, 0xa2, 0x47, 0x03, 0x46, 0x28, 0x17, 0x18, 0x04, 0x5f, 0xba, 0xb6, 0xec, 0xd7, 0x45, 0x8e, 0x66, 0x29, 0xbf, 0xf4, 0x98, 0x8c, 0x7d, 0x80, 0x77, 0xbc, 0xfb, 0x8c, 0xf5, 0x63, 0x5c, 0x92, 0x90, 0x30, 0x67, 0xe4, 0xc6, 0xd0, 0x69, 0xe2, 0x80, 0x89, 0x9e, 0x69, 0xf4, 0x17, 0xae, 0x8c, 0x82, 0x6c, 0xbe, 0x08, 0x29, 0x36, 0x6c, 0x17, 0x8a, 0x70, 0x8c, 0x24, 0xa2, 0x94, 0x0d, 0x72, 0x08, 0xba, 0xe3, 0x45, 0x95, 0x27, 0xe5, 0x26, 0xda, 0x13, 0xd0, 0xf4, 0x0e, 0x50, 0x7b, 0x7d, 0x4c, 0xd8, 0xe5, 0xfa, 0x9b, 0xb7, 0xf7, 0x09, 0xc9, 0x40, 0x08, 0x65, 0x1f, 0x9e, 0xec, 0x06, 0x51, 0x1f, 0xe9, 0x5a, 0xb4, 0xe7, 0x91, 0x39, 0x30, 0x50, 0xd6, 0x9e, 0xd3, 0xe8, 0x98, 0x55, 0xeb, 0xbc, 0x92, 0x03, 0x1c, 0x71, 0x56, 0x7e, 0xd5, 0x7a, 0x85, 0x3f, 0xb9, 0xf1, 0x0f, 0x12, 0xc6, 0x80, 0x99, 0xac, 0x80, 0xd5, 0x1f, 0x73, 0x10, 0x71, 0x9f, 0x21, 0x82, 0x55, 0xe1, 0xf2, 0xc0, 0x26, 0xe4, 0x01, 0x17, 0x98, 0xe6, 0xef, 0xc9, 0x67, 0x77, 0x7e, 0xde, 0xe0, 0xba, 0xe1, 0x26, 0xd0, 0x37, 0xd7, 0x7a, 0x00, 0xb7, 0x24, 0x7a, 0xfb, 0x89, 0x83, 0xb9, 0x92, 0xfe, 0x9b, 0xf4, 0x84, 0x23, 0x76, 0x0c, 0xef, 0xc3, 0x4e, 0xb5, 0x58, 0x56, 0xa0, 0xf9, 0x85, 0x5c, 0xdb, 0xfc, 0x19, 0xe5, 0xd0, 0x68, 0x34, 0x3d, 0x13, 0xf0, 0xcc, 0xac, 0x16, 0x0c, 0x05, 0x57, 0x3d, 0xab, 0xa1, 0xea, 0x17, 0xdc, 0x1b, 0xd8, 0xae, 0x9b, 0x32, 0x8d, 0x20, 0x88, 0xe6, 0x08, 0x59, 0xa4, 0xa9, 0x65, 0x11, 0xb0, 0x93, 0x2c, 0x7b, 0x28, 0x1c, 0xfc, 0x6c, 0xa3, 0xc7, 0x44, 0x79, 0x13, 0xe8, 0xa4, 0xda, 0xc0, 0x23, 0x93, 0x96, 0x98, 0xc5, 0x63, 0xbd, 0xb0, 0xd0, 0x3d, 0x95, 0xd0, 0x1d, 0xf6, 0xd6, 0xfd, 0xc0, 0xca, 0x45, 0x6e, 0xfd, 0xad, 0x63, 0xe8, 0x16, 0x05, 0xca, 0x4c, 0x12, 0x3d, 0xcf, 0x58, 0x2e, 0x5f, 0x5c, 0xe0, 0x65, 0x60, 0x56, 0x2b, 0xe9, 0x3a, 0xd4, 0xbc, 0x23, 0xbf, 0x0b, 0x09, 0xed, 0x42, 0x8d, 0xe1, 0xde, 0xb0, 0x93, 0x54, 0x62, 0x59, 0x05, 0x3c, 0x29, 0x64, 0x3f, 0x39, 0xec, 0xe9, 0xa0, 0x67, 0x50, 0x66, 0xf4, 0x4c, 0x66, 0x14, 0x8f, 0x52, 0xb5, 0x8d, 0x83, 0x47, 0x36, 0xe3, 0x65, 0xb3, 0xe5, 0x5a, 0xc8, 0x9e, 0x9d, 0x58, 0x1f, 0xea, 0x12, 0x8e, 0xae, 0x81, 0x26, 0xb8, 0x64, 0xb6, 0x1f, 0x05, 0x6d, 0x48, 0x4f, 0xd8, 0xfc, 0x96, 0x30, 0x10, 0xe0, 0xce, 0x2a, 0x2f, 0x61, 0x67, 0xe5, 0x62, 0x16, 0xe4, 0x14, 0x17, 0x99, 0xa8, 0x33, 0x96, 0x16, 0x2b, 0x08, 0x79, 0x2b, 0x0b, 0x4c, 0x2c, 0xe4, 0x76, 0x74, 0x17, 0xb3, 0xe0, 0x08, 0xd8, 0x28, 0x9c, 0xee, 0xf3, 0xef, 0xc2, 0xfe, 0xf3, 0xb0, 0xbc, 0xb6, 0xad, 0x64, 0x5a, 0x4d, 0x80, 0xa3, 0x9e, 0xe1, 0x2b, 0x5f, 0x63, 0x5e, 0x5a, 0x27, 0x25, 0xb3, 0x93, 0x42, 0x7d, 0xa8, 0x69, 0xfe, 0xa8, 0x71, 0x61, 0xb8, 0x09, 0xb2, 0x2b, 0xde, 0xc6, 0x21, 0x55, 0xc3, 0x58, 0xe0, 0x1e, 0x9f, 0xe1, 0x6f, 0xbb, 0x5d, 0x37, 0x1a, 0xcc, 0x77, 0xd5, 0x20, 0x15, 0xa8, 0x8a, 0x15, 0xbe, 0x5c, 0x7c, 0xfc, 0x3f, 0xce, 0xfc, 0x82, 0xce, 0x11, 0xdb, 0x7a, 0xec, 0xbc, 0x83, 0x13, 0x3e, 0x7a, 0xf8, 0x16, 0x07, 0x41, 0xe0, 0x64, 0xdd, 0xfe, 0x73, 0xca, 0xa9, 0xc9, 0x8e, 0x77, 0x65, 0xe0, 0x0b, 0xde, 0xf1, 0xfc, 0xf5, 0x6a, 0x55, 0xf8, 0x5b, 0x03, 0x7a, 0x51, 0xf9, 0x79, 0xce, 0x3a, 0x2e, 0xf3, 0x6e, 0xec, 0xbe, 0xa7, 0xd9, 0x95, 0xb5, 0xf8, 0x9f, 0x21, 0x26, 0x5e, 0x01, 0x63, 0x21, 0xb4, 0x15, 0xfe, 0xe8, 0x4e, 0xd1, 0x9a, 0xf9, 0xf3, 0xc9, 0x09, 0x16, 0x07, 0xf1, 0x9a, 0xc2, 0xb6, 0xcf, 0xf5, 0x51, 0xd3, 0xfe, 0x1f, 0x85, 0x36, 0xcc, 0x10, 0x87, 0x2f, 0xdd, 0xb0, 0x17, 0x05, 0x13, 0x8e, 0x8b, 0xd3, 0x9a, 0x75, 0xa7, 0x3e, 0x95, 0x56, 0x4a, 0xc0, 0xce, 0xfb, 0x8e, 0x96, 0x18, 0x15, 0x3f, 0xd5, 0x71, 0x5d, 0x6b, 0xc0, 0x27, 0x84, 0x08, 0x9f, 0x96, 0xfb, 0x7e, 0x5d, 0x99, 0xca, 0x79, 0xea, 0xc4, 0x16, 0x13, 0x90, 0x62, 0xd0, 0xe6, 0xc8, 0xa5, 0x2c, 0x02, 0x76, 0x2f, 0x22, 0xa0, 0x74, 0xa7, 0x8d, 0x0a, 0xf6, 0x92, 0x03, 0xec, 0xe0, 0x30, 0x1c, 0xa2, 0x80, 0xfa, 0xb1, 0x29, 0x33, 0x25, 0xcf, 0xe8, 0x39, 0x46, 0xf6, 0x79, 0xb3, 0x51, 0x8e, 0x6d, 0x62, 0xe1, 0xf8, 0x0f, 0x76, 0x72, 0x86, 0x1d, 0x3e, 0x18, 0x46, 0x79, 0xf3, 0x8f, 0x8d, 0x9a, 0xdb, 0xa9, 0xb0, 0x67, 0x57, 0xe3, 0x96, 0xdf, 0xc7, 0x8e, 0x67, 0xdd, 0x2a, 0xff, 0xd4, 0x12, 0x43, 0x91, 0x07, 0xe1, 0x48, 0x8e, 0xa4, 0x9a, 0x1c, 0x65, 0x26, 0x6a, 0x66, 0x5b, 0xf2, 0xdd, 0xa7, 0xcb, 0x40, 0x6a, 0x60, 0x33, 0x73, 0x17, 0xa0, 0xb4, 0x10, 0x83, 0xe5, 0x30, 0xa1, 0x0f, 0x4b, 0x4a, 0x8f, 0x35, 0xe0, 0x94, 0xca, 0xa1, 0x71, 0x20, 0x1d, 0xe0, 0x27, 0x27, 0xbc, 0x8a, 0xf3, 0x1d, 0xae, 0x0b, 0x58, 0xec, 0x3d, 0xe8, 0x75, 0x77, 0xa5, 0x32, 0x95, 0x23, 0x39, 0x5c, 0x9e, 0x12, 0x84, 0xc4, 0x88, 0x28, 0xac, 0x8c, 0xf8, 0xe3, 0xf3, 0xcd, 0x84, 0xa9, 0x5c, 0xe5, 0xf3, 0x65, 0x11, 0xb9, 0xc1, 0x1e, 0x59, 0x76, 0xea, 0x21, 0x64, 0xae, 0xfb, 0x83, 0xf8, 0x92, 0x2e, 0x52, 0x88, 0x64, 0x10, 0x40, 0x31, 0xe5, 0x3b, 0x01, 0x21, 0x55, 0x62, 0x66, 0x25, 0x82, 0x53, 0x3a, 0x84, 0x21, 0x04, 0x87, 0x10, 0xb1, 0x77, 0x5c, 0xea, 0xdf, 0x02, 0xfd, 0xb4, 0x8d, 0x45, 0xdc, 0xd8, 0x3e, 0x6b, 0x40, 0xb0, 0x87, 0xd1, 0x9f, 0xd7, 0xcc, 0x09, 0x68, 0x2c, 0xe6, 0xc2, 0xf5, 0x51, 0x86, 0xc5, 0x91, 0xe9, 0x85, 0x07, 0x72, 0x65, 0x1e, 0x29, 0xc7, 0x27, 0x58, 0x06, 0xdb, 0x3b, 0xf7, 0xdd, 0x52, 0x08, 0x39, 0x86, 0xe7, 0xd8, 0x91, 0x05, 0xb8, 0xab, 0xe9, 0x03, 0x5a, 0x75, 0xd9, 0x00, 0xc3, 0x07, 0x0a, 0xa5, 0xc3, 0x74, 0x50, 0x9f, 0x39, 0xc9, 0x11, 0xf2, 0x46, 0x07, 0xa1, 0x48, 0xe2, 0x27, 0x44, 0xa9, 0x99, 0xe1, 0x3b, 0xfb, 0x64, 0xa6, 0x98, 0x50, 0xc8, 0x0b, 0x37, 0xb2, 0xf4, 0x67, 0x6c, 0x3c, 0xf7, 0x56, 0x09, 0x77, 0xf5, 0xe1, 0x6c, 0x44, 0x7a, 0x18, 0x32, 0x18, 0x09, 0xbb, 0x00, 0x06, 0x65, 0x33, 0xdd, 0x66, 0x6e, 0x22, 0x48, 0x41, 0x1e, 0x29, 0xf7, 0xe7, 0xe7, 0x87, 0xeb, 0x4b, 0x70, 0x45, 0xf7, 0x5d, 0x39, 0xdc, 0xe2, 0x09, 0x2a, 0xbb, 0xde, 0x4b, 0xb1, 0xce, 0x75, 0x8b, 0xce, 0xcb, 0x88, 0x86, 0xbe, 0xb0, 0x82, 0xad, 0x3f, 0x50, 0xa8, 0xc0, 0x6f, 0xfe, 0x54, 0x56, 0x5a, 0x92, 0xd4, 0x86, 0x3f, 0x64, 0xbb, 0x2a, 0x9b, 0x68, 0x15, 0x04, 0xbd, 0xcc, 0x6c, 0x00, 0x8b
//                            };

#define DATA_SIZE 1472  
char send_data[DATA_SIZE]= {
    0x93, 0xc2, 0xd9, 0xe5, 0x16, 0xc9, 0xe9, 0xed, 0x1d, 0x0d, 0x19, 0xef, 0x64, 0x92, 0xc5, 0x77, 0x70, 0xec, 0xd8, 0xee, 0x6f, 0x8d, 0xd1, 0x08, 0x52, 0x00, 0xe7, 0x37, 0x81, 0x97, 0x1c, 0x8f, 0x4d, 0x75, 0xa4, 0x79, 0x64, 0x59, 0xf2, 0x24, 0x31, 0x17, 0x49, 0xe4, 0x2a, 0x91, 0xb5, 0x1f, 0x95, 0x82, 0xe2, 0x46, 0x4b, 0x23, 0x5e, 0x09, 0x8f, 0xa5, 0xb5, 0xd9, 0xf9, 0xd5, 0x8a, 0xe8, 0x12, 0xcd, 0x19, 0xd6, 0x06, 0xee, 0xf9, 0xf9, 0x77, 0x30, 0x88, 0xcd, 0x7b, 0xc9, 0x71, 0x7f, 0xa3, 0x70, 0xf1, 0x10, 0xc4, 0x89, 0x5b, 0x0b, 0xe9, 0xd9, 0x12, 0x11, 0xfb, 0xe9, 0x43, 0x04, 0x10, 0x6c, 0x44, 0xb9, 0x7f, 0x49, 0x00, 0x7a, 0x05, 0x83, 0x86, 0xf7, 0x7a, 0x63, 0x88, 0xd7, 0xb7, 0x6d, 0xe9, 0x1e, 0x62, 0xb0, 0xf1, 0x01, 0x4b, 0x6c, 0x0a, 0x5b, 0x50, 0x0e, 0xdf, 0xa2, 0x92, 0x8f, 0x5c, 0x2c, 0xef, 0x63, 0x93, 0xf6, 0x92, 0xb1, 0x44, 0x08, 0x57, 0x90, 0x87, 0xd9, 0x6b, 0xea, 0x4e, 0x24, 0x65, 0x24, 0x4f, 0x41, 0x45, 0x8f, 0xc6, 0x48, 0x38, 0x0d, 0x82, 0x54, 0xe6, 0x48, 0x2b, 0x0f, 0xc4, 0x2e, 0x0a, 0x8f, 0xc1, 0x5d, 0x9c, 0x3a, 0x8d, 0x29, 0xe8, 0x25, 0x40, 0xe5, 0x69, 0x32, 0xa2, 0xc3, 0xca, 0xd0, 0xd3, 0x97, 0x05, 0xd1, 0x0a, 0x92, 0x18, 0x60, 0xb9, 0x90, 0x36, 0xb5, 0xb0, 0xf8, 0x82, 0xd0, 0xd2, 0x97, 0xa7, 0x41, 0xbf, 0xa1, 0x01, 0x4d, 0xb6, 0xe3, 0x52, 0xee, 0x97, 0x89, 0x64, 0xa4, 0x52, 0x06, 0x46, 0x05, 0xd4, 0x5c, 0xb1, 0x03, 0x36, 0xf2, 0x60, 0x78, 0x31, 0xca, 0xe7, 0x1b, 0x17, 0x8b, 0xa4, 0x4a, 0x16, 0x8b, 0x58, 0x67, 0x7a, 0x97, 0x20, 0x5b, 0xde, 0x70, 0x2e, 0x99, 0x22, 0x0c, 0x18, 0xce, 0xf3, 0x8b, 0xa2, 0x0e, 0x2d, 0x66, 0x3f, 0xc0, 0xbe, 0x78, 0xd2, 0x98, 0xa0, 0x57, 0x26, 0x04, 0xd8, 0x8e, 0x8a, 0x57, 0xb8, 0xc2, 0x24, 0xb7, 0x8b, 0x78, 0xe5, 0x16, 0x95, 0xbf, 0xfb, 0x61, 0x8f, 0xeb, 0xd0, 0x0e, 0x38, 0xdb, 0x13, 0xea, 0x63, 0xdb, 0x76, 0x2f, 0x5c, 0x52, 0xb4, 0xbf, 0x75, 0x91, 0x98, 0x17, 0x88, 0x5e, 0x87, 0x5f, 0xd8, 0xff, 0x11, 0x3c, 0x0c, 0x13, 0x6f, 0x0d, 0x5f, 0x49, 0x9a, 0xb0, 0x2e, 0xda, 0x3d, 0x55, 0x80, 0x86, 0x02, 0x9a, 0x42, 0xf6, 0x63, 0x57, 0xe3, 0x01, 0xfa, 0x13, 0x5a, 0x65, 0x8d, 0xfd, 0xb3, 0x7e, 0x96, 0xe5, 0x62, 0x41, 0xb0, 0x15, 0x4c, 0x70, 0x3f, 0x56, 0x24, 0x98, 0x73, 0x28, 0xa1, 0xf0, 0xb9, 0x26, 0xf7, 0x98, 0xe2, 0x27, 0x34, 0xf3, 0x90, 0x19, 0x3d, 0x60, 0x14, 0xb9, 0x2a, 0x98, 0x69, 0xe0, 0xd9, 0x6e, 0x71, 0x85, 0x94, 0xf2, 0xde, 0xaa, 0xc9, 0xd9, 0x57, 0xe6, 0x61, 0x92, 0x1f, 0x16, 0xc7, 0xf1, 0xe5, 0x20, 0x38, 0x6c, 0x6c, 0x5b, 0xcb, 0xc8, 0xa4, 0xd0, 0xa8, 0x01, 0xbc, 0x02, 0x38, 0x83, 0x51, 0x93, 0x99, 0xd6, 0x43, 0x37, 0xfa, 0xbf, 0x64, 0x4e, 0x85, 0x84, 0xcb, 0x5f, 0x89, 0xdd, 0x48, 0x66, 0xfd, 0x7c, 0x5e, 0x6a, 0x2b, 0x34, 0x80, 0x12, 0x14, 0xa1, 0xdd, 0x50, 0xdc, 0x72, 0x0e, 0xe0, 0xed, 0x5d, 0x1a, 0x29, 0x66, 0xeb, 0xf0, 0x62, 0xed, 0x6a, 0xaf, 0xaa, 0xc9, 0x90, 0xb3, 0xb2, 0x26, 0x5d, 0xa7, 0x7c, 0xb6, 0xbc, 0xca, 0x45, 0x58, 0x6a, 0x7c, 0x76, 0xdb, 0xa2, 0xbf, 0x46, 0x7c, 0x13, 0x4a, 0xf5, 0x46, 0x2a, 0x4f, 0xba, 0x34, 0x4e, 0x1d, 0x90, 0xd3, 0xc3, 0x6a, 0xd6, 0x7a, 0x9c, 0x92, 0x8b, 0x47, 0x25, 0xb3, 0xe0, 0x53, 0x6b, 0x65, 0x44, 0xf6, 0xd9, 0x46, 0x4e, 0x56, 0x99, 0xdf, 0x2f, 0xb8, 0xe6, 0xb5, 0x13, 0xa8, 0xdf, 0xd1, 0xbf, 0x12, 0xbf, 0xea, 0x19, 0xf8, 0x5a, 0x99, 0xc2, 0xba, 0x4d, 0xbf, 0x83, 0x16, 0xe5, 0x7a, 0x44, 0x28, 0x35, 0x4f, 0x22, 0x64, 0x4c, 0xf6, 0xed, 0x9e, 0xd4, 0x9a, 0xe1, 0x94, 0x42, 0xc8, 0x65, 0xe9, 0xba, 0x98, 0x72, 0x32, 0xc7, 0x40, 0x0f, 0x35, 0xb0, 0xa6, 0x2a, 0xe2, 0x90, 0xc9, 0x21, 0x22, 0xbc, 0x1f, 0x12, 0x11, 0x82, 0xb1, 0x12, 0xd9, 0xd6, 0x8f, 0x33, 0x45, 0x57, 0xea, 0x03, 0x99, 0x4c, 0x15, 0x27, 0xfa, 0xa6, 0xd5, 0x6b, 0x52, 0x4e, 0x29, 0x8b, 0x5b, 0x78, 0x89, 0xef, 0x6c, 0x56, 0x39, 0x81, 0x75, 0xf6, 0x08, 0xd5, 0xc2, 0x24, 0x7e, 0xbe, 0x59, 0x18, 0xc1, 0xda, 0x6b, 0x44, 0x3b, 0x14, 0xfc, 0x2f, 0x07, 0x37, 0xcc, 0x16, 0x08, 0x2d, 0x39, 0x90, 0x38, 0xc8, 0x0d, 0x9f, 0xc3, 0x1e, 0x68, 0x44, 0xf9, 0x69, 0xd9, 0x29, 0x80, 0x6b, 0x2a, 0x0c, 0x91, 0xa7, 0x55, 0x53, 0x01, 0xf3, 0x7c, 0xde, 0xd2, 0x96, 0x0f, 0x83, 0xaa, 0x8a, 0xe6, 0x80, 0x68, 0xec, 0xec, 0x4f, 0x73, 0x90, 0x27, 0x33, 0xed, 0x8e, 0xde, 0xaa, 0x77, 0x97, 0x10, 0xfd, 0xbc, 0x63, 0xf9, 0x4d, 0x5b, 0x2a, 0x8d, 0xd2, 0x4c, 0xca, 0xdb, 0x72, 0xf6, 0xd7, 0x8a, 0x9c, 0x2a, 0x0d, 0x6f, 0xa3, 0x41, 0xc0, 0x91, 0x87, 0x12, 0xe2, 0xcb, 0x76, 0x4c, 0xc5, 0x40, 0x73, 0x52, 0x81, 0x1b, 0xbc, 0xf6, 0xfa, 0xad, 0xbe, 0xc9, 0x15, 0x96, 0xdf, 0xe8, 0xfa, 0x5a, 0xa6, 0x5b, 0x4b, 0x6e, 0x48, 0x07, 0xc9, 0x64, 0xff, 0xd7, 0x57, 0x58, 0x0d, 0xac, 0xe7, 0x3b, 0x41, 0x79, 0x1d, 0x2f, 0x1b, 0xf0, 0x85, 0x05, 0x64, 0x96, 0xe3, 0x6e, 0x8f, 0xfd, 0x4f, 0x51, 0x83, 0xf2, 0x09, 0x52, 0x9f, 0x7b, 0x39, 0x4b, 0x8e, 0x8f, 0xb1, 0xeb, 0x3a, 0xde, 0xa1, 0x82, 0x00, 0x2f, 0x0f, 0xc0, 0xa1, 0x00, 0x89, 0x57, 0x66, 0xf5, 0x68, 0x78, 0xf8, 0xd9, 0xd9, 0xc7, 0xfe, 0x58, 0x3d, 0x62, 0xc0, 0x86, 0xaf, 0x8a, 0xb7, 0xbf, 0xb2, 0x91, 0xc3, 0x30, 0x8d, 0x37, 0xba, 0x39, 0xb2, 0x7d, 0x51, 0xcd, 0x11, 0x29, 0xd4, 0xda, 0x1d, 0x04, 0x78, 0xce, 0x30, 0x1d, 0x22, 0x4c, 0x33, 0x22, 0xa2, 0x5a, 0x0d, 0x30, 0x89, 0xb7, 0x7e, 0xa3, 0xd8, 0x2e, 0x42, 0x35, 0x07, 0x90, 0x67, 0x21, 0xde, 0x29, 0xb3, 0x75, 0xa7, 0xd4, 0xd6, 0xf4, 0x05, 0x53, 0xf6, 0x29, 0xc2, 0x83, 0x99, 0xcf, 0x3b, 0x9d, 0x54, 0xf7, 0xf9, 0x55, 0xb9, 0xde, 0x39, 0x83, 0xba, 0x4c, 0x3b, 0xa9, 0xbb, 0xa1, 0x41, 0x3e, 0x1d, 0xe5, 0xac, 0xc5, 0x9f, 0x66, 0x9f, 0x54, 0xfe, 0x8d, 0x25, 0x9f, 0x53, 0xd0, 0x47, 0x62, 0x07, 0x1a, 0xe5, 0x2a, 0x85, 0x66, 0x71, 0x1a, 0x8c, 0x76, 0xe0, 0x98, 0x53, 0xa7, 0x9b, 0xad, 0xac, 0x44, 0x66, 0xa2, 0xab, 0x8f, 0xb2, 0x90, 0x58, 0x31, 0x50, 0x26, 0x84, 0xa8, 0x58, 0x02, 0x96, 0x7f, 0xa5, 0x26, 0x28, 0x7f, 0x3b, 0x13, 0x71, 0xd1, 0xb7, 0x7d, 0x76, 0xe1, 0x24, 0x8b, 0xdb, 0x9d, 0xa5, 0x95, 0x6b, 0xdc, 0x3b, 0xd9, 0x71, 0xac, 0x90, 0x48, 0xec, 0x58, 0x9a, 0xd2, 0x79, 0x49, 0xd0, 0x65, 0x57, 0x0e, 0x87, 0x95, 0x0b, 0x14, 0x49, 0x86, 0xd1, 0xb1, 0x58, 0x58, 0xab, 0xf9, 0x52, 0x18, 0x8a, 0xfa, 0x02, 0xa7, 0x8b, 0x78, 0x7c, 0x27, 0x02, 0x78, 0xfa, 0x14, 0x58, 0xa3, 0xba, 0xc2, 0x27, 0x8e, 0x78, 0xa7, 0xda, 0x03, 0x9f, 0xa0, 0xfa, 0x92, 0x82, 0x92, 0xd1, 0x69, 0x71, 0x71, 0x77, 0x4d, 0x59, 0x91, 0xfb, 0x08, 0xda, 0x17, 0x7e, 0x7f, 0x35, 0x01, 0x12, 0x75, 0x8c, 0x92, 0x31, 0xf3, 0x97, 0xab, 0x46, 0x1e, 0xda, 0x90, 0xd0, 0x27, 0x29, 0xb6, 0x09, 0xe0, 0x06, 0x50, 0x00, 0x86, 0xcf, 0x5a, 0x75, 0x83, 0xee, 0xfb, 0x22, 0x5a, 0x93, 0x1e, 0xb5, 0x5d, 0xc5, 0x85, 0x0b, 0x95, 0x91, 0xe4, 0x80, 0xec, 0x67, 0xa0, 0x7a, 0x69, 0x0e, 0x7c, 0xdd, 0x40, 0x0f, 0x83, 0x71, 0x61, 0xbb, 0x02, 0x53, 0x44, 0xe6, 0xb4, 0x41, 0x7f, 0x9c, 0xaf, 0xc5, 0x02, 0x19, 0xfb, 0x7b, 0xc0, 0xd5, 0xe3, 0xc4, 0x47, 0x9f, 0xa9, 0x5b, 0x1c, 0x12, 0x45, 0x97, 0x6e, 0x38, 0x13, 0x41, 0x83, 0xb6, 0x79, 0x73, 0x28, 0x18, 0xa4, 0x81, 0x81, 0x2e, 0xd0, 0x87, 0xf9, 0xa1, 0x81, 0xcc, 0xea, 0x88, 0x42, 0x18, 0x7a, 0x00, 0xcd, 0x4a, 0xa3, 0x2d, 0x2e, 0x07, 0xff, 0x38, 0x11, 0x83, 0x9a, 0x12, 0xfa, 0xa1, 0x24, 0xcd, 0x40, 0x00, 0xe3, 0xa7, 0x4b, 0x08, 0xd1, 0x50, 0x33, 0xc8, 0x18, 0x5d, 0xa3, 0xd2, 0x55, 0xce, 0x2b, 0xef, 0xa0, 0xae, 0xde, 0x61, 0x8c, 0xaa, 0x40, 0x4c, 0x2c, 0xf0, 0x74, 0x13, 0xac, 0x38, 0x56, 0x64, 0x8e, 0x7f, 0x18, 0x33, 0x80, 0x8f, 0x5a, 0xee, 0x5c, 0xea, 0x9e, 0xd0, 0xf1, 0xbf, 0xc0, 0xbc, 0x73, 0x52, 0x7f, 0x41, 0x6a, 0xb1, 0x50, 0x69, 0xb9, 0xb9, 0x55, 0x24, 0x27, 0xbb, 0xb4, 0x55, 0x9d, 0xc2, 0xec, 0x02, 0xcd, 0xd6, 0x07, 0xf8, 0x7b, 0xc0, 0xe1, 0x82, 0xbd, 0x1c, 0x30, 0xf1, 0xe7, 0x98, 0x2b, 0xdd, 0x0c, 0x99, 0x6b, 0xc7, 0xb0, 0x8c, 0xf7, 0x51, 0xad, 0x90, 0x23, 0x10, 0xf5, 0x4e, 0xcd, 0x93, 0xf5, 0xf9, 0xc2, 0x2d, 0xf4, 0x42, 0xaa, 0x1d, 0xaf, 0xdc, 0x06, 0x7c, 0xfc, 0x64, 0x18, 0xc1, 0xe5, 0x3d, 0x22, 0xa8, 0x02, 0x41, 0x1e, 0x15, 0x0f, 0xcb, 0xc6, 0xea, 0xe1, 0x2e, 0x44, 0xae, 0x2c, 0x59, 0x59, 0x19, 0xdc, 0xfe, 0xbf, 0xe1, 0x9f, 0x02, 0xc9, 0x9f, 0xae, 0xe4, 0x82, 0x79, 0x7d, 0x20, 0xa9, 0x5d, 0x5f, 0x8e, 0xaf, 0x30, 0xff, 0x09, 0x45, 0x71, 0xb9, 0x60, 0x79, 0xbe, 0xd9, 0xab, 0xea, 0x3c, 0x0a, 0x15, 0x1e, 0xc4, 0x2b, 0x5f, 0x2e, 0xdf, 0x38, 0xe8, 0x5b, 0xec, 0x4b, 0xac, 0xfd, 0x08, 0x18, 0x5a, 0x84, 0xa6, 0x08, 0xf7, 0x01, 0xdb, 0x10, 0xc1, 0x31, 0x92, 0x67, 0x9a, 0x85, 0x97, 0x24, 0x68, 0x51, 0x26, 0x62, 0x79, 0x58, 0xb7, 0x9d, 0xb3, 0x1a, 0x23, 0x9c, 0xc9, 0xa3, 0xf9, 0xca, 0xf4, 0x0a, 0x27, 0xd5, 0xc9, 0x10, 0x79, 0x9f, 0x6c, 0x1b, 0x3b, 0xd0, 0xda, 0xf0, 0xca, 0x56, 0xc7, 0x16, 0x21, 0x6f, 0x6a, 0x76, 0x29, 0x14, 0x7e, 0x02, 0xa0, 0x88, 0x36, 0x00, 0x2d, 0x85, 0xfb, 0xb5, 0x31, 0xd3, 0x76, 0xc0, 0x12, 0x27, 0xc4, 0x11, 0x54, 0xc4, 0x67, 0x0a, 0x4b, 0x15, 0x6b, 0x67, 0x8a, 0x64, 0xbe, 0x1f, 0xbc, 0x05, 0x3a, 0x3c, 0x68, 0x67, 0x26, 0x58, 0xdf, 0x14, 0xda, 0x5d, 0x5e, 0x8c, 0xa4, 0x04, 0x96, 0xdb, 0x91, 0x78, 0xd6, 0xe7, 0xfa, 0xb0, 0xe8, 0x57, 0x43, 0xca, 0xd7, 0xe8, 0x85, 0x92, 0x75, 0x4d, 0x1d, 0x1f, 0x7e, 0xae, 0xc7, 0xaa, 0xba, 0x0d, 0x1b, 0xed, 0xf0, 0xcc, 0x91, 0xef, 0xb8, 0x48, 0xe7, 0xe1, 0xdb, 0x50, 0x64, 0x8e, 0xb1
};
// 全局变量，用于通知停止测试
volatile int running = 1;

// 处理中断信号，优雅地结束测试
void handle_signal(int sig) {
    printf("\n接收到信号 %d，准备停止测试...\n", sig);
    running = 0;
}

// 将测试结果保存到CSV文件
void save_results_to_csv(int payload_size, double throughput, double packet_loss, double pps) {
    FILE *fp;
    int file_exists = 0;
    
    // 检查文件是否已存在
    if (access(CSV_FILENAME, F_OK) == 0) {
        file_exists = 1;
    }
    
    // 打开CSV文件进行追加
    fp = fopen(CSV_FILENAME, "a");
    if (fp == NULL) {
        perror("无法打开CSV文件");
        return;
    }
    
    // 如果文件不存在，先写入标题行（包含单位）
    if (!file_exists) {
        fprintf(fp, "payload_size(bytes),throughput(Mbps),packet_loss(%%),pps(packets/s)\n");
    }
    
    // 写入测试结果（只包含四个指定字段）
    fprintf(fp, "%d,%.2f,%.2f,%.2f\n", 
            payload_size, throughput, packet_loss, pps);
    
    // 关闭文件
    fclose(fp);
    
    printf("测试结果已保存到 %s\n", CSV_FILENAME);
}

int main() {
    // 设置信号处理
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
    
    // 创建UDP套接字
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("套接字创建失败");
        return 1;
    }
    
    // 设置服务器地址
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        perror("无效的服务器IP地址");
        close(sockfd);
        return 1;
    }
    
    // 设置本地地址绑定
    struct sockaddr_in local_addr;
    memset(&local_addr, 0, sizeof(local_addr));
    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    local_addr.sin_port = htons(LOCAL_PORT);
    
    if (bind(sockfd, (struct sockaddr *)&local_addr, sizeof(local_addr)) < 0) {
        perror("绑定本地地址失败");
        close(sockfd);
        return 1;
    }
    
    // 设置非阻塞模式
    int flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
    
    // 准备发送数据（已经有定义好的send_data）
    char *buffer = malloc(DATA_SIZE);
    if (!buffer) {
        perror("内存分配失败");
        close(sockfd);
        return 1;
    }
    
    // 复制发送数据
    memcpy(buffer, send_data, DATA_SIZE);
    
    printf("UDP单线程性能测试开始\n");
    printf("服务器地址: %s:%d\n", SERVER_IP, SERVER_PORT);
    printf("本地端口: %d\n", LOCAL_PORT);
    printf("测试时长: %d 秒\n", TEST_DURATION);
    printf("数据包大小: %d 字节\n", DATA_SIZE);
    printf("开始测试...\n");
    
    struct timeval start_time, current_time;
    gettimeofday(&start_time, NULL);
    
    long packets_sent = 0;
    long packets_received = 0;
    long bytes_sent = 0;
    
    char recv_buffer[256];
    socklen_t addr_len = sizeof(server_addr);
    
    // 循环发送，直到测试时间结束
    while (running) {
        gettimeofday(&current_time, NULL);
        double elapsed = (current_time.tv_sec - start_time.tv_sec) + 
                         (current_time.tv_usec - start_time.tv_usec) / 1000000.0;
        
        if (elapsed >= TEST_DURATION) {
            break;
        }
        
        // 发送数据包
        int bytes = sendto(sockfd, buffer, DATA_SIZE, 0, 
                          (struct sockaddr *)&server_addr, sizeof(server_addr));
        
        if (bytes > 0) {
            packets_sent++;
            bytes_sent += bytes;
        } else if (bytes < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
            perror("发送失败");
        }
        
        // 尝试接收响应（非阻塞）
        int recv_bytes = recvfrom(sockfd, recv_buffer, sizeof(recv_buffer), 0,
                                 (struct sockaddr *)&server_addr, &addr_len);
        
        if (recv_bytes > 0) {
            packets_received++;
        }
        
        // 短暂休眠，避免CPU占用过高
        // usleep(10);
    }
    
    // 计算最终测试时间
    gettimeofday(&current_time, NULL);
    double test_time = (current_time.tv_sec - start_time.tv_sec) + 
                      (current_time.tv_usec - start_time.tv_usec) / 1000000.0;
    
    // 计算性能指标
    double mbps = (bytes_sent * 8.0 / (1024 * 1024)) / test_time;  // Mbps
    double loss_rate = 0.0;
    if (packets_sent > 0) {
        loss_rate = (1.0 - (double)packets_received / packets_sent) * 100.0;  // 百分比
    }
    double pps_rate = packets_sent / test_time;  // 每秒包数
    
    // 打印最终结果（带单位）
    printf("\n======== 测试结果 ========\n");
    printf("发送包数: %ld 包\n", packets_sent);
    printf("接收包数: %ld 包\n", packets_received);
    printf("发送字节: %ld 字节\n", bytes_sent);
    printf("测试时间: %.2f 秒\n", test_time);
    printf("吞吐量: %.2f Mbps\n", mbps);
    printf("丢包率: %.2f%%\n", loss_rate);
    printf("PPS: %.2f 包/秒\n", pps_rate);
    
    // 保存结果到CSV文件（只保存四个指定字段）
    save_results_to_csv(DATA_SIZE, mbps, loss_rate, pps_rate);
    
    // 清理资源
    free(buffer);
    close(sockfd);
    
    return 0;
}
