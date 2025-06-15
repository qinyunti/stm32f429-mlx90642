#include <stdint.h>
#include "MLX90642.h"
#include "xprintf.h"
#include "clock.h"
#include "lcd_itf.h"

static uint8_t s_gray[32*24];
static uint16_t s_temp[MLX90642_TOTAL_NUMBER_OF_PIXELS + 1];

#define mlx90642_abs(x)      ((x)>0?(x):-(x))

/* -40 0
 * x   y
 * 260 255
 * k = (255-0)/(260-(-40))=(y-0)/(x-(-40))
 * 255/300 = y/(x+40)
 * y = 255*(x+40)/300
 */
static void mlx90642_temp2gray(uint16_t* buffer, uint8_t* gray){
    int idx = 0;
    int temp;
    for(int i=0; i<32; i++){
        for(int j=0; j<24; j++){
            temp = ((int16_t)buffer[i]*2+50) / 100;   /* 除以50对应温度,四舍五入 */
            gray[idx] = (uint8_t)((int32_t)255*((int32_t)temp+40)/(int32_t)300);
            idx++;
        }
    }
}

static void mlx90642_gray2rgb(uint8_t gray, uint8_t* r, uint8_t* g, uint8_t* b){
    *r=mlx90642_abs((int)0-(int)gray);
    *g=mlx90642_abs((int)127-(int)gray);
    *b=mlx90642_abs((int)255-(int)gray);
}

/**
 * Byte0         Byte1
 * D7~D3  D2~0   D7~5  D4~D0
 * R     |    G       |  B
 */
#define RGB(r,g,b) (((uint16_t)r&0xF8) | ((uint16_t)g>>5) | ((((uint16_t)g&0xE0) | ((uint16_t)b&0x1F))<<8))

int mlx90642_disp(void){
    uint8_t r;
    uint8_t g;
    uint8_t b;
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

    mlx90642_temp2gray(s_temp, s_gray);
    for(int x=0; x<32; x++){
        for(int y=0; y<24; y++){
            mlx90642_gray2rgb(s_gray[idx], &r, &g, &b);
            lcd_itf_fill(x*10,10,y*10,10,RGB(r,g,b));
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
    MLX90642_SetOutputFormat(SA_90642_DEFAULT, MLX90642_TEMPERATURE_OUTPUT); 

    MLX90642_SetI2CLevel(SA_90642_DEFAULT, MLX90642_I2C_LEVEL_VDD);
    MLX90642_SetSDALimitState(SA_90642_DEFAULT, MLX90642_I2C_SDA_CUR_LIMIT_OFF);
    MLX90642_SetI2CMode(SA_90642_DEFAULT, MLX90642_I2C_MODE_FM_PLUS); 
    MLX90642_Set_Delay(20ul);

    return 0;
}