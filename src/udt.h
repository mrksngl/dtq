#ifndef _UDT_H
#define _UDT_H

#include <stdint.h>

enum PROPERTY_TYPE {
	PROPERTY_TYPE_INTEGER = 1,
	PROPERTY_TYPE_STRING  = 2,
	PROPERTY_TYPE_ARRAY   = 4
};

struct Property {
	enum PROPERTY_TYPE type;
	char * name;
	char * val;
	int len;
	struct Property * nextProperty;
};

struct Node {
	char * name;
	struct Property * properties;
	struct Node * parent;
	struct Node * children;
	struct Node * sibling;
};

struct DeviceTree {
	struct Node * root;
	uint32_t boot_cpuid_phys;
};

struct DeviceTree * unflattenDeviceTree(const void * fdt);

void printDeviceTree(const struct DeviceTree * dt);

void freeDeviceTree(struct DeviceTree * dt);

#endif
