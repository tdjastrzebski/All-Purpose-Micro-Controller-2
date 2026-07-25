#include "lvgl_sub.h"

#include <stdarg.h>
#include <stdio.h>
#include <time.h>

#include "lvgl.h"
// #include "lvgl/demos/lv_demos.h"
#include "main.h"
#include "my_printf.h"
#include "st7789.h"
#include "terminal_colors.h"

#define LV_BPP sizeof(lv_color_t)  // bytes per LV pixel
#define LCD_BPP 2                  // bytes per LCD pixel
#define CACHE_ROW_SIZE 32U
#define DRAW_BUFFER_SIZE_PX ((SCREEN_WIDTH * SCREEN_HEIGHT) / 10)
#define AL(x, n) (x % n == 0 ? x : x + n - (x % n))  // align length
#define DRAW_BUFFER_SIZE_ALIGNED_PX AL(DRAW_BUFFER_SIZE_PX, CACHE_ROW_SIZE / LV_BPP)

LV_FONT_DECLARE(lv_font_montserrat_24)
LV_FONT_DECLARE(lv_font_montserrat_32)
LV_FONT_DECLARE(lv_font_montserrat_48)
LV_FONT_DECLARE(liberation_mono_128)
LV_FONT_DECLARE(Montserrat_Medium_32)
LV_FONT_DECLARE(DejaVuSans_32)

static lv_display_t* _display;                                                // lvgl display driver
ALIGN_32BYTES(static lv_color_t _lvDrawBuffer[DRAW_BUFFER_SIZE_ALIGNED_PX]);  // declare a buffer of 1/10 screen size
static char _displayTextBuffer[32];
static lv_obj_t* _largeLabel;
static lv_obj_t* _smallLabel;
static lv_obj_t* _screen = lv_scr_act();

extern TIM_HandleTypeDef LvglTickTimer;
extern TIM_HandleTypeDef LvglTaskTimer;
extern SPI_HandleTypeDef LcdSpi;

static spi_channel_dev_ctx _lcd_spi = {.channel = &LcdSpi, .cs_port = LCD_CS_GPIO_Port, .cs_pin = LCD_CS_Pin};

static void _flushBufferStart(lv_display_t* drv, const lv_area_t* area, uint8_t* px_map);
// static void _flushBufferComplete(DMA2D_HandleTypeDef* hdma2d);
static void _flushBufferWait(lv_display_t* drv);
static void _lvglTick(TIM_HandleTypeDef* htim);
static void _lvglTask(TIM_HandleTypeDef* htim);
static void _spiTxClpt(SPI_HandleTypeDef* hspi);
static void _helloWorld(void);

void lvgl_init(void) {
	lv_init();
	_display = lv_display_create(SCREEN_WIDTH, SCREEN_HEIGHT);
	// initialize display
	// TODO: test using 2nd buffer
	lv_display_set_buffers(_display, _lvDrawBuffer, nullptr, DRAW_BUFFER_SIZE_PX * LV_BPP, LV_DISPLAY_RENDER_MODE_PARTIAL);
	lv_display_set_flush_cb(_display, _flushBufferStart);
	// lv_display_set_flush_wait_cb(_display, _flushBufferWait);
	lv_display_set_resolution(_display, SCREEN_WIDTH, SCREEN_HEIGHT);
	lv_display_set_physical_resolution(_display, SCREEN_WIDTH, SCREEN_HEIGHT);

	LvglTickTimer.PeriodElapsedCallback = _lvglTick;
	LvglTaskTimer.PeriodElapsedCallback = _lvglTask;
	LcdSpi.TxCpltCallback = _spiTxClpt;

	// start LVGL timer 10ms
	HAL_TIM_Base_Start_IT(&LvglTickTimer);  // Note: this interrupt must have "Preemption Priority" higher than DMA interrupt. Lower "Preemption Priority" (DMA) is served FIRST and uninterrupted.
	HAL_TIM_Base_Start_IT(&LvglTaskTimer);  // Note: this interrupt must have "Preemption Priority" higher than LvglTickTimer

	_screen = lv_scr_act();
	_largeLabel = lv_label_create(_screen);
	_smallLabel = lv_label_create(_screen);

	_helloWorld();
	HAL_Delay(3000);

	lv_label_set_text_static(_largeLabel, "0");
	static lv_style_t largeFontStyle;
	lv_style_init(&largeFontStyle);
	lv_style_set_text_font(&largeFontStyle, &liberation_mono_128);
	lv_style_set_text_color(&largeFontStyle, lv_color_black());
	lv_obj_add_style(_largeLabel, &largeFontStyle, 0);
	lv_obj_align(_largeLabel, LV_ALIGN_CENTER, 0, 0);;

	lv_label_set_text_static(_smallLabel, "00:00:00");
	static lv_style_t smallFontStyle;
	lv_style_init(&smallFontStyle);
	lv_style_set_text_font(&smallFontStyle, &DejaVuSans_32);
	lv_style_set_text_color(&smallFontStyle, lv_color_black());
	lv_obj_add_style(_smallLabel, &smallFontStyle, 0);
	lv_obj_align(_smallLabel, LV_ALIGN_BOTTOM_MID, 0, 0);
}

static void _flushBufferStart(lv_display_t* drv, const lv_area_t* area, uint8_t* px_map) {
	lv_coord_t width = lv_area_get_width(area);
	lv_coord_t height = lv_area_get_height(area);
	st7789_SetWindow(&_lcd_spi, area->x1, area->y1, area->x2, area->y2);

	HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_RESET);

	// for unknow reason below does not work, not even simple bulk transfer without IRQ :(
	// HAL_SPI_Transmit_DMA(_lcd_spi.channel, px_map, width * height * LCD_BPP);
	// HAL_SPI_Transmit_IT(_lcd_spi.channel, px_map, width * height * LCD_BPP);
	// HAL_SPI_Transmit(_lcd_spi.channel, px_map, width * height * LCD_BPP, HAL_MAX_DELAY);

	// Why can I transfer only 2 lines at a time, even at 20Mbps ?
	while (height >= 2) {
		HAL_SPI_Transmit(_lcd_spi.channel, px_map, width * LCD_BPP * 2, HAL_MAX_DELAY);
		px_map += (width * LCD_BPP) * 2;
		height -= 2;
	}
	while (height > 0) {
		HAL_SPI_Transmit(_lcd_spi.channel, px_map, width * LCD_BPP, HAL_MAX_DELAY);
		px_map += (width * LCD_BPP);
		height--;
	}

	HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET);
	lv_display_flush_ready(_display);
}

static void _flushBufferWait(lv_display_t* drv) {
	// This wmethow will run within tick timer IRQ priority.
	// Probably I could lower tick timer IRQ priority and let other ISR run while waiting for DMA tansfer to finish.
}

static void _lvglTick(TIM_HandleTypeDef* htim) {
	lv_tick_inc(10);
}

static void _lvglTask(TIM_HandleTypeDef* htim) {
	lv_timer_handler();
}

static void _spiTxClpt(SPI_HandleTypeDef* hspi) {
	HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET);
	lv_display_flush_ready(_display);
}

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef* hspi) {
	if (hspi->Instance == SPI3) {
		HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET);
		lv_display_flush_ready(_display);
	}
}

static void _helloWorld() {
	// See: https://docs.lvgl.io/latest/en/html/widgets/label.html

	static lv_style_t largeFontStyle;
	lv_style_init(&largeFontStyle);
	lv_style_set_text_font(&largeFontStyle, &DejaVuSans_32);
	lv_style_set_text_color(&largeFontStyle, lv_color_black());
	lv_obj_add_style(_largeLabel, &largeFontStyle, 0);
	lv_label_set_text_static(_largeLabel, "Hello World!");
	lv_obj_align(_largeLabel, LV_ALIGN_CENTER, 0, 0);
}

void lvgl_showNumber(int8_t number) {
	sprintf(_displayTextBuffer, "%i", number);
	lv_label_set_text_static(_largeLabel, _displayTextBuffer);
	// lv_label_set_text_vfmt(_largeLabel, "%i", number);
	//lv_obj_align(_largeLabel, LV_ALIGN_CENTER, 0, 0);
}

void lvgl_showTime(tm* dt) {
	sprintf(_displayTextBuffer, "%.2i:%.2i:%.2i", dt->tm_hour, dt->tm_min, dt->tm_sec);
	lv_label_set_text_static(_smallLabel, _displayTextBuffer);
	//lv_obj_align(_smallLabel, LV_ALIGN_BOTTOM_MID, 0, 0);
}