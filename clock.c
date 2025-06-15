#include "stm32f4_regs.h"
#include "clock.h"

#define CONFIG_HSE_HZ	8000000
#define CONFIG_PLL_M	8
#define CONFIG_PLL_N	360
#define CONFIG_PLL_P	2
#define CONFIG_PLL_Q	7
/* 
 * VCO input frequency = PLL input clock frequency / PLLM with 2 ≤PLLM ≤63
 * 这里是CONFIG_HSE_HZ / CONFIG_PLL_M ,PPLM=8在范围内
 * 
 * VCO output frequency = VCO input frequency × PLLN  
 * 必须在100~432MHz之间
 * 即(CONFIG_HSE_HZ / CONFIG_PLL_M) * CONFIG_PLL_N = 360MHz在范围内
 * 
 * PLL output clock frequency = VCO frequency / PLLP with PLLP = 2, 4, 6, or 8
 * 这里PLLP=2 所以360M/2=180M 在范围<=180MHz范围内
 * 
*/
#define PLLCLK_HZ (((CONFIG_HSE_HZ / CONFIG_PLL_M) * CONFIG_PLL_N) / CONFIG_PLL_P)
#if PLLCLK_HZ == 180000000
#define FLASH_LATENCY	5
#else
#error PLL clock does not match 180 MHz
#endif

static volatile uint32_t s_ms=0;

void tick(void)
{
	volatile uint32_t* ctrl = (volatile uint32_t*)SYSTICK_BASE;
	if(*ctrl & 0x10000){ /* 读清除标志bit16 */
		s_ms++;
	}
}

uint32_t get_ticks(void){
	return s_ms;
}

void systick_init(void){
	volatile uint32_t* ctrl = (volatile uint32_t*)SYSTICK_BASE;
	volatile uint32_t* load = (volatile uint32_t*)(SYSTICK_BASE+0x04);
	volatile uint32_t* val = (volatile uint32_t*)(SYSTICK_BASE+0x08);
	//volatile uint32_t* calib = (volatile uint32_t*)(SYSTICK_BASE+0x0C);
	*load = PLLCLK_HZ/1000-1;  /* 24位 最大 16777216 */
	*val = 0;
	*ctrl = (1u<<2) | (1u<<1) | (1u<<0);
}

void systick_deinit(void){
	volatile uint32_t* ctrl = (volatile uint32_t*)SYSTICK_BASE;
	*ctrl = 0;
}

uint32_t clock_get_apb1(void){
	volatile uint32_t *RCC_CFGR = (void *)(RCC_BASE + 0x08);
	uint32_t div = (*RCC_CFGR >> 10) & 0x07;
	if(div == 4){
		return PLLCLK_HZ/2;
	}else if(div == 5){
		return PLLCLK_HZ/4;	
	}if(div == 6){
		return PLLCLK_HZ/8;
	}else if(div == 7){
		return PLLCLK_HZ/16;	
	}else{
		return PLLCLK_HZ;
	}
}

uint32_t clock_get_apb2(void){
	volatile uint32_t *RCC_CFGR = (void *)(RCC_BASE + 0x08);
	uint32_t div = (*RCC_CFGR >> 13) & 0x07;
	if(div == 4){
		return PLLCLK_HZ/2;
	}else if(div == 5){
		return PLLCLK_HZ/4;	
	}if(div == 6){
		return PLLCLK_HZ/8;
	}else if(div == 7){
		return PLLCLK_HZ/16;	
	}else{
		return PLLCLK_HZ;
	}
}

uint32_t clock_get_ahb(void){
	volatile uint32_t *RCC_CFGR = (void *)(RCC_BASE + 0x08);
	uint32_t div = (*RCC_CFGR >> 4) & 0x0F;
	if(div == 8){
		return PLLCLK_HZ/2;
	}else if(div == 9){
		return PLLCLK_HZ/4;	
	}if(div == 10){
		return PLLCLK_HZ/8;
	}else if(div == 11){
		return PLLCLK_HZ/16;	
	}else if(div == 12){
		return PLLCLK_HZ/64;	
	}else if(div == 13){
		return PLLCLK_HZ/128;	
	}else if(div == 14){
		return PLLCLK_HZ/256;	
	}else if(div == 15){
		return PLLCLK_HZ/512;	
	}else{
		return PLLCLK_HZ;
	}
}

void clock_setup(void)
{
	volatile uint32_t *RCC_CR = (void *)(RCC_BASE + 0x00);
	volatile uint32_t *RCC_PLLCFGR = (void *)(RCC_BASE + 0x04);
	volatile uint32_t *RCC_CFGR = (void *)(RCC_BASE + 0x08);
	volatile uint32_t *FLASH_ACR = (void *)(FLASH_BASE + 0x00);
	volatile uint32_t *RCC_AHB1ENR = (void *)(RCC_BASE + 0x30);
	volatile uint32_t *RCC_AHB2ENR = (void *)(RCC_BASE + 0x34);
	volatile uint32_t *RCC_AHB3ENR = (void *)(RCC_BASE + 0x38);
	volatile uint32_t *RCC_APB1ENR = (void *)(RCC_BASE + 0x40);
	volatile uint32_t *RCC_APB2ENR = (void *)(RCC_BASE + 0x44);
	volatile uint32_t *RCC_AHB1LPENR= (void *)(RCC_BASE + 0x50);
	volatile uint32_t *PWR_CR= (void *)(0x40007000 + 0x00);
	volatile uint32_t *PWR_CSR= (void *)(0x40007000 + 0x04);

	uint32_t val;

	*RCC_CR |= RCC_CR_HSEON;
	while (!(*RCC_CR & RCC_CR_HSERDY)) {
	}

	val = *RCC_CFGR;
	/**
	 * AHB最大不分频180MHz
	 * 0xxx: system clock not divided
     * 1000: system clock divided by 2
	 * 1001: system clock divided by 4
	 * 1010: system clock divided by 8
	 * 1011: system clock divided by 16
	 * 1100: system clock divided by 64
	 * 1101: system clock divided by 128
	 * 1110: system clock divided by 256
	 * 1111: system clock divided by 512
	 */
	val &= ~RCC_CFGR_HPRE_MASK;
	//val |= 0 << 4; // not divided
	/** 
	 * APB1最大45MHz  
	 * 0xx: AHB clock not divided
	 * 100: AHB clock divided by 2
	 * 101: AHB clock divided by 4
	 * 110: AHB clock divided by 8
	 * 111: AHB clock divided by 16
	 */
	val &= ~RCC_CFGR_PPRE1_MASK;         
	val |= 0x5 << 10; // divided by 4
	/**
	 * APB2最大90MHz 
	 * 0xx: AHB clock not divided
	 * 100: AHB clock divided by 2
	 * 101: AHB clock divided by 4
	 * 110: AHB clock divided by 8
	 * 111: AHB clock divided by 16
	 */
	val &= ~RCC_CFGR_PPRE2_MASK;     
	val |= 0x4 << 13; // divided by 2
	*RCC_CFGR = val;

	/* __HAL_RCC_PWR_CLK_ENABLE 先使能PWR模块时钟,否则后续PWR操作都不会生效 */
	*RCC_APB1ENR |= (uint32_t)1u<<28;

	/* 
	 * VCAP1和VCAP2引脚要通过电容接GND
	 * VOS[1:0]选Scale 1/2 mode 默认是Scale 1 mode
	 * These bits can be modified only when the PLL is OFF.  
	 */
	*PWR_CR = (*PWR_CR & (~((uint32_t)3u<<14))) | ((uint32_t)3u<<14);

	val = 0;
	val |= RCC_PLLCFGR_PLLSRC_HSE;
	val |= CONFIG_PLL_M;
	val |= CONFIG_PLL_N << 6;
	val |= ((CONFIG_PLL_P >> 1) - 1) << 16;
	val |= CONFIG_PLL_Q << 24;
	*RCC_PLLCFGR = val;

	*RCC_CR |= RCC_CR_PLLON;
	while (!(*RCC_CR & RCC_CR_PLLRDY));
	/* 
	 * 跑180MHz,需要设置Over-drive 模式 见手册P124
	 * 当前使用HSI or HSE，默认就是HSI
	 * 此前已经配置PLLCFGR且PLLON
	 * VOS[1:0]选Scale 3 mode 默认是Scale 1 mode
	 * set PWR_CR.ODEN=1, wait PWR_CSR.ODRDY=1 
	 * set PWR_CR.ODSWEN=1, wait PWR_CSR.ODSWRDY=1
	 */
	*PWR_CR |= ((uint32_t)1u<<16);
	while((*PWR_CSR & ((uint32_t)1u<<16)) == 0);
	*PWR_CR |= ((uint32_t)1u<<17);
	while((*PWR_CSR & ((uint32_t)1u<<17)) == 0);

	/* 手册P81 电压2.7~3.6V最少设置LATENCY=5
	 */
	*FLASH_ACR = FLASH_ACR_ICEN | FLASH_ACR_PRFTEN | FLASH_LATENCY;

	*RCC_CFGR &= ~RCC_CFGR_SW_MASK;
	*RCC_CFGR |= RCC_CFGR_SW_PLL;
	while ((*RCC_CFGR & RCC_CFGR_SWS_MASK) != RCC_CFGR_SWS_PLL) {
	}

	/*  Enable all clocks, unused ones will be gated at end of kernel boot */
	*RCC_AHB1ENR |= 0x7ef417ff;
	*RCC_AHB2ENR |= 0xf1;
	*RCC_AHB3ENR |= 0x1;
	*RCC_APB1ENR |= 0xf6fec9ff;
	*RCC_APB2ENR |= 0x4777f33;

	/* Clear bit OTGHSULPILPEN in register AHB1LPENR when OTG HS in FS mode with internal PHY */
  	/* https://my.st.com/public/STe2ecommunities/mcu/Lists/cortex_mx_stm32/Flat.aspx?RootFolder=%2Fpublic%2FSTe2ecommunities%2Fmcu%2FLists%2Fcortex_mx_stm32%2FPower%20consumption%20without%20low%20power&FolderCTID=0x01200200770978C69A1141439FE559EB459D7580009C4E14902C3CDE46A77F0FFD06506F5B&currentviews=469 */
	*RCC_AHB1LPENR &= ~RCC_AHB1LPENR_OTGHSULPILPEN;
}

static uint32_t diff_u32(uint32_t pre, uint32_t now){
	if(now >= pre){
		return now-pre;
	}else{
		return 0xFFFFFFFF - pre + now + 1;
	}
}

void clock_delay(int t){
	uint32_t t0 = get_ticks();
	volatile uint32_t t1;
	while(1){
		t1 = get_ticks();
		if(diff_u32(t0,t1) >= t){
			return;
		}
	}
}