#include "clock.h"
#include "lcd_itf.h"
#include "xprintf.h"

/**
 * Byte0         Byte1
 * D7~D3  D2~0   D7~5  D4~D0
 * R     |    G       |  B
 */
#define RGB(r,g,b) (((uint16_t)r&0xF8) | ((uint16_t)g>>5) | ((((uint16_t)g&0xE0) | ((uint16_t)b&0x1F))<<8))

static void rgb_test(void)
{
    for(int x=0;x<LCD_HSIZE;x++)
    {
        for(int y=0;y<LCD_VSIZE;y++)
        {
            lcd_itf_set_pixel(x, y, RGB(255,0,0));
        }
    }
    lcd_itf_sync();
	clock_delay(1000);

    for(int x=0;x<LCD_HSIZE;x++)
    {
        for(int y=0;y<LCD_VSIZE;y++)
        {
            lcd_itf_set_pixel(x, y, RGB(0,255,0));
        }
    }
    lcd_itf_sync();
	clock_delay(1000);

    for(int x=0;x<LCD_HSIZE;x++)
    {
        for(int y=0;y<LCD_VSIZE;y++)
        {
            lcd_itf_set_pixel(x, y, RGB(0,0,255));
        }
    }
    lcd_itf_sync();
	clock_delay(1000);
}

static void rgb_test_direct(void)
{
    for(int x=0;x<LCD_HSIZE;x++)
    {
        for(int y=0;y<LCD_VSIZE;y++)
        {
            lcd_itf_set_pixel_direct(x, y, RGB(0,0,0));
        }
    }
	clock_delay(1000);

    for(int x=0;x<LCD_HSIZE;x++)
    {
        for(int y=0;y<LCD_VSIZE;y++)
        {
            lcd_itf_set_pixel_direct(x, y, RGB(255,0,0));
        }
    }
	clock_delay(1000);

    for(int x=0;x<LCD_HSIZE;x++)
    {
        for(int y=0;y<LCD_VSIZE;y++)
        {
            lcd_itf_set_pixel_direct(x, y, RGB(0,255,0));
        }
    }
	clock_delay(1000);

    for(int x=0;x<LCD_HSIZE;x++)
    {
        for(int y=0;y<LCD_VSIZE;y++)
        {
            lcd_itf_set_pixel_direct(x, y, RGB(0,0,255));
        }
    }
	clock_delay(1000);

    for(int x=0;x<LCD_HSIZE;x++)
    {
        for(int y=0;y<LCD_VSIZE;y++)
        {
            lcd_itf_set_pixel_direct(x, y, RGB(255,255,255));
        }
    }
	clock_delay(1000);


}

int lcd_test(void)
{
    lcd_itf_init();
    rgb_test();
	rgb_test_direct();
	return 0;
}