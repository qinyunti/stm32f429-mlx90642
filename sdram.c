#include <stdint.h>
#include "sdram.h"
#include "xprintf.h"

#define FMC_BASE	0xA0000000
#define FMC_SDSR_BUSY	(1 << 5)

#define SYSCFG_BASE	0x40013800
#define SYSCFG_MEMRMP_SWP_FMC	0x1

#define SDRAM_BANK2 0x02
#define SDRAM_BANK1 0x01

#define SDRAM_MODE_NORMAL      0x00
#define SDRAM_MODE_CLKCFGEN    0x01
#define SDRAM_MODE_PALL        0x02
#define SDRAM_MODE_AUTOREFRESH 0x03
#define SDRAM_MODE_LMR         0x04
#define SDRAM_MODE_SELFREFRESH 0x05
#define SDRAM_MODE_POWERDOWN   0x06

#define SDRAM_BANK_2 0
#define SDRAM_BANK_4 1
#define SDRAM_BUS_WIDTH_8 0
#define SDRAM_BUS_WIDTH_16 1
#define SDRAM_BUS_WIDTH_32 2
#define SDRAM_ROW_BITS_11 0
#define SDRAM_ROW_BITS_12 1
#define SDRAM_ROW_BITS_13 2
#define SDRAM_COLUMN_BITS_8 0
#define SDRAM_COLUMN_BITS_9 1
#define SDRAM_COLUMN_BITS_10 2
#define SDRAM_COLUMN_BITS_11 3

typedef struct sdram_cr{
    uint8_t rpipe;  /**< FMC_SDCR2的只读, CAS延迟之后，再延迟多少个HCLK去读数据, 可配0,1,2 */
    uint8_t rburst; /**< FMC_SDRCR2的无用, 单读请求是否按照Burst管理    */
    uint8_t sdclk;  /**< 配置SDCLK时钟分频 00:关闭SDCLK 10:HCLK/2 11:HCLK/3 */
    uint8_t wp;     /**< 1写保护 0可以正常写 */
    uint8_t cas;    /**< CAS延迟,可配置1，2，3 */
    uint8_t nb;     /**< 配置SDRAM内部BANK数 0:2个BANK 1:4个BANK */
    uint8_t mwid;   /**< 数据总线宽度 0:8位,1:16位,2:32位 */
    uint8_t nr;     /**< 行地址位数,0:11位,1:12位,2:13位 */
    uint8_t nc;     /**< 列地址位数,0:8位,1:9位,2:10位,3:11位 */
} sdram_cr_st;

typedef struct sdram_tr{
    /* 接了两个BANK的SDRAM, TMRD,TRAS,TXSR以FMC_SDTR1为准
     * TRP，TRC只在FMC_SDTR1配置，以低速为准
    */
    uint8_t trcd;   /**< Activate和Read/Write命令之间延迟的SDCLk时钟数,寄存器值是时钟数-1，时钟数1~16 */
    uint8_t trp;    /**< Precharge和其他命令之间延迟的SDCLK时钟数,寄存器值是时钟数-1，时钟数1~16, FMC_SDTR2无用    */
    uint8_t twr;    /**< Write和Precharge命令之间延迟的SDCLK时钟数,寄存器值是时钟数-1，时钟数1~16 TWR≥TRAS-TRCD，TWR≥TRC-TRCD-TRP 两个BANK要设置一样以低速为准 */
    uint8_t trc;    /**< Refresh和Activate命令之间.或者两个Refresh命令之间延迟的SDCLK时钟数,寄存器值是时钟数-1，时钟数1~16 仅FMC_SDTR1配置,两个BANK一致以低速为准 */
    uint8_t tras;   /**< 最小self-refresh周期,SDCLK时钟数, 寄存器值是时钟数-1，时钟数1~16 */
    uint8_t txsr;   /**< Self-refresh到Activate命令之间的延迟SDCLK时钟数. 寄存器值是时钟数-1，时钟数1~16 */
    uint8_t tmrd;   /**< 加载模式寄存器和Active/Refresh命令之间延迟的SDCLK时钟数, 寄存器值是时钟数-1，时钟数1~16 */
} sdram_tr_st;

volatile uint32_t *FMC_SDSR = (void *)(FMC_BASE + 0x158);
volatile uint32_t *FMC_SDCR1 = (void *)(FMC_BASE + 0x140);
volatile uint32_t *FMC_SDCR2 = (void *)(FMC_BASE + 0x144);
volatile uint32_t *FMC_SDTR1 = (void *)(FMC_BASE + 0x148);
volatile uint32_t *FMC_SDTR2 = (void *)(FMC_BASE + 0x14C);
volatile uint32_t *FMC_SDCMR = (void *)(FMC_BASE + 0x150);

/* 手册P1682  */
static void sdram_wait_busy(void)
{
	while ((*FMC_SDSR & FMC_SDSR_BUSY)) {
	}
}

extern uint32_t get_ticks(void);

static void sdram_delay_ms(int ms){
	uint32_t pre = 0;
	uint32_t cur = 0;
	pre = get_ticks();
	while(1){
		cur = get_ticks();
		if(cur - pre >= ms){
			return;
		}
	}
}

void sdram_get_cr(int bank, sdram_cr_st* cr){
	uint32_t tmp;
	if((bank & 0x01)){
		tmp = *FMC_SDCR1;
	}else if(bank & 0x02){
		tmp = *FMC_SDCR2;
	}else{
		return;
	}
	cr->nc = tmp & 0x03;
	cr->nr = (tmp >> 2) & 0x03;
	cr->mwid = (tmp >> 4) & 0x03;
	cr->nb = (tmp >> 6) & 0x01;
	cr->cas = (tmp >> 7) & 0x03;
	cr->wp = (tmp >>9) & 0x01;
	cr->sdclk = (tmp >>10) & 0x03;
	cr->rburst = (tmp >> 12) & 0x01;
	cr->rpipe = (tmp >> 13) & 0x03;
}

void sdram_set_cr(int bank, sdram_cr_st* cr){
	uint32_t tmp = 0;

	tmp |= (uint32_t)cr->nc & 0x03;
    tmp |= ( (uint32_t)cr->nr & 0x03) << 2;
    tmp |= ( (uint32_t)cr->mwid & 0x03) << 4;
    tmp |= ( (uint32_t)cr->nb & 0x01) << 6;
    tmp |= ( (uint32_t)cr->cas & 0x03) << 7;
    tmp |= ( (uint32_t)cr->wp & 0x01) << 9;
    tmp |= ( (uint32_t)cr->sdclk & 0x03) << 10;
    tmp |= ( (uint32_t)cr->rburst & 0x01) << 12;
    tmp |= ( (uint32_t)cr->rpipe & 0x03) << 13;

	if(bank & 0x01){
		xprintf("set SDCR1 to %x\r\n",tmp);
		*FMC_SDCR1 = tmp;
		xprintf("get SDCR1 %x\r\n",*FMC_SDCR1);
	}else if(bank & 0x02){
		xprintf("set SDCR2 to %x\r\n",tmp);
		*FMC_SDCR2 = tmp;
		xprintf("get SDCR2 %x\r\n",*FMC_SDCR2);
	}else{
		return;
	}
}

void sdram_get_tr(int bank, sdram_tr_st* tr){
	uint32_t tmp;
	if(bank & 0x01){
		tmp = *FMC_SDTR1;
	}else if(bank & 0x02){
		tmp = *FMC_SDTR2;
	}else{
		return;
	}
	tr->tmrd = tmp & 0x0F;
	tr->txsr = (tmp >> 4) & 0x0F;
	tr->tras = (tmp >> 8) & 0x0F;
	tr->trc = (tmp >> 12) & 0x0F;
	tr->twr = (tmp >> 16) & 0x0F;
	tr->trp = (tmp >> 20) & 0x0F;
	tr->trcd = (tmp >> 24) & 0x0F;
}

void sdram_set_tr(int bank, sdram_tr_st* tr){
	uint32_t tmp = 0;

	tmp |=  (uint32_t)tr->tmrd & 0x0F;
    tmp |= ( (uint32_t)tr->txsr & 0x0F) << 4;
    tmp |= ( (uint32_t)tr->tras & 0x0F) << 8;
    tmp |= ( (uint32_t)tr->trc & 0x0F) << 12;
    tmp |= ( (uint32_t)tr->twr & 0x0F) << 16;
    tmp |= ( (uint32_t)tr->trp & 0x0F) << 20;
    tmp |= ( (uint32_t)tr->trcd & 0x0F) << 24;

	if(bank & 0x01){
		xprintf("set SDTR1 to %x\r\n",tmp);
		*FMC_SDTR1 = tmp;
		xprintf("get SDTR1 %x\r\n",*FMC_SDTR1);
	}else if(bank & 0x02){
		xprintf("set SDTR2 to %x\r\n",tmp);
		*FMC_SDTR2 = tmp;
		xprintf("get SDTR2 %x\r\n",*FMC_SDTR2);
	}else{
		return;
	}
}

/**
 * mode:  000 Normal模式
 *        001 时钟配置使能
 *        010 PALL All Bank Precharge命令
 *        011 Auto-refresh命令
 *        100 加载模式寄存器
 *        101 Self_refresh命令
 *        110 Power-down命令
 *        111 保留
 * bank: bit0 BANK2  bit1 BANK1
 * nrfs:MODE=011即Auto-refresh命令时,指定连续自动刷新次数。寄存器值是次数-1，次数1~15
 * mrd:待写入模式寄存器的内容
 */
void sdram_set_mode(int bank, int mode, int nrfs, int mrd){
	uint32_t tmp = 0;
	tmp |= (uint32_t)mode & 0x07;
	tmp |= ((uint32_t)nrfs & 0x0F) << 5;
	tmp |= ((uint32_t)mrd & 0x1FFF) << 9;
	if(bank & 0x01){
		tmp |= (uint32_t)1<<4;
	}
	if(bank & 0x02){
		tmp |= (uint32_t)1<<3;
	}
	xprintf("set SDCMR to %x\r\n",tmp);
	*FMC_SDCMR = tmp;
	xprintf("get SDCMR %x\r\n",*FMC_SDCMR);
}

void sdram_swap(int en){
	volatile uint32_t *SYSCFG_MEMRMP = (void *)(SYSCFG_BASE + 0x00);
	uint32_t tmp;
    /* 手册P298
	 * SWP_FMC=01 则SDRAM BANK1/2的地址分别是0x80000000,0x90000000
	 * 否则地址分别是0xC0000000,0xD0000000.
	 */
	tmp = *SYSCFG_MEMRMP;
	tmp &= ~((uint32_t)0x03 << 10);
	if(en){
		tmp |= (uint32_t)SYSCFG_MEMRMP_SWP_FMC << 10;
	}
	xprintf("set MEMRMP to %x\r\n",tmp);
	*SYSCFG_MEMRMP = tmp;
	xprintf("get MEMRMP %x\r\n",*SYSCFG_MEMRMP);
}

/**
 * cre  1:清除re刷新错误状态
 * count 刷新定时器计数值 单位SDCLK。 最小值0x29.
 * COUNT = SDRAM刷新率 x SDRAM时钟频率 - 20
 * SDRAM刷新率 = SDRAM刷新周期 / 行数 
 * reie 1:使能刷新错误中断
 */
void sdram_set_rtr(int reie, int count, int cre){
	volatile uint32_t *FMC_SDRTR = (void *)(FMC_BASE + 0x154);
	uint32_t tmp = *FMC_SDRTR;
	tmp &= (uint32_t)0x7FFF;
	tmp |= (uint32_t)cre & 0x01;
	tmp |= ((uint32_t)count & 0x1FFF) << 1;
	tmp |= ((uint32_t)reie & 0x01) << 14;
	xprintf("set SDRTR to %x\r\n",tmp);
	*FMC_SDRTR = tmp;
	xprintf("get SDRTR %x\r\n",*FMC_SDRTR);
}

static sdram_cr_st cr1;
static sdram_cr_st cr2;
static sdram_tr_st tr1;
static sdram_tr_st tr2;

void sdram_init(void){
	sdram_get_cr(SDRAM_BANK1, &cr1);
	cr1.cas = 0;
	cr1.mwid= 0;
	cr1.nb = 0;
	cr1.nc = 0;
	cr1.nr = 0;
	cr1.rburst = 1;
	cr1.rpipe = 0;
	cr1.sdclk = 2; /* HCLK/2 */
	cr1.wp = 0;
	sdram_set_cr(SDRAM_BANK1, &cr1);

	sdram_get_cr(SDRAM_BANK2, &cr2);
	cr2.cas = 3;
	cr2.mwid= SDRAM_BUS_WIDTH_16;   
	cr2.nb = SDRAM_BANK_4;    
#if defined(USE_IS42S16320F)
	cr2.nc = SDRAM_COLUMN_BITS_10;   /* 4x(2^13)x(2^10)x2B = 64MB */
	cr2.nr = SDRAM_ROW_BITS_13;
#else  
	cr2.nc = SDRAM_COLUMN_BITS_8;  /* 4x(2^12)x(2^8)x2B = 8MB */
	cr2.nr = SDRAM_ROW_BITS_12;
#endif
	cr2.rburst = 1;
	cr2.rpipe = 0;  /* rpipe和sdclk 对于BANK2是只读的 由BANK1配置 */
	cr2.sdclk = 0;  
	cr2.wp = 0;
	sdram_set_cr(SDRAM_BANK2, &cr2);

	sdram_get_tr(SDRAM_BANK1, &tr1);
    tr1.trcd = 0;  
    tr1.trp = 1;   
    tr1.twr = 0;   
    tr1.trc = 5;    
    tr1.tras = 0;   
    tr1.txsr = 0;  
    tr1.tmrd = 0;
	sdram_set_tr(SDRAM_BANK1, &tr1);

	sdram_get_tr(SDRAM_BANK2, &tr2);
    tr2.trcd = 1;  
    tr2.trp = 0;   
    tr2.twr = 1;   
    tr2.trc = 0;    
    tr2.tras = 3;   
    tr2.txsr = 6;  
    tr2.tmrd = 1;
	sdram_set_tr(SDRAM_BANK2, &tr2);

	sdram_wait_busy();

	sdram_set_mode(SDRAM_BANK2, SDRAM_MODE_CLKCFGEN, 0, 0);
	sdram_delay_ms(10);
	sdram_wait_busy();

	/* PALL */
	sdram_set_mode(SDRAM_BANK2, SDRAM_MODE_PALL, 0, 0);
	sdram_wait_busy();

	/* Auto-Refresh */
	sdram_set_mode(SDRAM_BANK2, SDRAM_MODE_AUTOREFRESH, 8-1, 0);
	sdram_wait_busy();

	/* 设置模式寄存器 */
	sdram_set_mode(SDRAM_BANK2, SDRAM_MODE_LMR, 0, 0x230);

	/* 设置刷新率 */
#if defined(USE_IS42S16320F)
	sdram_set_rtr(0, 1386/2, 0);
#else
	sdram_set_rtr(0, 1386, 0);
#endif
	sdram_wait_busy();

	/* swap bank地址 */
	sdram_swap(1);
}
