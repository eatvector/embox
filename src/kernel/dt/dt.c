#include <string.h>
#include <assert.h>
#include <lib/libfdt/libfdt.h>
#include <mem/objalloc.h>
#include <kernel/dt/dt.h>

struct device_node *root;

OBJALLOC_DEF(node_allocator, struct device_node, TOTAL_NODES);
OBJALLOC_DEF(property_allocator, struct property, TOTAL_PROPERTY);

static struct property *create_property(const char *name, const void *value, uint32_t length) {
    struct property *prop = objalloc(&property_allocator);
    if (!prop) {
        return NULL;
    }
    
    prop->name = name;
    prop->value = value;
    prop->length = length;
    prop->next = NULL;
    return prop;
}

static struct device_node *create_node(const char *name, const char *type) {
    struct device_node *node = objalloc(&node_allocator);
    if (!node) {
        return NULL;
    }
    node->name = name;
    node->type = type;
    node->props = NULL;
    node->parent = NULL;
    node->child = NULL;
    node->sibling = NULL;
    return node;
}

static void add_property(struct device_node *node, struct property *prop) {
    if (!node || !prop) {
        return;
    }
    prop->next = node->props;
    node->props = prop;
}

static void add_child_node(struct device_node *parent, struct device_node *child) {
    if (!parent || !child) {
        return;
    }
    child->parent = parent;
    child->sibling = parent->child;
    parent->child = child;
}

static struct device_node *fdt_parse_node(const void *fdt, int nodeoffset, struct device_node *parent) {
    const char *name = fdt_get_name(fdt, nodeoffset, NULL);
    struct device_node *node = create_node(name, NULL);
    if (!node) {
        return NULL;
    }

    if (parent) {
        add_child_node(parent, node);
    }

    int prop_offset;
    fdt_for_each_property_offset(prop_offset, fdt, nodeoffset) {
        const struct fdt_property *prop = fdt_get_property_by_offset(fdt, prop_offset, NULL);
        const char *prop_name = fdt_string(fdt, fdt32_to_cpu(prop->nameoff));
        const void *prop_value = prop->data;
        struct property *new_prop = create_property(prop_name, prop_value, fdt32_to_cpu(prop->len));
        if (new_prop) {
            add_property(node, new_prop);
        }
    }

    int subnode;
    fdt_for_each_subnode(subnode, fdt, nodeoffset) {
        fdt_parse_node(fdt, subnode, node);
    }

    return node;
}

static struct device_node *fdt_to_tree(const void *fdt) {
    if (fdt_check_header(fdt)) {
        return NULL;
    }

    int root_offset = fdt_path_offset(fdt, "/");
    if (root_offset < 0) {
        return NULL;
    }

    return fdt_parse_node(fdt, root_offset, NULL);
}

ARRAY_SPREAD_DEF(struct dt_config_descriptor *, __dt_config_registry);

static int dt_config_call_all(void) {
    int ret = 0;
    struct dt_config_descriptor *desc;
    
    array_spread_foreach(desc, __dt_config_registry) {
        if (desc && desc->init) {
            ret = desc->init();
            if (ret) {
            }
        }
    }
    
    return ret;
}

int dt_init(void){
    extern char _dtb_start;
    root = fdt_to_tree(&_dtb_start);
    if (!root) {
        return -1;
    }
    dt_config_call_all();
    return 0;
}

struct device_node *of_find_node_by_name(struct device_node *from, const char *name) {
    if (!name) return NULL;
    struct device_node *node = from ? from : root;
    
    if (node && strcmp(node->name, name) == 0) {
        return node;
    }
    
    if (node && node->child) {
        struct device_node *found = of_find_node_by_name(node->child, name);
        if (found) return found;
    }
    
    if (node && node->sibling) {
        struct device_node *found = of_find_node_by_name(node->sibling, name);
        if (found) return found;
    }
    
    return NULL;
}

struct device_node *of_find_compatible_node(struct device_node *from, const char *type, const char *compatible) {
    if (!compatible) {
        return NULL;
    }
    struct device_node *node = from ? from : root;
    
    if (node) {
        const char *comp = NULL;
        of_property_read_string(node, "compatible", &comp);
        if (comp && strstr(comp, compatible) != NULL) {
            return node;
        }
    }
    
    if (node && node->child) {
        struct device_node *found = of_find_compatible_node(node->child, type, compatible);
        if (found) {
            return found;
        }
    }
    
    if (node && node->sibling) {
        struct device_node *found = of_find_compatible_node(node->sibling, type, compatible);
        if (found) {
            return found;
        }
    }
    
    return NULL;
}

int of_match_device_type(struct device_node *np, const char *type) {
    if (!np || !type) {
        return -1;
    }
    const char *device_type = NULL;
    if (of_property_read_string(np, "device_type", &device_type) != 0) {
        return 0;
    }
    return strcmp(device_type, type) == 0;
}

struct device_node *of_get_parent(const struct device_node *node) {
    return node ? node->parent : NULL;
}

struct device_node *of_get_next_child(const struct device_node *node, struct device_node *prev) {
    if (!node) {
        return NULL;
    }
    if (!prev) {
        return node->child;
    } else {
        return prev->sibling;
    }
}

struct property *of_find_property(const struct device_node *np, const char *proname, int *lenp) {
    if (!np || !proname) {
        return NULL;
    }
    struct property *prop = np->props;
    while (prop) {
        if (strcmp(prop->name, proname) == 0) {
            if (lenp) {
                *lenp = prop->length;
            }
            return prop;
        }
        prop = prop->next;
    }
    return NULL;
}

int of_property_count_elems_of_size(const struct device_node *np, const char *proname, int elem_size) {
    if (!np || !proname || elem_size <= 0) {
        return -1;
    }
    struct property *prop = of_find_property(np, proname, NULL);
    if (!prop || !prop->value) {
        return -1;
    }
    return prop->length / elem_size;
}

int of_property_read_u32_index(const struct device_node *np, const char *proname, uint32_t index, uint32_t *out_value) {
    if (!np || !proname || !out_value) {
        return -1;
    }
    struct property *prop = of_find_property(np, proname, NULL);
    if (!prop || !prop->value) {
        return -1;
    }
    if ((index + 1) * sizeof(uint32_t) > prop->length) {
        return -1;
    }
    *out_value = fdt32_to_cpu(((uint32_t *)prop->value)[index]);
    return 0;
}

int of_property_read_u8_array(const struct device_node *np, const char *proname, uint8_t *out_value, size_t sz) {
    if (!np || !proname || !out_value) {
        return -1;
    }
    struct property *prop = of_find_property(np, proname, NULL);
    if (!prop || !prop->value) {
        return -1;
    }
    size_t count = prop->length;
    if (count > sz) {
        count = sz;
    }
    memcpy(out_value, prop->value, count);
    return count;
}

int of_property_read_u16_array(const struct device_node *np, const char *proname, uint16_t *out_value, size_t sz) {
    if (!np || !proname || !out_value) {
        return -1;
    }
    struct property *prop = of_find_property(np, proname, NULL);
    if (!prop || !prop->value) {
        return -1;
    }
    size_t count = prop->length / sizeof(uint16_t);
    if (count > sz) {
        count = sz;
    }
    for (size_t i = 0; i < count; i++) {
        out_value[i] = fdt16_to_cpu(((uint16_t *)prop->value)[i]);
    }
    return count;
}

int of_property_read_u32_array(const struct device_node *np, const char *proname, uint32_t *out_value, size_t sz) {
    if (!np || !proname || !out_value) {
        return -1;
    }
    struct property *prop = of_find_property(np, proname, NULL);
    if (!prop || !prop->value) {
        return -1;
    }
    size_t count = prop->length / sizeof(uint32_t);
    if (count > sz) {
        count = sz;
    }
    for (size_t i = 0; i < count; i++) {
        out_value[i] = fdt32_to_cpu(((uint32_t *)prop->value)[i]);
    }
    return count;
}

int of_property_read_u64_array(const struct device_node *np, const char *proname, uint64_t *out_value, size_t sz) {
    if (!np || !proname || !out_value) {
        return -1;
    }
    struct property *prop = of_find_property(np, proname, NULL);
    if (!prop || !prop->value) {
        return -1;
    }
    size_t count = prop->length / sizeof(uint64_t);
    if (count > sz) {
        count = sz;
    }
    for (size_t i = 0; i < count; i++) {
        out_value[i] = fdt64_to_cpu(((uint64_t *)prop->value)[i]);
    }
    return count;
}

int of_property_read_u8(const struct device_node *np, const char *proname, uint8_t *out_value) {
    return of_property_read_u8_array(np, proname, out_value, 1);
}

int of_property_read_u16(const struct device_node *np, const char *proname, uint16_t *out_value) {
    return of_property_read_u16_array(np, proname, out_value, 1);
}

int of_property_read_u32(const struct device_node *np, const char *proname, uint32_t *out_value) {
    return of_property_read_u32_array(np, proname, out_value, 1);
}

int of_property_read_u64(const struct device_node *np, const char *proname, uint64_t *out_value) {
    return of_property_read_u64_array(np, proname, out_value, 1);
}

int of_property_read_string(const struct device_node *np, const char *proname, const char **out_string) {
    if (!np || !proname || !out_string) {
        return -1;
    }
    struct property *prop = of_find_property(np, proname, NULL);
    if (!prop || !prop->value) {
        return -1;
    }
    if (memchr(prop->value, '\0', prop->length) == NULL) {
        return -1;
    }
    *out_string = (const char *)prop->value;
    return 0;
}

int of_n_addr_cells(const struct device_node *np) {
    uint32_t cells = 2;
    while (np) {
        if (of_property_read_u32(np, "#address-cells", &cells)!=1) {
            break;
        }
        np = np->parent;
    }
    return cells;
}

int of_n_size_cells(const struct device_node *np) {
    uint32_t cells = 1;
    while (np) {
        if (of_property_read_u32(np, "#size-cells", &cells)!=1) {
            break;
        }
        np = np->parent;
    }
    return cells;
}

bool of_device_is_compatible(const struct device_node *device, const char *compat) {
    if (!device || !compat) {
        return false;
    }
    const char *comp = NULL;
    if (of_property_read_string(device, "compatible", &comp) != 0)
        return false;
    
    const char *start = comp;
    const char *end;
    
    while (*start) {
        end = start;
        while (*end && *end != ',') end++;
        if (end - start == strlen(compat) && 
            strncmp(start, compat, end - start) == 0) {
            return true;
        }
        if (!*end) {
            break;
        }
        start = end + 1;
    }
    return false;
}

int of_property_read_reg(const struct device_node *np, int index, uint64_t *addr, uint64_t *size) {
    if (!np || !addr || !size) {
        return -1;
    }

    struct device_node *f_np=of_get_parent(np);
    if(!f_np){
        return -1;
    }

    int addr_cells = of_n_addr_cells(f_np);
    int size_cells = of_n_size_cells(f_np);
    int reg_cell_count = addr_cells + size_cells;
    if (reg_cell_count <= 0) {
        return -1;
    }

    struct property *prop = of_find_property(np, "reg", NULL);
    if (!prop || !prop->value) {
        return -1;
    }

    int total_cells = prop->length / sizeof(uint32_t);
    if (total_cells < (index + 1) * reg_cell_count) {
        return -1;
    }

    uint32_t *reg_data = (uint32_t *)prop->value;
    uint32_t *entry = &reg_data[index * reg_cell_count];

    *addr = 0;
    for (int i = 0; i < addr_cells; i++) {
        *addr = (*addr << 32) | fdt32_to_cpu(entry[i]);
    }

    *size = 0;
    for (int i = 0; i < size_cells; i++) {
        *size = (*size << 32) | fdt32_to_cpu(entry[addr_cells + i]);
    }
    return 0;
}

struct device_node *of_get_next_node(const struct device_node *prev) {
    if (!prev) {
        return root;
    }

    if (prev->child) {
        return prev->child;
    }

    if (prev->sibling) {
        return prev->sibling;
    }

    while (prev->parent) {
        prev = prev->parent;
        if (prev->sibling) {
            return prev->sibling;
        }
    }

    return NULL;
}

struct device_node *of_find_node_by_phandle(uint32_t phandle) {
    struct device_node *node = root;
    while (node) {
        struct property *prop = of_find_property(node, "phandle", NULL);
        if (prop && prop->value && *(uint32_t *)prop->value == phandle) {
            return node;
        }
        node = of_get_next_node(node);
    }
    return NULL;
}

const struct device_node* of_irq_find_parent(const struct device_node *np){
    const struct device_node*parent=np;
    if(!np){
        return NULL;
    }

    while (parent) {
        struct property *pprop = of_find_property(parent, "interrupt-parent", NULL);
        if (pprop && pprop->value) {
            break;
        }
        parent = parent->parent;
    }
    return parent;
}

int of_irq_parse_one(const struct device_node *np, int index,
                    struct of_phandle_args *out_irq) {
    struct property *prop = of_find_property(np, "interrupts", NULL);
    if (!prop || !prop->value) {
        return -1;
    }

    const struct device_node *parent = of_irq_find_parent(np);
    if(!parent){
        return -1;
    }

    struct property *pprop = of_find_property(parent, "interrupt-parent", NULL);
    uint32_t parent_phandle=*(uint32_t *)pprop->value;
    if (!parent_phandle) {
        return -1;
    }

    out_irq->np = of_find_node_by_phandle(parent_phandle);
    if (!out_irq->np) {
        return -1;
    }

    struct property *cells_prop = of_find_property(out_irq->np, "#interrupt-cells", NULL);
    if (!cells_prop || !cells_prop->value) {
        return -1;
    }
    out_irq->args_count = fdt32_to_cpu(*(uint32_t *)cells_prop->value);

    uint32_t *int_data = (uint32_t *)prop->value;
    int cell_size = out_irq->args_count;
    int total_cells = prop->length / sizeof(uint32_t);

    if (index >= total_cells / cell_size) {
        return -1;
    }

    uint32_t *entry = &int_data[index * cell_size];
    for (int i = 0; i < out_irq->args_count; i++) {
        out_irq->args[i] = fdt32_to_cpu(entry[i]);
    }

    return 0;
}


bool is_pci_device(struct device_node *dev) {
    bool is_pci_device = of_match_device_type(dev, "pci") || 
                         of_match_device_type(dev, "pcie");
    return is_pci_device;
}

static uint64_t parse_pci_address(const uint32_t *in_addr) {
    return ((uint64_t)in_addr[0] << 32) | (in_addr[1] << 16) | in_addr[2];
}

uint64_t of_translate_address_recursive(struct device_node *dev, const uint32_t *in_addr, int level) {
    uint64_t child_addr = 0;
    int child_addr_cells;
    struct device_node *parent;
    
    if (!dev || !in_addr || level > FDT_MAX_DEPTH) {
        return 0;
    }

    child_addr_cells = of_n_addr_cells(dev);
    bool is_pci = is_pci_device(dev);
    if (is_pci && child_addr_cells == 3) {
        child_addr = parse_pci_address(in_addr);
    } else {
        for (int i = 0; i < child_addr_cells; i++) {
            child_addr = (child_addr << 32) | in_addr[i];
        }
    }

    parent = of_get_parent(dev);
    if (!parent) {
        return child_addr;
    }

    struct property *ranges;
    int ranges_len;
    ranges = of_find_property(dev, "ranges", &ranges_len);

    if (!ranges) {
        return 0;
    }

    if (ranges_len == 0) {
        return of_translate_address_recursive(parent, in_addr, level + 1);
    }

    int parent_addr_cells = of_n_addr_cells(parent);
    int child_size_cells = of_n_size_cells(dev);
    const uint32_t *ranges_data = (const uint32_t *)ranges->value;
    int tuple_size = child_addr_cells + parent_addr_cells + child_size_cells;
    int tuple_count = ranges_len / (4 * tuple_size);
    
    for (int i = 0; i < tuple_count; i++) {
        const uint32_t *tuple = &ranges_data[i * tuple_size];
        uint64_t child_base = 0;
        if (is_pci && child_addr_cells == 3) {
            child_base = parse_pci_address(tuple);
        } else {
            for (int j = 0; j < child_addr_cells; j++) {
                child_base = (child_base << 32) | tuple[j];
            }
        }

        uint64_t parent_base = 0;
        for (int j = 0; j < parent_addr_cells; j++) {
            parent_base = (parent_base << 32) | tuple[child_addr_cells + j];
        }

        uint64_t size = 0;
        for (int j = 0; j < child_size_cells; j++) {
            size = (size << 32) | tuple[child_addr_cells + parent_addr_cells + j];
        }

        if (is_pci) {
            uint64_t child_addr_masked = child_addr & 0x00FFFFFF;
            uint64_t child_base_masked = child_base & 0x00FFFFFF;
            if (child_addr_masked >= child_base_masked && 
                child_addr_masked < child_base_masked + size) {
                uint64_t translated = parent_base + (child_addr_masked - child_base_masked);
                uint32_t parent_addr[2] = {0};
                if (parent_addr_cells == 1) {
                    parent_addr[0] = (uint32_t)translated;
                } else if (parent_addr_cells == 2) {
                    parent_addr[0] = (uint32_t)(translated >> 32);
                    parent_addr[1] = (uint32_t)translated;
                }
                return of_translate_address_recursive(parent, parent_addr, level + 1);
            }
        } else {
            if (child_addr >= child_base && child_addr < child_base + size) {
                uint64_t translated = parent_base + (child_addr - child_base);
                uint32_t parent_addr[2] = {0};
                if (parent_addr_cells == 1) {
                    parent_addr[0] = (uint32_t)translated;
                } else if (parent_addr_cells == 2) {
                    parent_addr[0] = (uint32_t)(translated >> 32);
                    parent_addr[1] = (uint32_t)translated;
                }
                return of_translate_address_recursive(parent, parent_addr, level + 1);
            }
        }
    }

    return 0;
}

uint64_t of_translate_address(struct device_node *dev, const uint32_t *in_addr) {
    return of_translate_address_recursive(dev, in_addr, 0);
}