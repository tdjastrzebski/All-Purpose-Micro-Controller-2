#ifdef __cplusplus
extern "C" {
#endif

#include <time.h>

#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 280

void lvgl_init(void);
void lvgl_showNumber(int8_t number);
void lvgl_showTime(tm* dt);
// void lvgl_tick(uint8_t period);
// void lvgl_task(uint8_t period);

#ifdef __cplusplus
}
#endif