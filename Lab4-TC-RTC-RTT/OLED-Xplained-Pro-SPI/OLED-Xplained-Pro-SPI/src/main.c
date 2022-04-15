#include <asf.h>

#include "gfx_mono_ug_2832hsweg04.h"
#include "gfx_mono_text.h"
#include "sysfont.h"

typedef struct  {
	uint32_t year;
	uint32_t month;
	uint32_t day;
	uint32_t week;
	uint32_t hour;
	uint32_t minute;
	uint32_t second;
} calendar;


// LED
#define LED_PIO      PIOC
#define LED_PIO_ID   ID_PIOC
#define LED_IDX      8
#define LED_IDX_MASK (1 << LED_IDX)

#define OLED1_PIO       PIOA
#define OLED1_PIO_ID    ID_PIOA
#define OLED1_PIO_IDX   0
#define OLED1_PIO_IDX_MASK (1 << OLED1_PIO_IDX)

#define OLED2_PIO       PIOC
#define OLED2_PIO_ID    ID_PIOC
#define OLED2_PIO_IDX   30
#define OLED2_PIO_IDX_MASK (1 << OLED2_PIO_IDX)

#define OLED3_PIO       PIOB
#define OLED3_PIO_ID    ID_PIOB
#define OLED3_PIO_IDX   2
#define OLED3_PIO_IDX_MASK (1 << OLED3_PIO_IDX)

// Botão


#define BUT1_PIO	      PIOD
#define BUT1_PIO_ID    ID_PIOD
#define BUT1_PIO_IDX   28
#define BUT1_PIO_IDX_MASK (1u << BUT1_PIO_IDX)





void LED_init(int estado);
void TC_init(Tc * TC, int ID_TC, int TC_CHANNEL, int freq);
void pin_toggle(Pio *pio, uint32_t mask);
void io_init(void);
static void RTT_init(float freqPrescale, uint32_t IrqNPulses, uint32_t rttIRQSource);
void RTC_init(Rtc *rtc, uint32_t id_rtc, calendar t, uint32_t irq_type);
void pisca_led(int n, int t);

volatile char but_flag1_press = 0;
volatile char but_flag1_up = 0;
volatile char flag_rtc_alarm = 0;
volatile char flag_cronometro = 0;

volatile int segundo = 0;
volatile int minuto = 0;
volatile int hora = 0;

void but_callback1 (void) {
	but_flag1_press= 1;
}

void io_init(void)
{

	// Configura led
	
	pmc_enable_periph_clk(LED_PIO_ID);
	pio_configure(LED_PIO, PIO_OUTPUT_0, LED_IDX_MASK, PIO_DEFAULT);
	
	
	pmc_enable_periph_clk(OLED2_PIO_ID);
	pio_configure(OLED2_PIO, PIO_OUTPUT_1, OLED2_PIO_IDX_MASK, PIO_DEFAULT);
	
	

	// Inicializa clock do periférico PIO responsavel pelo botao
	
	pmc_enable_periph_clk(BUT1_PIO_ID);
	
	


	// Configura PIO para lidar com o pino do botão como entrada
	// com pull-up
	
	
	pio_configure(BUT1_PIO, PIO_INPUT, BUT1_PIO_IDX_MASK, PIO_PULLUP | PIO_DEBOUNCE);
	pio_set_debounce_filter(BUT1_PIO, BUT1_PIO_IDX_MASK, 60);
	

	// Configura interrupção no pino referente ao botao e associa
	// função de callback caso uma interrupção for gerada
	// a função de callback é a: but_callback()
	pio_handler_set(BUT1_PIO,
					BUT1_PIO_ID,
					BUT1_PIO_IDX_MASK,
					PIO_IT_EDGE,
					but_callback1);
	
	

	// Ativa interrupção e limpa primeira IRQ gerada na ativacao
	pio_enable_interrupt(BUT1_PIO, BUT1_PIO_IDX_MASK);
	pio_get_interrupt_status(BUT1_PIO);
	
	
	// Configura NVIC para receber interrupcoes do PIO do botao
	// com prioridade 4 (quanto mais próximo de 0 maior)
	NVIC_EnableIRQ(BUT1_PIO_ID);
	NVIC_SetPriority(BUT1_PIO_ID, 4); // Prioridade 4
	
}


void TC1_Handler(void) {
	/**
	* Devemos indicar ao TC que a interrupção foi satisfeita.
	* Isso é realizado pela leitura do status do periférico
	**/
	volatile uint32_t status = tc_get_status(TC0, 1);

	/** Muda o estado do LED (pisca) **/
	pin_toggle(OLED1_PIO, OLED1_PIO_IDX_MASK);  
}

void TC2_Handler(void) {
	/**
	* Devemos indicar ao TC que a interrupção foi satisfeita.
	* Isso é realizado pela leitura do status do periférico
	**/
	volatile uint32_t status = tc_get_status(TC0, 2);

	/** Muda o estado do LED (pisca) **/
	pin_toggle(LED_PIO, LED_IDX_MASK);  
}

void TC0_Handler(void) {
	/**
	* Devemos indicar ao TC que a interrupção foi satisfeita.
	* Isso é realizado pela leitura do status do periférico
	**/
	volatile uint32_t status = tc_get_status(TC0, 0);

	flag_cronometro = 1;  
}

void RTT_Handler(void) {
	uint32_t ul_status;

	/* Get RTT status - ACK */
	ul_status = rtt_get_status(RTT);

	/* IRQ due to Alarm */
	if ((ul_status & RTT_SR_ALMS) == RTT_SR_ALMS) {
		RTT_init(4, 0, RTT_MR_RTTINCIEN);
	}
	
	/* IRQ due to Time has changed */
	if ((ul_status & RTT_SR_RTTINC) == RTT_SR_RTTINC) {
		pin_toggle(OLED2_PIO, OLED2_PIO_IDX_MASK);    // BLINK Led
	}

}

void RTC_Handler(void) {
	uint32_t ul_status = rtc_get_status(RTC);
	
	/* seccond tick */
	if ((ul_status & RTC_SR_SEC) == RTC_SR_SEC) {
		// o código para irq de segundo vem aqui
	}
	
	/* Time or date alarm */
	if ((ul_status & RTC_SR_ALARM) == RTC_SR_ALARM) {
		// o código para irq de alame vem aqui
		flag_rtc_alarm = 1;
	}

	rtc_clear_status(RTC, RTC_SCCR_SECCLR);
	rtc_clear_status(RTC, RTC_SCCR_ALRCLR);
	rtc_clear_status(RTC, RTC_SCCR_ACKCLR);
	rtc_clear_status(RTC, RTC_SCCR_TIMCLR);
	rtc_clear_status(RTC, RTC_SCCR_CALCLR);
	rtc_clear_status(RTC, RTC_SCCR_TDERRCLR);
}

void pisca_led (int n, int t) {
	for (int i=0;i<n;i++){
		pio_clear(OLED3_PIO, OLED3_PIO_IDX_MASK);
		delay_ms(t);
		pio_set(OLED3_PIO, OLED3_PIO_IDX_MASK);
		delay_ms(t);
	}
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


/**
* @Brief Inicializa o pino do LED
*/
void LED_init(int estado) {
	pmc_enable_periph_clk(OLED1_PIO_ID);
	pio_set_output(OLED1_PIO, OLED1_PIO_IDX_MASK, estado, 0, 0);
	
	pmc_enable_periph_clk(OLED2_PIO_ID);
	pio_set_output(OLED2_PIO, OLED2_PIO_IDX_MASK, estado, 0, 0);
	
	pmc_enable_periph_clk(OLED3_PIO_ID);
	pio_set_output(OLED3_PIO, OLED3_PIO_IDX_MASK, estado, 0, 0);
	
	pmc_enable_periph_clk(LED_PIO_ID);
	pio_set_output(LED_PIO, LED_IDX_MASK, estado, 0, 0);
	
}

/**
* @Brief Inverte o valor do pino 0->1/ 1->0
*/
void pin_toggle(Pio *pio, uint32_t mask) {
	if(pio_get_output_data_status(pio, mask))
	pio_clear(pio, mask);
	else
	pio_set(pio,mask);
}

/**
* Configura TimerCounter (TC) para gerar uma interrupcao no canal (ID_TC e TC_CHANNEL)
* na taxa de especificada em freq.
* O TimerCounter é meio confuso
* o uC possui 3 TCs, cada TC possui 3 canais
*	TC0 : ID_TC0, ID_TC1, ID_TC2
*	TC1 : ID_TC3, ID_TC4, ID_TC5
*	TC2 : ID_TC6, ID_TC7, ID_TC8
*
**/
void TC_init(Tc * TC, int ID_TC, int TC_CHANNEL, int freq){
	uint32_t ul_div;
	uint32_t ul_tcclks;
	uint32_t ul_sysclk = sysclk_get_cpu_hz();

	/* Configura o PMC */
	pmc_enable_periph_clk(ID_TC);

	/** Configura o TC para operar em  freq hz e interrupçcão no RC compare */
	tc_find_mck_divisor(freq, ul_sysclk, &ul_div, &ul_tcclks, ul_sysclk);
	tc_init(TC, TC_CHANNEL, ul_tcclks | TC_CMR_CPCTRG);
	tc_write_rc(TC, TC_CHANNEL, (ul_sysclk / ul_div) / freq);

	/* Configura NVIC*/
	NVIC_SetPriority(ID_TC, 4);
	NVIC_EnableIRQ((IRQn_Type) ID_TC);
	tc_enable_interrupt(TC, TC_CHANNEL, TC_IER_CPCS);
}

void RTC_init(Rtc *rtc, uint32_t id_rtc, calendar t, uint32_t irq_type) {
	/* Configura o PMC */
	pmc_enable_periph_clk(ID_RTC);

	/* Default RTC configuration, 24-hour mode */
	rtc_set_hour_mode(rtc, 0);

	/* Configura data e hora manualmente */
	rtc_set_date(rtc, t.year, t.month, t.day, t.week);
	rtc_set_time(rtc, t.hour, t.minute, t.second);

	/* Configure RTC interrupts */
	NVIC_DisableIRQ(id_rtc);
	NVIC_ClearPendingIRQ(id_rtc);
	NVIC_SetPriority(id_rtc, 4);
	NVIC_EnableIRQ(id_rtc);

	/* Ativa interrupcao via alarme */
	rtc_enable_interrupt(rtc,  irq_type);
}

void cronometro(){
	segundo++;
	
	if(segundo == 60){
		minuto++;
		segundo = 0;
		gfx_mono_draw_string(" ", 90,0, &sysfont);
	}
	
	
	if(minuto == 60){
		hora++;
		minuto = 0;
		gfx_mono_draw_string(" ", 60,0, &sysfont);
	}
	
	if(hora == 24){
		hora = 0;
		gfx_mono_draw_string(" ", 30,0, &sysfont);
	}
	char hora_str[5];
	sprintf(hora_str, "%02d", hora);
	
	char minuto_str[5];
	sprintf(minuto_str, "%02d", minuto);
	
	char segundo_str[5];
	sprintf(segundo_str, "%02d", segundo);

	
	gfx_mono_draw_string(hora_str, 20,0, &sysfont);
	gfx_mono_draw_string(":", 40,0, &sysfont);
	gfx_mono_draw_string(minuto_str, 50,0, &sysfont);
	gfx_mono_draw_string(":", 70,0, &sysfont);
	gfx_mono_draw_string(segundo_str, 80,0, &sysfont);
}

int main (void)
{
	board_init();
	sysclk_init();
	io_init();
	delay_init();

  // Init OLED
	gfx_mono_ssd1306_init();
  
  
	//gfx_mono_draw_filled_circle(20, 16, 16, GFX_PIXEL_SET, GFX_WHOLE);
  //gfx_mono_draw_string("mundo", 50,16, &sysfont);

	/* Disable the watchdog */
	WDT->WDT_MR = WDT_MR_WDDIS;

	/* Configura Leds */
	LED_init(1);
	

	/**
	 * Configura timer TC0, canal 1 
	 * e inicializa contagem 
	 */
	
	 /** Configura RTC */
	calendar rtc_initial = {2018, 3, 19, 12, 15, 45 ,1};
	RTC_init(RTC, ID_RTC, rtc_initial, RTC_IER_ALREN);
	


	TC_init(TC0, ID_TC1, 1, 4);
	tc_start(TC0, 1);
	
	TC_init(TC0, ID_TC2, 2, 5);
	tc_start(TC0, 2);
	
	TC_init(TC0, ID_TC0, 0, 1);
	tc_start(TC0, 0);
	
	RTT_init(0.25, 1, RTT_MR_RTTINCIEN);   
  

  /* Insert application code here, after the board has been initialized. */
	while(1) {
		
		
		if(but_flag1_press){
			uint32_t current_hour, current_min, current_sec;
			uint32_t current_year, current_month, current_day, current_week;
			rtc_get_time(RTC, &current_hour, &current_min, &current_sec);
			rtc_get_date(RTC, &current_year, &current_month, &current_day, &current_week);
			rtc_set_date_alarm(RTC, 1, current_month, 1, current_day);
			
			/* configura alarme do RTC para daqui 20 segundos */
			if(current_sec<40){
				rtc_set_time_alarm(RTC, 1, current_hour, 1, current_min, 1, current_sec + 20);
				
			}
			else{
				rtc_set_time_alarm(RTC, 1, current_hour, 1, current_min+1, 1, current_sec - 40);
			}
			
			
			
			
			but_flag1_press = 0;
			
			
		}
		if(flag_rtc_alarm){
			pisca_led(3, 200);
			flag_rtc_alarm = 0;
			
			
		}
		
		if(flag_cronometro){
			cronometro();
			flag_cronometro = 0;
		}
		
			
			
		
		
				



			// Escreve na tela um circulo e um texto
			
		
		
		pmc_sleep(SAM_PM_SMODE_SLEEP_WFI);
			
			
	}
}
