/**
 * @file
 * @brief
 *
 * @author Aleksey Zhmulin
 * @date 20.10.23
 */
#ifndef DRIVERS_INTERRUPT_GIC_GIC_UTIL_H_
#define DRIVERS_INTERRUPT_GIC_GIC_UTIL_H_

#include <framework/mod/options.h>

struct gic_info{
    uintptr_t gicd_base;
    uintptr_t gicr_base;
    uintptr_t gicc_base;
}gic;

#if OPTION_DEFINED(NUMBER, gicd_base)
#define GICD_BASE (gic.gicd_base)
#endif

#if OPTION_DEFINED(NUMBER, gicr_base)
#define GICR_BASE (gic.gicr_base)
#endif

#if OPTION_DEFINED(NUMBER, gicc_base)
#define GICC_BASE (gic.gicc_base)
#endif

#endif /* DRIVERS_INTERRUPT_GIC_GIC_UTIL_H_ */
