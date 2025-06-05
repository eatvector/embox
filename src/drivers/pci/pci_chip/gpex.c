/**
 * @file gpex.c
 * @brief Driver for Generic PCI Express Bridge Emulator. Refer to
 * http://www.kernel.org/doc/Documentation/devicetree/bindings/pci/host-generic-pci.txt
 * for more details
 *
 * @author Denis Deryugin <deryugin.denis@gmail.com>
 * @version
 * @date 14.08.2019
 */

#include <assert.h>
#include <stdint.h>
#include <string.h>

#include <drivers/common/memory.h>
#include <drivers/pci/pci.h>
#include <drivers/pci/pci_chip/pci_utils.h>
#include <hal/reg.h>
#include <util/log.h>
#include <kernel/dt/dt.h>

extern struct pci_info pci;

#define GPEX_MMIO_BASE (pci.base_addr)
#define GPEX_IRQ_BASE  (pci.irq_base)
#define GPEX_IRQ_NUM   4 /* GPEX_IRQ_BASE + 0, ... GPEX_IRQ_BASE + 3 */

#define PCI_REG_ADDR(bus, physdev, fun, where) \
	(((where) & ~3) | ((fun) << 12) | ((physdev) << 15) | ((bus) << 20))

static uint32_t gpex_read_config(uint32_t bus, uint32_t dev_fn,
				uint32_t where, void *value, size_t size) {
	uint32_t tmp;
	uintptr_t addr;

	assert(value);

	log_debug("bus=%d dev_fn=%d where=%d size=%d",
			bus, dev_fn, where, size);

	addr = PCI_REG_ADDR(bus, dev_fn >> 3, dev_fn & 0x7, where) + GPEX_MMIO_BASE;

	log_debug("read addr %p", (void *) addr);

	switch (size) {
	case 1:
		tmp = REG8_LOAD(addr);
		break;
	case 2:
		tmp = REG16_LOAD(addr);
		break;
	case 4:
		tmp = REG32_LOAD(addr);
		break;
	default:
		log_error("Wrong PCI config read size: %d", size);
	}

	log_debug("value=0x%x", tmp);

	memcpy(value, &tmp, size);

	return PCIUTILS_SUCCESS;
}

uint32_t pci_read_config8(uint32_t bus, uint32_t dev_fn,
				uint32_t where, uint8_t *value) {
	return gpex_read_config(bus, dev_fn, where, value, sizeof(uint8_t));
}

uint32_t pci_read_config16(uint32_t bus, uint32_t dev_fn,
				uint32_t where, uint16_t *value) {
	return gpex_read_config(bus, dev_fn, where, value, sizeof(uint16_t));
}

uint32_t pci_read_config32(uint32_t bus, uint32_t dev_fn,
				uint32_t where, uint32_t *value) {
	return gpex_read_config(bus, dev_fn, where, value, sizeof(uint32_t));
}

static uint32_t gpex_write_config(uint32_t bus, uint32_t dev_fn,
				uint32_t where, void *value, size_t size) {
	uintptr_t addr;

	assert(value);

	log_debug("bus=%d dev_fn=%d where=%d size=%d",
			bus, dev_fn, where, size);

	addr = PCI_REG_ADDR(bus, dev_fn >> 3, dev_fn & 0x7, where) + GPEX_MMIO_BASE;

	log_debug("write addr %p", (void *) addr);

	switch (size) {
	case 1:
		REG8_STORE(addr, *((uint8_t *) value));
		break;
	case 2:
		REG16_STORE(addr, *((uint16_t *) value));
		break;
	case 4:
		REG32_STORE(addr, *((uint32_t *) value));
		break;
	default:
		log_error("Wrong PCI config read size: %d", size);
	}

	return PCIUTILS_SUCCESS;
}

uint32_t pci_write_config8(uint32_t bus, uint32_t dev_fn,
				uint32_t where, uint8_t value) {
	return gpex_write_config(bus, dev_fn, where, &value, sizeof(uint8_t));
}

uint32_t pci_write_config16(uint32_t bus, uint32_t dev_fn,
				uint32_t where, uint16_t value) {
	return gpex_write_config(bus, dev_fn, where, &value, sizeof(uint16_t));
}

uint32_t pci_write_config32(uint32_t bus, uint32_t dev_fn,
				uint32_t where,	uint32_t value) {
	return gpex_write_config(bus, dev_fn, where, &value, sizeof(uint32_t));
}

unsigned int pci_irq_number(struct pci_slot_dev *dev) {
	return (unsigned int) (GPEX_IRQ_BASE + (dev->irq_pin + 1) % GPEX_IRQ_NUM);
}

PERIPH_MEMORY_DEFINE(gpex_mmio, 0, 0);

int gpex_pci_dt_config_init(void){
    struct device_node *np;
    uintptr_t gpex_base;
    uintptr_t gpex_len;

    np=of_find_compatible_node(NULL, NULL,"pci-host-ecam-generic");
    if(!np){
        return -1;
    }
    if(of_property_read_reg(np, 0, &gpex_base, &gpex_len)){
        return -1;
    }
    pci.base_addr = gpex_base;

    gpex_mmio_mem.start = gpex_base;
	gpex_mmio_mem.len = 0x2EFF0000;


    struct of_phandle_args irq;
    int irq_index = 0;
    if(of_irq_parse_one(np, irq_index, &irq) == 0){
        pci.irq_base=irq.args[1];
    }else{
		return -1;
	}
    
	short bus_num = 0;
    uint32_t bus_range[2];
	of_property_read_u32_array(np, "bus-range", bus_range, 2);
	bus_num=bus_range[1]-bus_range[0]+1;
    pci.bus_num=bus_num;
    return 0;
}

DT_CONFIG_INIT("gpex",gpex_pci_dt_config_init);


