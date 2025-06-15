#include "MLX90642.h"
#include "xprintf.h"

int mlx90642_test(int n)
{
    int status = 0; 
    static uint16_t mlxto[MLX90642_TOTAL_NUMBER_OF_PIXELS + 1];
    xprintf("num = %d\r\n",n);
    if(n<=0){
        n=1;
    }
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

    while(n--){
        xprintf("Start GetImage\r\n");
        /* Read out the image data */
        status = MLX90642_GetImage(SA_90642_DEFAULT, mlxto);
        if(status < 0){
            xprintf("MLX90642_GetImage err %d\r\n",status);
        }else{
            int idx=0;
            xprintf("MLX90642_GetImage ok\r\n");
            for(int i=0;i<32;i++){
                for(int j=0;j<24;j++){
                    xprintf("%-5d",mlxto[idx]);
                    idx++;
                }
                xprintf("\r\n");
            }
        }
        
        /* wait for new data */
        status = MLX90642_NO; 
        while(status == MLX90642_NO){
            status = MLX90642_IsReadWindowOpen(SA_90642_DEFAULT);
            if(status < 0){
                xprintf("MLX90642_IsReadWindowOpen err %d\r\n",status);
            }         
        }
    }
    return 0;
}