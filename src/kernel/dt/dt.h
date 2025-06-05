#ifndef __DT_H
#define __DT_H

#include <stdbool.h>
#include <stdint.h>
#include <lib/libds/array.h>

#define TOTAL_NODES 30
#define TOTAL_PROPERTY 300
#define FDT_MAX_DEPTH 10

struct property {
    const char *name;
    uint32_t length;
    const void *value;
    struct property *next;
};

struct device_node {
    const char *name;
    const char *type;
    struct property *props;
    struct device_node *parent;
    struct device_node *child;
    struct device_node *sibling;
};

struct of_phandle_args {
    struct device_node *np;
    int args_count;
    uint32_t args[4];
};

typedef int (*dt_config_init_fn)(void);

struct dt_config_descriptor {
    const char *name;
    dt_config_init_fn init;
};

extern struct dt_config_descriptor * volatile __dt_config_registry[];

int dt_init(void);

struct device_node *of_find_node_by_name(struct device_node *from, const char *name);
struct device_node *of_find_compatible_node(struct device_node *from, const char *type, const char *compatible);
struct device_node *of_get_parent(const struct device_node *node);
struct device_node *of_get_next_child(const struct device_node *node, struct device_node *prev);
struct device_node *of_get_next_node(const struct device_node *prev);
struct device_node *of_find_node_by_phandle(uint32_t phandle);
const struct device_node *of_irq_find_parent(const struct device_node *np);

struct property *of_find_property(const struct device_node *np, const char *proname, int *lenp);
int of_property_count_elems_of_size(const struct device_node *np, const char *proname, int elem_size);
int of_property_read_u32_index(const struct device_node *np, const char *proname, uint32_t index, uint32_t *out_value);
int of_property_read_u8_array(const struct device_node *np, const char *proname, uint8_t *out_value, size_t sz);
int of_property_read_u16_array(const struct device_node *np, const char *proname, uint16_t *out_value, size_t sz);
int of_property_read_u32_array(const struct device_node *np, const char *proname, uint32_t *out_value, size_t sz);
int of_property_read_u64_array(const struct device_node *np, const char *proname, uint64_t *out_value, size_t sz);
int of_property_read_u8(const struct device_node *np, const char *proname, uint8_t *out_value);
int of_property_read_u16(const struct device_node *np, const char *proname, uint16_t *out_value);
int of_property_read_u32(const struct device_node *np, const char *proname, uint32_t *out_value);
int of_property_read_u64(const struct device_node *np, const char *proname, uint64_t *out_value);
int of_property_read_string(const struct device_node *np, const char *proname, const char **out_string);
int of_property_read_reg(const struct device_node *np, int index, uint64_t *addr, uint64_t *size);

int of_n_addr_cells(const struct device_node *np);
int of_n_size_cells(const struct device_node *np);
int of_match_device_type(struct device_node *np, const char *type);
bool of_device_is_compatible(const struct device_node *device, const char *compat);
bool is_pci_device(struct device_node *dev);

int of_irq_parse_one(const struct device_node *np, int index, struct of_phandle_args *out_irq);
uint64_t of_translate_address_recursive(struct device_node *dev, const uint32_t *in_addr, int level);
uint64_t of_translate_address(struct device_node *dev, const uint32_t *in_addr);

#define DT_CONFIG_INIT(dev_name, fn) \
    static struct dt_config_descriptor __dt_config_desc_##fn \
        __attribute__((used)) = { \
        .name = dev_name, \
        .init = fn \
    }; \
    ARRAY_SPREAD_ADD(__dt_config_registry, &__dt_config_desc_##fn)

#endif