#include <asf.h>

#include "gfx_mono_ug_2832hsweg04.h"
#include "gfx_mono_text.h"
#include "sysfont.h"



//ECHO E TRIGGER
#define ECHO_PIO      PIOA
#define ECHO_ID   ID_PIOA
#define ECHO_IDX      6
#define ECHO_IDX_MASK (1 << ECHO_IDX)

#define TRIGG_PIO      PIOD
#define TRIGG_ID    ID_PIOD
#define TRIGG_IDX   30
#define TRIGG_IDX_MASK (1 << TRIGG_IDX)




// Botão
#define BUT_PIO      PIOA
#define BUT_PIO_ID   ID_PIOA
#define BUT_IDX  11
#define BUT_IDX_MASK (1 << BUT_IDX)

//Variáveis
volatile int tempo = 0;
volatile char but_flag = 0;
volatile char echo_flag = 0;
volatile double freq = 1/(2*0.000058);
volatile char display_flag = 0;



void io_init(void);
static void RTT_init(float freqPrescale, uint32_t IrqNPulses, uint32_t rttIRQSource);

void but_callback(void){
	but_flag = 1;
}


void callback_echo (void) {
	
	if(!echo_flag) {
		
		RTT_init(freq,0,0);
		echo_flag = 1;
		
	
	}
	else{
		tempo = rtt_read_timer_value(RTT);
		echo_flag = 0;
		display_flag = 1;
		
		 
		
	}
	
}




void RTT_Handler(void) {
	uint32_t ul_status;

	/* Get RTT status - ACK */
	ul_status = rtt_get_status(RTT);

	

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
	//pio_set_output(TRIGG_PIO, TRIGG_IDX_MASK, 0, 0, 0);
	
	
	
	
	pmc_enable_periph_clk(BUT_PIO_ID);

	pio_configure(BUT_PIO, PIO_INPUT, BUT_IDX_MASK, PIO_PULLUP | PIO_DEBOUNCE);
	pio_set_debounce_filter(BUT_PIO, BUT_IDX_MASK, 60);
	

	pio_handler_set(ECHO_PIO,
					ECHO_ID,
					ECHO_IDX_MASK,
					PIO_IT_EDGE,
					callback_echo);
					
	pio_handler_set(BUT_PIO,
					BUT_PIO_ID,
					BUT_IDX_MASK,
					PIO_IT_FALL_EDGE,
					but_callback);
					
	
	
	pio_enable_interrupt(BUT_PIO, BUT_IDX_MASK);
	pio_get_interrupt_status(BUT_PIO);
	
	pio_enable_interrupt(ECHO_PIO, ECHO_IDX_MASK);
	pio_get_interrupt_status(ECHO_PIO);
	
	 
	NVIC_EnableIRQ(ECHO_ID);
	NVIC_SetPriority(ECHO_ID, 1); 
	
	NVIC_EnableIRQ(BUT_PIO_ID);
	NVIC_SetPriority(BUT_PIO_ID, 2);
	
}

void delay_trigg(){
	pio_set(TRIGG_PIO, TRIGG_IDX_MASK);
	delay_us(10);
	pio_clear(TRIGG_PIO, TRIGG_IDX_MASK);
	
}

void display(){
	char str[5];
	double tempo2 = tempo/freq;
	
	double distancia = ((320.0*tempo2)/2.0)*100.0;
	gfx_mono_draw_string("       ", 10,8, &sysfont);
	gfx_mono_draw_string("        ", 70,8, &sysfont);
	sprintf(str, "%.1f", distancia); //
	gfx_mono_draw_string("dist:", 10,8, &sysfont);
	
	
	gfx_mono_draw_string(str, 70,8, &sysfont);
	
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
		
		if(but_flag){
			delay_trigg();
			but_flag = 0;
		}
		
		if(display_flag){
			display();
			display_flag = 0;
			
		}
		
		
		
				
		pmc_sleep(SAM_PM_SMODE_SLEEP_WFI);
	}

	
}
