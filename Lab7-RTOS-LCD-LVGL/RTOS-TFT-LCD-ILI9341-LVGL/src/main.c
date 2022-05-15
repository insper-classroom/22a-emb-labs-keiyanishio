/************************************************************************/
/* includes                                                             */
/************************************************************************/

#include <asf.h>
#include <string.h>
#include "ili9341.h"
#include "lvgl.h"
#include "touch/touch.h"
#include "pslogo.h"
#include "mercedes.h"
#include "relogio.h"


/************************************************************************/
/* LCD / LVGL                                                           */
/************************************************************************/
LV_FONT_DECLARE(dseg70);
LV_FONT_DECLARE(dseg25);
LV_FONT_DECLARE(dseg40);
LV_FONT_DECLARE(dseg15);




#define LV_HOR_RES_MAX          (320)
#define LV_VER_RES_MAX          (240)

/*A static or global variable to store the buffers*/
static lv_disp_draw_buf_t disp_buf;

/*Static or global buffer(s). The second buffer is optional*/
static lv_color_t buf_1[LV_HOR_RES_MAX * LV_VER_RES_MAX];
static lv_disp_drv_t disp_drv;          /*A variable to hold the drivers. Must be static or global.*/
static lv_indev_drv_t indev_drv;


// global
static lv_obj_t * labelBtn1;
static lv_obj_t * labelMenu;
static lv_obj_t * labelRelo;
static lv_obj_t * labelUp;
static lv_obj_t * labelDown;
static lv_obj_t * labelFloor;
static lv_obj_t * labelClock;
static lv_obj_t * labelSetValue;
static lv_obj_t * labelPonto4;
static lv_obj_t * labelData;
static lv_obj_t * labelCelsius;



///semaphore////
SemaphoreHandle_t xSemaphoreTime;

////RTC STRUCTURE///////
typedef struct  {
	uint32_t year;
	uint32_t month;
	uint32_t day;
	uint32_t week;
	uint32_t hour;
	uint32_t minute;
	uint32_t second;
} calendar;

/************************************************************************/
/* RTOS                                                                 */
/************************************************************************/

#define TASK_LCD_STACK_SIZE                (1024*6/sizeof(portSTACK_TYPE))
#define TASK_LCD_STACK_PRIORITY            (tskIDLE_PRIORITY)

#define TASK_RTC_STACK_SIZE                (1024*4/sizeof(portSTACK_TYPE))
#define TASK_RTC_STACK_PRIORITY            (tskIDLE_PRIORITY)

extern void vApplicationStackOverflowHook(xTaskHandle *pxTask,  signed char *pcTaskName);
extern void vApplicationIdleHook(void);
extern void vApplicationTickHook(void);
extern void vApplicationMallocFailedHook(void);
extern void xPortSysTickHandler(void);

extern void vApplicationStackOverflowHook(xTaskHandle *pxTask, signed char *pcTaskName) {
	printf("stack overflow %x %s\r\n", pxTask, (portCHAR *)pcTaskName);
	for (;;) {	}
}

extern void vApplicationIdleHook(void) { }

extern void vApplicationTickHook(void) { }

extern void vApplicationMallocFailedHook(void) {
	configASSERT( ( volatile void * ) NULL );
}



void RTC_Handler(void) {
	uint32_t ul_status = rtc_get_status(RTC);
	
	/* seccond tick */
	if ((ul_status & RTC_SR_SEC) == RTC_SR_SEC) {
		// o código para irq de segundo vem aqui
		BaseType_t xHigherPriorityTaskWoken = pdTRUE;
		xSemaphoreGiveFromISR(xSemaphoreTime, &xHigherPriorityTaskWoken);
	}
	
	/* Time or date alarm */
	if ((ul_status & RTC_SR_ALARM) == RTC_SR_ALARM) {
		// o código para irq de alame vem aqui
		
	}

	rtc_clear_status(RTC, RTC_SCCR_SECCLR);
	rtc_clear_status(RTC, RTC_SCCR_ALRCLR);
	rtc_clear_status(RTC, RTC_SCCR_ACKCLR);
	rtc_clear_status(RTC, RTC_SCCR_TIMCLR);
	rtc_clear_status(RTC, RTC_SCCR_CALCLR);
	rtc_clear_status(RTC, RTC_SCCR_TDERRCLR);
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



/************************************************************************/
/* lvgl                                                                 */
/************************************************************************/

static void event_handler(lv_event_t * e) {
	lv_event_code_t code = lv_event_get_code(e);

	if(code == LV_EVENT_CLICKED) {
		LV_LOG_USER("Clicked");
	}
	else if(code == LV_EVENT_VALUE_CHANGED) {
		LV_LOG_USER("Toggled");
	}
}

static void menu_handler(lv_event_t * e) {
	lv_event_code_t code = lv_event_get_code(e);

	if(code == LV_EVENT_CLICKED) {
		LV_LOG_USER("Clicked");
	}
	else if(code == LV_EVENT_VALUE_CHANGED) {
		LV_LOG_USER("Toggled");
	}
}

static void clock_handler(lv_event_t * e) {
	lv_event_code_t code = lv_event_get_code(e);

	if(code == LV_EVENT_CLICKED) {
		LV_LOG_USER("Clicked");
	}
	else if(code == LV_EVENT_VALUE_CHANGED) {
		LV_LOG_USER("Toggled");
	}
}

static void up_handler(lv_event_t * e) {
	lv_event_code_t code = lv_event_get_code(e);
	char *c;
	int temp;
	if(code == LV_EVENT_CLICKED) {
		c = lv_label_get_text(labelSetValue);
		temp = atoi(c);
		lv_label_set_text_fmt(labelSetValue, "%02d", temp + 1);
	}
}


static void down_handler(lv_event_t * e) {
	lv_event_code_t code = lv_event_get_code(e);
	char *c;
	int temp;
	if(code == LV_EVENT_CLICKED) {
		c = lv_label_get_text(labelSetValue);
		temp = atoi(c);
		lv_label_set_text_fmt(labelSetValue, "%02d", temp - 1);
	}
}



void lv_ex_btn_1(void) {
	lv_obj_t * label;

	lv_obj_t * btn1 = lv_btn_create(lv_scr_act());
	lv_obj_add_event_cb(btn1, event_handler, LV_EVENT_ALL, NULL);
	lv_obj_align(btn1, LV_ALIGN_CENTER, 0, -40);

	label = lv_label_create(btn1);
	lv_label_set_text(label, "Corsi");
	lv_obj_center(label);

	lv_obj_t * btn2 = lv_btn_create(lv_scr_act());
	lv_obj_add_event_cb(btn2, event_handler, LV_EVENT_ALL, NULL);
	lv_obj_align(btn2, LV_ALIGN_CENTER, 0, 40);
	lv_obj_add_flag(btn2, LV_OBJ_FLAG_CHECKABLE);
	lv_obj_set_height(btn2, LV_SIZE_CONTENT);

	label = lv_label_create(btn2);
	lv_label_set_text(label, "Toggle");
	lv_obj_center(label);
}



void lv_termostato(void) {
	     lv_obj_t * labelBtn1;
		 lv_obj_t * labelMenu;
		 lv_obj_t * labelRelo;
		 
		 
		 static lv_style_t style;
		 
		 /////////BUTTON///////
		 
		 /////////POWER BUTTON////////////
	     lv_style_init(&style);
		 lv_style_set_bg_color(&style, lv_color_black());
		 lv_style_set_border_width(&style, 5);

	     lv_obj_t * btn1 = lv_btn_create(lv_scr_act());
	     lv_obj_add_event_cb(btn1, event_handler, LV_EVENT_ALL, NULL);
	     lv_obj_align(btn1, LV_ALIGN_BOTTOM_LEFT, 0, 0);
		 lv_obj_add_style(btn1, &style, 0);

	     labelBtn1 = lv_label_create(btn1);
	     lv_label_set_text(labelBtn1, "[  " LV_SYMBOL_POWER);
	     lv_obj_center(labelBtn1);
		 
		 
		 
		 ////////////MENU BUTTON///////////////
		 lv_style_init(&style);
		 lv_style_set_bg_color(&style, lv_color_black());
		 lv_style_set_border_width(&style, 5);

	     lv_obj_t * btnmenu = lv_btn_create(lv_scr_act());
	     lv_obj_add_event_cb(btnmenu, menu_handler, LV_EVENT_ALL, NULL);
	     lv_obj_align_to(btnmenu, btn1,  LV_ALIGN_RIGHT_MID, 35, -13);
		 
		 lv_obj_add_style(btnmenu, &style, 0);

	     labelMenu = lv_label_create(btnmenu);
	     lv_label_set_text(labelMenu, "| M |");
	     lv_obj_center(labelMenu);
		 
		 
		 
		 /////////////SETTINGS BUTTON//////////////
		 lv_style_init(&style);
		 lv_style_set_bg_color(&style, lv_color_black());
		 lv_style_set_border_width(&style, 5);

		 lv_obj_t * btnclock = lv_btn_create(lv_scr_act());
		 lv_obj_add_event_cb(btnclock, clock_handler, LV_EVENT_ALL, NULL);
		 lv_obj_align_to(btnclock, btnmenu,  LV_ALIGN_RIGHT_MID, 35, -13);
		 
		 lv_obj_add_style(btnclock, &style, 0);

		 labelRelo = lv_label_create(btnclock);
		 lv_label_set_text(labelRelo, "    ]");
		 lv_obj_center(labelRelo);
		 
		 
		 
		 ///////////DOWN BUTTON///////////////////
		 lv_style_init(&style);
		 lv_style_set_bg_color(&style, lv_color_black());
		 lv_style_set_border_width(&style, 5);

		 lv_obj_t * btndown = lv_btn_create(lv_scr_act());
		 lv_obj_add_event_cb(btndown, down_handler, LV_EVENT_ALL, NULL);
		 lv_obj_align(btndown, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
		 
		 lv_obj_add_style(btndown, &style, 0);

		 labelDown = lv_label_create(btndown);
		 lv_label_set_text(labelDown, "| " LV_SYMBOL_DOWN " ]");
		 lv_obj_center(labelDown);
		 
		 
		 
		 //////////UP BUTTON////////////////////////
		 lv_style_init(&style);
		 lv_style_set_bg_color(&style, lv_color_black());
		 lv_style_set_border_width(&style, 5);

		 lv_obj_t * btnup = lv_btn_create(lv_scr_act());
		 lv_obj_add_event_cb(btnup, up_handler, LV_EVENT_ALL, NULL);
		 lv_obj_align_to(btnup, btndown, LV_ALIGN_LEFT_MID, -70, -13);
		 
		 lv_obj_add_style(btnup, &style, 0);

		 labelUp = lv_label_create(btnup);
		 lv_label_set_text(labelUp,  "[  " LV_SYMBOL_UP);
		 lv_obj_center(labelUp);
		 
		 
		 
		 
		 ///////////LABELS/////////////
		 
		//////////////FLOOR///////////////
		labelFloor = lv_label_create(lv_scr_act());
		lv_obj_align(labelFloor, LV_ALIGN_LEFT_MID, 35 , -45);
		lv_obj_set_style_text_font(labelFloor, &dseg70, LV_STATE_DEFAULT);
		lv_obj_set_style_text_color(labelFloor, lv_color_white(), LV_STATE_DEFAULT);
		lv_label_set_text_fmt(labelFloor, "%02d", 23);
		
		
		///////////////////PONTO 4///////////////
		
		labelPonto4 = lv_label_create(lv_scr_act());
		lv_obj_align(labelPonto4, LV_ALIGN_CENTER, 10 , -30);
		lv_obj_set_style_text_font(labelPonto4, &dseg40, LV_STATE_DEFAULT);
		lv_obj_set_style_text_color(labelPonto4, lv_color_white(), LV_STATE_DEFAULT);
		lv_label_set_text_fmt(labelPonto4, ".%d", 4);
	
		 
		///////////////CLOCK////////////////////////
		labelClock = lv_label_create(lv_scr_act());
		lv_obj_align(labelClock, LV_ALIGN_TOP_RIGHT, 0, 0);
		lv_obj_set_style_text_font(labelClock, &dseg25, LV_STATE_DEFAULT);
		lv_obj_set_style_text_color(labelClock, lv_color_white(), LV_STATE_DEFAULT);
		lv_label_set_text_fmt(labelClock, "%02d:%02d", 0, 0);
		
		
		/////////////////SET VALUE///////////////////
		labelSetValue = lv_label_create(lv_scr_act());
		lv_obj_align(labelSetValue,LV_ALIGN_RIGHT_MID, -5, -60);
		lv_obj_set_style_text_font(labelSetValue, &dseg40, LV_STATE_DEFAULT);
		lv_obj_set_style_text_color(labelSetValue, lv_color_white(), LV_STATE_DEFAULT);
		lv_label_set_text_fmt(labelSetValue, "%02d", 22);
		
		///////////////DATA//////////////////////////
		labelData = lv_label_create(lv_scr_act());
		lv_obj_align(labelData, LV_ALIGN_TOP_MID, 0, 0);
		lv_obj_set_style_text_font(labelData, &dseg15, LV_STATE_DEFAULT);
		lv_obj_set_style_text_color(labelData, lv_color_white(), LV_STATE_DEFAULT);
		lv_label_set_text_fmt(labelData, "%02d/%02d/%02d", 14, 05, 2022);
		
		
		///////////CELSIUS/////////////////////
		
		lv_style_init(&style);
		lv_style_set_bg_color(&style, lv_color_black());
		lv_style_set_border_width(&style, 5);

		lv_obj_t * celsius = lv_btn_create(lv_scr_act());
		lv_obj_add_event_cb(celsius, event_handler, LV_EVENT_ALL, NULL);
		lv_obj_align_to(celsius, labelSetValue, LV_ALIGN_LEFT_MID, -80, -30);
		lv_obj_add_style(celsius, &style, 0);

		labelCelsius = lv_label_create(celsius);
		lv_label_set_text(labelCelsius, "°C");
		
		
		
		
		
		
		
		
		
	
}

/************************************************************************/
/* TASKS                                                                */
/************************************************************************/

static void task_lcd(void *pvParameters) {
	int px, py;
	lv_obj_t * img = lv_img_create(lv_scr_act());
	lv_img_set_src(img, &pslogo);
	lv_obj_align(img, LV_ALIGN_TOP_LEFT, 0, 0);
	
	lv_obj_t * img2 = lv_img_create(lv_scr_act());
	lv_img_set_src(img2, &mercedes);
	lv_obj_align_to(img2, img,  LV_ALIGN_RIGHT_MID, 30, 0);
	
	
	
	lv_termostato();
	
	lv_obj_t * img3 = lv_img_create(lv_scr_act());
	lv_img_set_src(img3, &relogio);
	lv_obj_align(img3,  LV_ALIGN_BOTTOM_LEFT, 110, -5);
	
	


	

	for (;;)  {
		lv_tick_inc(50);
		lv_task_handler();
		vTaskDelay(50);
	}
}


static void task_time(void *pvParameters) {
	
	calendar rtc_initial = {2022, 5, 14, 12, 17, 1 ,0};
	RTC_init(RTC, ID_RTC, rtc_initial, RTC_IER_ALREN | RTC_IER_SECEN);
	uint32_t current_hour, current_min, current_sec;
	int cont = 0;
	
	

	for (;;)  {
		
		if (xSemaphoreTake(xSemaphoreTime, 1000)) {
			rtc_get_time(RTC, &current_hour, &current_min, &current_sec);
			
			if (cont == 0){
				lv_label_set_text_fmt(labelClock, "%02d:%02d", current_hour, current_min);
				cont = 1;
			}
			else if (cont == 1){
				lv_label_set_text_fmt(labelClock, "%02d %02d", current_hour, current_min);
				cont = 0;
			}
		}
		
		vTaskDelay(50);
	}
}
/************************************************************************/
/* configs                                                              */
/************************************************************************/

static void configure_lcd(void) {
	/**LCD pin configure on SPI*/
	pio_configure_pin(LCD_SPI_MISO_PIO, LCD_SPI_MISO_FLAGS);  //
	pio_configure_pin(LCD_SPI_MOSI_PIO, LCD_SPI_MOSI_FLAGS);
	pio_configure_pin(LCD_SPI_SPCK_PIO, LCD_SPI_SPCK_FLAGS);
	pio_configure_pin(LCD_SPI_NPCS_PIO, LCD_SPI_NPCS_FLAGS);
	pio_configure_pin(LCD_SPI_RESET_PIO, LCD_SPI_RESET_FLAGS);
	pio_configure_pin(LCD_SPI_CDS_PIO, LCD_SPI_CDS_FLAGS);
	
	ili9341_init();
	ili9341_backlight_on();
}

static void configure_console(void) {
	const usart_serial_options_t uart_serial_options = {
		.baudrate = USART_SERIAL_EXAMPLE_BAUDRATE,
		.charlength = USART_SERIAL_CHAR_LENGTH,
		.paritytype = USART_SERIAL_PARITY,
		.stopbits = USART_SERIAL_STOP_BIT,
	};

	/* Configure console UART. */
	stdio_serial_init(CONSOLE_UART, &uart_serial_options);

	/* Specify that stdout should not be buffered. */
	setbuf(stdout, NULL);
}

/************************************************************************/
/* port lvgl                                                            */
/************************************************************************/

void my_flush_cb(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p) {
	ili9341_set_top_left_limit(area->x1, area->y1);   ili9341_set_bottom_right_limit(area->x2, area->y2);
	ili9341_copy_pixels_to_screen(color_p,  (area->x2 + 1 - area->x1) * (area->y2 + 1 - area->y1));
	
	/* IMPORTANT!!!
	* Inform the graphics library that you are ready with the flushing*/
	lv_disp_flush_ready(disp_drv);
}

void my_input_read(lv_indev_drv_t * drv, lv_indev_data_t*data) {
	int px, py, pressed;
	
	if (readPoint(&px, &py))
		data->state = LV_INDEV_STATE_PRESSED;
	else
		data->state = LV_INDEV_STATE_RELEASED; 
	
	data->point.x = px;
	data->point.y = py;
}

void configure_lvgl(void) {
	lv_init();
	lv_disp_draw_buf_init(&disp_buf, buf_1, NULL, LV_HOR_RES_MAX * LV_VER_RES_MAX);
	
	lv_disp_drv_init(&disp_drv);            /*Basic initialization*/
	disp_drv.draw_buf = &disp_buf;          /*Set an initialized buffer*/
	disp_drv.flush_cb = my_flush_cb;        /*Set a flush callback to draw to the display*/
	disp_drv.hor_res = LV_HOR_RES_MAX;      /*Set the horizontal resolution in pixels*/
	disp_drv.ver_res = LV_VER_RES_MAX;      /*Set the vertical resolution in pixels*/

	lv_disp_t * disp;
	disp = lv_disp_drv_register(&disp_drv); /*Register the driver and save the created display objects*/
	
	/* Init input on LVGL */
	lv_indev_drv_init(&indev_drv);
	indev_drv.type = LV_INDEV_TYPE_POINTER;
	indev_drv.read_cb = my_input_read;
	lv_indev_t * my_indev = lv_indev_drv_register(&indev_drv);
}

/************************************************************************/
/* main                                                                 */
/************************************************************************/
int main(void) {
	/* board and sys init */
	board_init();
	sysclk_init();
	configure_console();

	/* LCd, touch and lvgl init*/
	configure_lcd();
	configure_touch();
	configure_lvgl();
	
	xSemaphoreTime = xSemaphoreCreateBinary();
	 if (xSemaphoreTime == NULL){
		 printf("falha em criar o semaforo \n");

	 }
		
	/* Create task to control oled */
	if (xTaskCreate(task_lcd, "LCD", TASK_LCD_STACK_SIZE, NULL, TASK_LCD_STACK_PRIORITY, NULL) != pdPASS) {
		printf("Failed to create lcd task\r\n");
	}
	
	if (xTaskCreate(task_time, "Time", TASK_RTC_STACK_SIZE, NULL, TASK_RTC_STACK_PRIORITY, NULL) != pdPASS) {
		printf("Failed to create time task\r\n");
	}
	
	/* Start the scheduler. */
	vTaskStartScheduler();

	while(1){ }
}
