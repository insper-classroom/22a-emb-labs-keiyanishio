#include <asf.h>

#include "gfx_mono_ug_2832hsweg04.h"
#include "gfx_mono_text.h"
#include "sysfont.h"



// LED
#define ECHO_PIO      PIOD
#define ECHO_ID   ID_PIOD
#define ECHO_IDX      30
#define ECHO_IDX_MASK (1 << ECHO_IDX)

#define TRIGG_PIO      PIOC
#define TRIGG_ID    ID_PIOC
#define TRIGG_IDX   13
#define TRIGG_IDX_MASK (1 << TRIGG_IDX)

#define LED_PIO      PIOC
#define LED_PIO_ID   ID_PIOC
#define LED_IDX      8
#define LED_IDX_MASK (1 << LED_IDX)


// Botão
#define BUT_PIO      PIOA
#define BUT_PIO_ID   ID_PIOA
#define BUT_IDX  11
#define BUT_IDX_MASK (1 << BUT_IDX)

double tempo = 0;
volatile char but_flag = 0;
volatile char echo_flag = 0;
volatile float freq = (float) 


void io_init(void);
void pisca_led(int n, int t);
static void RTT_init(float freqPrescale, uint32_t IrqNPulses, uint32_t rttIRQSource);

void but_callback{
	but_flag = 1;
};



void callback_echo (void) {
	if(echo_flag == 1){
		tempo = rtt_read_timer_value(RTT);
		echo_flag = 0;
		
	}
	else if(echo_flag == 0) {
		RTT_init(,,0);
		echo_flag = 1;
		
	}
	
}


void RTT_Handler(void) {
	uint32_t ul_status;

	/* Get RTT status - ACK */
	ul_status = rtt_get_status(RTT);

	/* IRQ due to Alarm */
	if ((ul_status & RTT_SR_ALMS) == RTT_SR_ALMS) {
		//RTT_init(4, 0, RTT_MR_RTTINCIEN);
	}
	
	/* IRQ due to Time has changed */
	if ((ul_status & RTT_SR_RTTINC) == RTT_SR_RTTINC) {
		//pin_toggle(LED_PIO, LED_IDX_MASK);    // BLINK Led
	}

}

void pin_toggle(Pio *pio, uint32_t mask){
	if(pio_get_output_data_status(pio, mask))
	pio_clear(pio, mask);
	else
	pio_set(pio,mask);
}

static float get_time_rtt(){
	uint ul_previous_time = rtt_read_timer_value(RTT);
}

static void RTT_init(float freqPrescale, uint32_t IrqNPulses, uint32_t rttIRQSource) {

	uint16_t pllPreScale = (int) (((float) 32768) / freqPrescale);
	
	rtt_sel_source(RTT, false);
	rtt_init(RTT, pllPreScale);
	
	if (rttIRQSource & RTT_MR_ALMIEN) {
		uint32_t ul_previous_time;
		ul_previous_time = rtt_read_timer_value(RTT);
		while (ul_previous_time == rtt_read_timer_value(RTT));
		rtt_write_alarm_time(RTT, IrqNPulses+ul_previous_time);
	}

	/* config NVIC */
	NVIC_DisableIRQ(RTT_IRQn);
	NVIC_ClearPendingIRQ(RTT_IRQn);
	NVIC_SetPriority(RTT_IRQn, 4);
	NVIC_EnableIRQ(RTT_IRQn);

	/* Enable RTT interrupt */
	if (rttIRQSource & (RTT_MR_RTTINCIEN | RTT_MR_ALMIEN))
	rtt_enable_interrupt(RTT, rttIRQSource);
	else
	rtt_disable_interrupt(RTT, RTT_MR_RTTINCIEN | RTT_MR_ALMIEN);
	
}

	

// Inicializa botao SW0 do kit com interrupcao
void io_init(void)
{

	
	pmc_enable_periph_clk(ECHO_ID);
	pio_configure(ECHO_PIO, PIO_OUTPUT_0, ECHO_IDX_MASK, PIO_DEFAULT);
	pio_set_input(ECHO_PIO, ECHO_IDX_MASK, PIO_DEFAULT);
	
	pmc_enable_periph_clk(TRIGG_ID);
	pio_configure(TRIGG_PIO, PIO_OUTPUT_0, TRIGG_IDX_MASK, PIO_DEFAULT);
	pio_set_output(TRIGG_PIO, TRIGG_IDX_MASK, 0, 0, 0);
	
	
	
	
	pmc_enable_periph_clk(BUT_PIO_ID);

	pio_configure(BUT_PIO, PIO_INPUT, BUT_IDX_MASK, PIO_PULLUP | PIO_DEBOUNCE);
	pio_set_debounce_filter(BUT_PIO, BUT_IDX_MASK, 60);
	

	pio_handler_set(ECHO_PIO,
					ECHO_ID,
					ECHO_IDX_MASK,
					PIO_IT_FALL_EDGE,
					callback_echo);
					
	
	
	pio_enable_interrupt(BUT_PIO, BUT_IDX_MASK);
	pio_get_interrupt_status(BUT_PIO);
	
	pio_enable_interrupt(ECHO_PIO, ECHO_IDX_MASK);
	pio_get_interrupt_status(ECHO_PIO);
	
	 
	NVIC_EnableIRQ(ECHO_ID);
	NVIC_SetPriority(ECHO_ID, 4); 
	
	NVIC_EnableIRQ(BUT_PIO_ID);
	NVIC_SetPriority(BUT_PIO_ID, 4);
	
}

void delay_trigg(){
	pio_set(TRIGG_PIO, TRIGG_IDX_MASK);
	delay_us(10);
	pio_clear(TRIGG_PIO, TRIGG_IDX_MASK);
	
}

void display(double tempo){
	
	double distancia = 340*tempo/2;
	char str[5];
	
	sprintf(str, "%.4lf", distancia); //
	gfx_mono_draw_string("distancia:", 0, 0, &sysfont);

	gfx_mono_draw_string(str, 60, 0, &sysfont);
	
}

int main (void)
{
	board_init();
	sysclk_init();
	io_init();
	delay_init();
	
	
	
	
	
  // Init OLED
	gfx_mono_ssd1306_init();
  
  // Escreve na tela um circulo e um texto
	//gfx_mono_draw_filled_circle(20, 16, 16, GFX_PIXEL_SET, GFX_WHOLE);
  //gfx_mono_draw_string("freq ms", 50,16, &sysfont);
  

  /* Insert application code here, after the board has been initialized. */
	while(1) {
		
		
				
		pmc_sleep(SAM_PM_SMODE_SLEEP_WFI);
	}

	
}
