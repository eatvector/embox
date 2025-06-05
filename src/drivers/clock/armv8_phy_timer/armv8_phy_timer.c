/**
 * @file
 *
 * @date Nov 24, 2020
 * @author Anton Bondarev
 */
#include <errno.h>
#include <stddef.h>
#include <stdint.h>

#include <drivers/common/memory.h>
#include <framework/mod/options.h>
#include <hal/clock.h>
#include <hal/reg.h>
#include <kernel/irq.h>
#include <kernel/time/clock_source.h>
#include <kernel/time/time_device.h>
#include <kernel/dt/dt.h>


static irq_return_t phy_timer_handler(unsigned int irq_nr, void *dev_id) {
	uint32_t freq;

	freq = ARCH_REG_LOAD(CNTFRQ_EL0);

	ARCH_REG_STORE(CNTP_TVAL_EL0, freq / 1000);
	clock_tick_handler(dev_id);
	return IRQ_HANDLED;
}

static int phy_timer_set_periodic(struct clock_source *cs) {
	uint32_t freq;

	freq = ARCH_REG_LOAD(CNTFRQ_EL0);

	ARCH_REG_STORE(CNTP_TVAL_EL0, freq / 1000);
	ARCH_REG_STORE(CNTP_CTL_EL0, CNTP_CTL_EL0_EN);

	return ENOERR;
}

static struct time_event_device phy_timer_event_device = {
    .set_periodic = phy_timer_set_periodic,
    .name = "armv8_phy_timer",
};
#define IRQ_NUM (phy_timer_event_device.irq_nr)


static int phy_timer_init(struct clock_source *cs) {
	return irq_attach(IRQ_NUM, phy_timer_handler, 0, cs, "armv8_phy_timer");
}

CLOCK_SOURCE_DEF(armv8_phy_timer, phy_timer_init, NULL, &phy_timer_event_device,
    NULL);

int armv8_phy_timer_dt_config_init(void){
    struct device_node *np;
    
    np=of_find_compatible_node(NULL, NULL,"arm,armv8-timer");
    if(!np){
        return -1;
    }
   
    struct of_phandle_args irq;
    int irq_index = 0;
    if(of_irq_parse_one(np, irq_index, &irq) == 0){
       phy_timer_event_device.irq_nr=irq.args[1];
    }
     return 0;
}

DT_CONFIG_INIT("pl011_ttys0",armv8_phy_timer_dt_config_init);		
