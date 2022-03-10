#include <asf.h>

#include "gfx_mono_ug_2832hsweg04.h"
#include "gfx_mono_text.h"
#include "sysfont.h"

#define OLED_PIO1       PIOA
#define OLED_PIO1_ID    ID_PIOA
#define OLED_PIO1_IDX   0
#define OLED_PIO1_IDX_MASK (1 << OLED_PIO1_IDX)

#define BUT_PIO1       PIOD
#define BUT_PIO1_ID    ID_PIOD
#define BUT_PIO1_IDX   28
#define BUT_PIO1_IDX_MASK (1u << BUT_PIO1_IDX)


// LED
#define LED_PIO      PIOC
#define LED_PIO_ID   ID_PIOC
#define LED_IDX      8
#define LED_IDX_MASK (1 << LED_IDX)

// Botão
#define BUT_PIO      PIOA
#define BUT_PIO_ID   ID_PIOA
#define BUT_IDX  11
#define BUT_IDX_MASK (1 << BUT_IDX)

void io_init(void);
void pisca_led(int n, int t);
volatile char but_flag;
volatile char but_flag1_press;
volatile char but_flag1_up;

void but_callback (void) {
	but_flag =1;
}

void but_callback1 (void) {
	if (pio_get(BUT_PIO1, PIO_INPUT, BUT_PIO1_IDX_MASK)) {
		but_flag1_press = 0;
		but_flag1_up = 1;
		
		} else {
		but_flag1_press= 1;
	}
}

void pisca_led(int n, int t){
	for (int i=0;i<n;i++){
		pio_clear(LED_PIO, LED_IDX_MASK);
		delay_ms(t);
		pio_set(LED_PIO, LED_IDX_MASK);
		delay_ms(t);
	}
}

void display(int delay){
	int periodo = 2*delay*0.001;
	double freq = 1/periodo;
	char fstr[128];


	sprintf(fstr, "%f", freq); //
	gfx_mono_draw_string("freq:", 0, 0, &sysfont);

	gfx_mono_draw_string(fstr, 10, 0, &sysfont);
}

	

// Inicializa botao SW0 do kit com interrupcao
void io_init(void)
{

	// Configura led
	
	pmc_enable_periph_clk(LED_PIO_ID);
	pio_configure(LED_PIO, PIO_OUTPUT_0, LED_IDX_MASK, PIO_DEFAULT);
	
	pmc_enable_periph_clk(OLED_PIO1);
	pio_configure(OLED_PIO1, PIO_OUTPUT_0, OLED_PIO1_IDX_MASK, PIO_DEFAULT);

	// Inicializa clock do periférico PIO responsavel pelo botao
	pmc_enable_periph_clk(BUT_PIO_ID);
	
	pmc_enable_periph_clk(BUT_PIO1_ID);

	// Configura PIO para lidar com o pino do botão como entrada
	// com pull-up
	pio_configure(BUT_PIO, PIO_INPUT, BUT_IDX_MASK, PIO_PULLUP | PIO_DEBOUNCE);
	pio_set_debounce_filter(BUT_PIO, BUT_IDX_MASK, 60);
	
	pio_configure(BUT_PIO1, PIO_INPUT, BUT_PIO1_IDX_MASK, PIO_PULLUP | PIO_DEBOUNCE);
	pio_set_debounce_filter(BUT_PIO1, BUT_PIO1_IDX_MASK, 60);

	// Configura interrupção no pino referente ao botao e associa
	// função de callback caso uma interrupção for gerada
	// a função de callback é a: but_callback()
	pio_handler_set(BUT_PIO1,
					BUT_PIO1_ID,
					BUT_PIO1_IDX_MASK,
					PIO_IT_EDGE,
					but_callback1);
					
	pio_handler_set(BUT_PIO,
					BUT_PIO_ID,
					BUT_IDX_MASK,
					PIO_IT_FALL_EDGE,
					but_callback);

	// Ativa interrupção e limpa primeira IRQ gerada na ativacao
	pio_enable_interrupt(BUT_PIO1, BUT_PIO1_IDX_MASK);
	pio_get_interrupt_status(BUT_PIO1);
	
	pio_enable_interrupt(BUT_PIO, BUT_IDX_MASK);
	pio_get_interrupt_status(BUT_PIO);
	
	
	// Configura NVIC para receber interrupcoes do PIO do botao
	// com prioridade 4 (quanto mais próximo de 0 maior)
	NVIC_EnableIRQ(BUT_PIO1_ID);
	NVIC_SetPriority(BUT_PIO1_ID, 4); // Prioridade 4
	
	NVIC_EnableIRQ(BUT_PIO_ID);
	NVIC_SetPriority(BUT_PIO_ID, 4);
}

int main (void)
{
	board_init();
	sysclk_init();
	io_init();
	delay_init();
	int delay = 200;
	int cont = 0;
	
	
	
	
  // Init OLED
	gfx_mono_ssd1306_init();
  
  // Escreve na tela um circulo e um texto
	//gfx_mono_draw_filled_circle(20, 16, 16, GFX_PIXEL_SET, GFX_WHOLE);
  //gfx_mono_draw_string("freq ms", 50,16, &sysfont);
  

  /* Insert application code here, after the board has been initialized. */
	while(1) {
		while (but_flag1_press){
			cont++;
		}
		if (but_flag1_up){
			if (cont>30000){
				delay-=100;
				display(delay);
			}
			else{
				delay+=100;
				
				display(delay);
			}
			
			but_flag1_up = 0;
			cont=0;	
			
		}
		if (but_flag){
			pisca_led(30, delay);
		}
		
		
		pmc_sleep(SAM_PM_SMODE_SLEEP_WFI);
	}

	
}
