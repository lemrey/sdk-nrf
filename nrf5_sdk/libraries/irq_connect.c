#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/irq.h>
#include <nrfx_gpiote.h>
#include <zephyr/linker/linker-defs.h>
#include <nrf_sdm.h>

void relocate_vector_table(void)
{
}

static int irq_init(void)
{
	#define VECTOR_ADDRESS ((uintptr_t)_vector_start)

#ifdef CONFIG_NRF5_SDK_IRQ_CONNECT_GPIOTE
        IRQ_CONNECT(GPIOTE_IRQn, 5, nrfx_isr, nrfx_gpiote_0_irq_handler, 0);
#endif

	uint32_t err = sd_softdevice_vector_table_base_set(VECTOR_ADDRESS);

	return (err == NRF_SUCCESS) ? 0 : -EIO;
}

SYS_INIT(irq_init, PRE_KERNEL_1, 0);
