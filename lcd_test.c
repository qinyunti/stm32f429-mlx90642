#include "clock.h"
#include "lcd_itf.h"
#include "xprintf.h"

static void rgb_test(void)
{
    for(int x=0;x<240;x++)
    {
        for(int y=0;y<320;y++)
        {
            lcd_itf_set_pixel(x, y, 0xF800);
        }
    }
    lcd_itf_sync();
	clock_delay(1000);

    for(int x=0;x<240;x++)
    {
        for(int y=0;y<320;y++)
        {
            lcd_itf_set_pixel(x, y, 0x07E0);
        }
    }
    lcd_itf_sync();
	clock_delay(1000);

    for(int x=0;x<240;x++)
    {
        for(int y=0;y<320;y++)
        {
            lcd_itf_set_pixel(x, y, 0x001F);
        }
    }
    lcd_itf_sync();
	clock_delay(1000);
}
/**
 * Byte0         Byte1
 * D7~D3  D2~0   D7~5  D4~D0
 * R     |    G       |  B
 */
#define RGB(r,g,b) (((uint16_t)r&0xF8) | ((uint16_t)g>>5) | ((((uint16_t)g&0xE0) | ((uint16_t)b&0x1F))<<8))
/*  bit0~3 G
 *  bit5~7 B
 *  bit10~12 R
 */
static void rgb_test_direct(void)
{
    for(int x=0;x<240;x++)
    {
        for(int y=0;y<320;y++)
        {
            lcd_itf_set_pixel_direct(x, y, RGB(0,0,0));
        }
    }
	clock_delay(1000);

    for(int x=0;x<240;x++)
    {
        for(int y=0;y<320;y++)
        {
            lcd_itf_set_pixel_direct(x, y, RGB(255,0,0));
        }
    }
	clock_delay(1000);

    for(int x=0;x<240;x++)
    {
        for(int y=0;y<320;y++)
        {
            lcd_itf_set_pixel_direct(x, y, RGB(0,255,0));
        }
    }
	clock_delay(1000);

    for(int x=0;x<240;x++)
    {
        for(int y=0;y<320;y++)
        {
            lcd_itf_set_pixel_direct(x, y, RGB(0,0,255));
        }
    }
	clock_delay(1000);

    for(int x=0;x<240;x++)
    {
        for(int y=0;y<320;y++)
        {
            lcd_itf_set_pixel_direct(x, y, RGB(255,255,255));
        }
    }
	clock_delay(1000);


}

int lcd_test(void)
{
    lcd_itf_init();
    //rgb_test();
	rgb_test_direct();
	return 0;
}