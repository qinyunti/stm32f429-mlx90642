#include <stdint.h>
#include <math.h>
#include "MLX90642.h"
#include "xprintf.h"
#include "clock.h"
#include "lcd_itf.h"

static uint16_t s_temp[MLX90642_TOTAL_NUMBER_OF_PIXELS + 1];
static uint8_t s_r[32*24];
static uint8_t s_g[32*24];
static uint8_t s_b[32*24];

void temp2rgb(int16_t* temp)
{
    int16_t maxtemp = 0;
    int16_t t;
    uint8_t gray;
    uint8_t gray_max=255;
    for(int i=0;i<32*24;i++){
        if(temp[i]>maxtemp){
            maxtemp = temp[i];
        }
    }
#if 0
    t = ((int16_t)maxtemp*2+50) / 100;   /* 除以50对应温度,四舍五入 */
    gray_max = (uint8_t)(((int32_t)255*((int32_t)t+40))/(int32_t)300);
#else
    gray_max = 255;
#endif
	for(int i=0;i<32*24;i++){
		/* 转温度为灰度 
        * -40 0
        * x   y
        * 260 255
        * k = (255-0)/(260-(-40))=(y-0)/(x-(-40))
        * 255/300 = y/(x+40)
        * y = 255*(x+40)/300
        */
        t = ((int16_t)temp[i]*2+50) / 100;   /* 除以50对应温度,四舍五入 */
        gray = (uint8_t)(((int32_t)255*((int32_t)t+40))/(int32_t)300);
		/* 计算 */
        if(gray<=(gray_max/8)){
            s_r[i]=0;
            s_g[i]=0;
            s_b[i]=(gray*255)/64;
        }
        else if(gray<=(gray_max*3/8)){
            s_r[i]=0;
            s_g[i]=((gray-64)*255)/64;
            s_b[i]=((127-gray)*255)/64;
        }
        else if(gray<=(gray_max*5)/8){
            s_r[i]=((gray-128)*255)/64;
            s_g[i]=255;
            s_b[i]=0;
        }
        else{
            s_r[i]=255;
            s_g[i]=((255-gray)*255)/64;
            s_b[i]=0;
        }
	}	
}

/**
 * Byte0         Byte1
 * D7~D3  D2~0   D7~5  D4~D0
 * R     |    G       |  B
 */
#define RGB(r,g,b) (((uint16_t)r&0xF8) | ((uint16_t)g>>5) | ((((uint16_t)g&0xE0) | ((uint16_t)b&0x1F))<<8))

int mlx90642_disp(void){
    int status;
    int  idx = 0;

    /* wait for new data */
    status = MLX90642_NO; 
    while(status == MLX90642_NO){
        status = MLX90642_IsReadWindowOpen(SA_90642_DEFAULT);
        if(status < 0){
            xprintf("MLX90642_IsReadWindowOpen err %d\r\n",status);
        }         
    }

    xprintf("Start GetImage\r\n");
    /* Read out the image data */
    status = MLX90642_GetImage(SA_90642_DEFAULT, s_temp);
    if(status < 0){
        xprintf("MLX90642_GetImage err %d\r\n",status);
        return -1;
    }else{
        xprintf("MLX90642_GetImage ok\r\n");
    }
    temp2rgb((int16_t*)s_temp);
    for(int y=0; y<24; y++){
        for(int x=0; x<32; x++){
            lcd_itf_fill(x*10,10,y*10,10,RGB(s_r[idx],s_g[idx],s_b[idx]));
            idx++;
        }
    }
    lcd_itf_sync();
    return 0;
}

int mlx90642_disp_init(void)
{
    int status = 0; 
    uint8_t version[3];
    MLX90642_Set_Delay(1000ul);  /* 1000~25K 10~1.4M */

    status = MLX90642_GetFWver(SA_90642_DEFAULT,version);
    if(status < 0){
        xprintf("GetFWver err %d\r\n",status);
    }else{
        xprintf("ver:%d.%d.%d\r\n",version[0],version[1],version[2]);
    }
    MLX90642_SetMeasMode(SA_90642_DEFAULT, MLX90642_STEP_MEAS_MODE); 
    if(status < 0){
        xprintf("SetMeasMode err %d\r\n",status);
    }
    status = MLX90642_Init(SA_90642_DEFAULT);
    if(status < 0){
        xprintf("MLX90642_Init err %d\r\n",status);
    }else{
        xprintf("MLX90642_Init ok\r\n");
    }
    MLX90642_SetRefreshRate(SA_90642_DEFAULT, MLX90642_REF_RATE_2HZ); 
    MLX90642_SetOutputFormat(SA_90642_DEFAULT, MLX90642_TEMPERATURE_OUTPUT); 

    MLX90642_SetI2CLevel(SA_90642_DEFAULT, MLX90642_I2C_LEVEL_VDD);
    MLX90642_SetSDALimitState(SA_90642_DEFAULT, MLX90642_I2C_SDA_CUR_LIMIT_OFF);
    MLX90642_SetI2CMode(SA_90642_DEFAULT, MLX90642_I2C_MODE_FM_PLUS); 
    MLX90642_Set_Delay(20ul);

    return 0;
}