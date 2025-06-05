/**
 * @file
 * @date 04 aug 2015
 * @author: Anton Bondarev
 */

#include <drivers/common/memory.h>
#include <drivers/serial/uart_dev.h>
#include <drivers/ttys.h>
#include <embox/unit.h>
#include <framework/mod/options.h>
#include <kernel/irq.h>
#include <util/macro.h>
#include <kernel/dt/dt.h>

EMBOX_UNIT_INIT(uart_init);

#define BAUD_RATE OPTION_GET(NUMBER, baud_rate)
#define TTY_NAME  ttyS0

extern irq_return_t uart_irq_handler(unsigned int irq_nr, void *data);

extern const struct uart_ops pl011_uart_ops;

static struct uart uart0 = {
    .dev_name = MACRO_STRING(TTY_NAME),
    .uart_ops = &pl011_uart_ops,
    .params = ((struct uart_params){
        .baud_rate = BAUD_RATE,
        .uart_param_flags = UART_PARAM_FLAGS_8BIT_WORD
                            | UART_PARAM_FLAGS_USE_IRQ,
    }),
};

#define UART_BASE (uart0.base_addr)
#define IRQ_NUM   (uart0.irq_num)


static const struct uart_params uart_defparams = {
    .baud_rate = BAUD_RATE,
    .uart_param_flags = UART_PARAM_FLAGS_8BIT_WORD | UART_PARAM_FLAGS_USE_IRQ,
};

static int uart_init(void) {
	return uart_register(&uart0, &uart_defparams);
}

PERIPH_MEMORY_DEFINE(pl011, 0, 0);
//not used in aarch64-qemu
STATIC_IRQ_ATTACH(IRQ_NUM, uart_irq_handler, &uart0);

TTYS_DEF(TTY_NAME, &uart0);

int pl011_ttys0_dt_config_init(void){
    struct device_node *np;
    uintptr_t uart_base;
    uintptr_t uart_len;

    np=of_find_compatible_node(NULL, NULL,"arm,pl011");
    if(!np){
        return -1;
    }
    if(of_property_read_reg(np, 0, &uart_base, &uart_len)){
        return -1;
    }
     uart0.base_addr=uart_base;
     pl011_mem.start=uart_base;
     pl011_mem.len=0x48;

    struct of_phandle_args irq;
    int irq_index = 0;
    if(of_irq_parse_one(np, irq_index, &irq) == 0){
        uart0.irq_num=irq.args[1];
    }
     return 0;
}

DT_CONFIG_INIT("pl011_ttys0",pl011_ttys0_dt_config_init);
