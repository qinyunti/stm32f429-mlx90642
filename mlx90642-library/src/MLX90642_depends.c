#include "stm32f4_regs.h"
#include "gpio.h"
#include "clock.h"
#include "io_iic.h"
#include "MLX90642.h"

/* IIC IO操作的移植 */
static void io_iic_scl_write_port(uint8_t val)
{
	gpio_write((void*)GPIOA_BASE, 'A', 8, val);
}

static void io_iic_sda_write_port(uint8_t val)
{
	gpio_set((void*)GPIOA_BASE, 'C', 9, 0, GPIOx_MODER_MODERy_GPOUTPUT,GPIOx_OSPEEDR_OSPEEDRy_HIGH, GPIOx_PUPDR_PULLUP);
	gpio_write((void*)GPIOA_BASE, 'C', 9, val);
}

static void io_iic_sda_2read_port(void)
{
	gpio_set((void*)GPIOA_BASE, 'C', 9, 0, GPIOx_MODER_MODERy_INPUT,GPIOx_OSPEEDR_OSPEEDRy_HIGH, GPIOx_PUPDR_PULLUP);
	gpio_write((void*)GPIOA_BASE, 'C', 9, 1);
}

static uint8_t io_iic_sda_read_port(void)
{
    return gpio_read((void*)GPIOA_BASE, 'C', 9)&0x01;
}

static void io_iic_delay_us_port(uint32_t delay)
{
	uint32_t volatile t=delay;
	while(t--);
}

static void io_iic_init_port(void)
{
    /**
     * PA8 I2C3_SCL 
     * PC9 I2C3_SDA 
     */
	volatile uint32_t *RCC_AHB1ENR = (void *)(RCC_BASE + 0x30);
	*RCC_AHB1ENR |= (1u<<0); /* GPIOA */
	*RCC_AHB1ENR |= (1u<<2); /* GPIOC */

	gpio_set((void*)GPIOA_BASE, 'A', 8, 0, GPIOx_MODER_MODERy_GPOUTPUT,GPIOx_OSPEEDR_OSPEEDRy_HIGH, GPIOx_PUPDR_PULLUP);
	gpio_write((void*)GPIOA_BASE, 'A', 8, 1);
	gpio_set((void*)GPIOA_BASE, 'C', 9, 0, GPIOx_MODER_MODERy_GPOUTPUT,GPIOx_OSPEEDR_OSPEEDRy_HIGH, GPIOx_PUPDR_PULLUP);
	gpio_write((void*)GPIOA_BASE, 'C', 9, 1);
}

static void io_iic_deinit_port(void)
{

}

static io_iic_dev_st iic_dev=
{
	.scl_write = io_iic_scl_write_port,
	.sda_write = io_iic_sda_write_port,
	.sda_2read = io_iic_sda_2read_port,
	.sda_read = io_iic_sda_read_port,
	.delay_pf = io_iic_delay_us_port,
	.init = io_iic_init_port,
	.deinit = io_iic_deinit_port,
	.delayus = 1000000ul,
};

static void MLX90642_I2CInit(void){
    static int s_mlx90642_init_flag = 0;
    if(s_mlx90642_init_flag == 0){
        s_mlx90642_init_flag = 1;
        io_iic_init(&iic_dev);
    }
}

int MLX90642_I2CRead(uint8_t slaveAddr, uint16_t startAddress, uint16_t nMemAddressRead, uint16_t *rData){
    int res;
    uint16_t tmp = 0;
    uint8_t byte_tmp;
    MLX90642_I2CInit();

    io_iic_start(&iic_dev);
    res = io_iic_write(&iic_dev, (slaveAddr<<1));
    if(res != 0){
        return res;
    }
    res = io_iic_write(&iic_dev, startAddress>>8); /* 高字节在前 */
    if(res != 0){
        return res;
    }
    res = io_iic_write(&iic_dev, startAddress&0xFF);
    if(res != 0){
        return res;
    }
    io_iic_start(&iic_dev);
    res = io_iic_write(&iic_dev, (slaveAddr<<1)|0x1); 
    if(res != 0){
        return res;
    }
    for(uint16_t i=0;i<nMemAddressRead;i++){
        res = io_iic_read(&iic_dev,&byte_tmp,0);
        if(res != 0){
            return res;
        }
        tmp = (uint16_t)byte_tmp<<8;
        res = io_iic_read(&iic_dev,&byte_tmp,0);
        if(res != 0){
            return res;
        }
        tmp |= (uint16_t)byte_tmp;
        rData[i] = tmp;
    }
    io_iic_stop(&iic_dev);
    return 0;
}

int MLX90642_Config(uint8_t slaveAddr, uint16_t writeAddress, uint16_t wData){
    int res;
    MLX90642_I2CInit();

    io_iic_start(&iic_dev);
    res = io_iic_write(&iic_dev, (slaveAddr<<1));
    if(res != 0){
        return res;
    }
    res = io_iic_write(&iic_dev, writeAddress>>8); /* 高字节在前 */
    if(res != 0){
        return res;
    }
    res = io_iic_write(&iic_dev, writeAddress&0xFF);
    if(res != 0){
        return res;
    }
    res = io_iic_write(&iic_dev, wData>>8); /* 高字节在前 */
    if(res != 0){
        return res;
    }
    res = io_iic_write(&iic_dev, wData&0xFF);
    if(res != 0){
        return res;
    }
    io_iic_stop(&iic_dev);
    return 0;
}

int MLX90642_I2CCmd(uint8_t slaveAddr, uint16_t i2c_cmd){
    int res;
    MLX90642_I2CInit();

    switch(i2c_cmd){
        case MLX90642_START_SYNC_MEAS_CMD:
        case MLX90642_SLEEP_CMD:
            res = MLX90642_Config(slaveAddr, MLX90642_CMD_OPCODE, i2c_cmd);
        break;
        default:
            res = -1;
        break;
    }
    return res;
}

int MLX90642_WakeUp(uint8_t slaveAddr){
    int res;
    MLX90642_I2CInit();

    io_iic_start(&iic_dev);
    res = io_iic_write(&iic_dev, (slaveAddr<<1));
    if(res != 0){
        return res;
    }
    res = io_iic_write(&iic_dev, 0x57);
    if(res != 0){
        return res;
    }
    io_iic_stop(&iic_dev);
    return 0;
}

void MLX90642_Wait_ms(uint16_t time_ms){
    clock_delay(time_ms);
}

void MLX90642_Set_Delay(uint32_t delay){
    iic_dev.delayus = delay;
}

