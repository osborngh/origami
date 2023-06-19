/* Author: Da Vinci
 * This Code is in the Public Domain
 */

#ifndef __COMMON_H__
#define __COMMON_H__

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define OG_CHECK_VK(val, msg) \
	if (val != VK_SUCCESS) \
	printf("[VK_ERROR]: %s [CODE]: %d\n", msg, val); \

#define OG_LOG_ERR(msg) \
	printf("[ERROR]: %s\n", msg); \

#define OG_LOG_INFO(msg) \
	printf("[INFO]: %s\n", msg); \

#define OG_ARR_SIZE(arr) \
	(int)sizeof(arr)/sizeof(arr[0])

#define OG_LOG_INFOVAR(head, msg) \
	printf("[%s]: %s\n", head, msg); \

#define OG_API

#endif //__COMMON_H__
