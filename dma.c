#include <stdint.h>
#include "dma.h"
void dma_dis(uint32_t base, int stream){
    volatile uint32_t* DMA_SxCR = (volatile uint32_t* )(base+0x10 + 0x18 * stream);
	*DMA_SxCR = 0;  /* EN=0 */
}

void dma_cfg(uint32_t base, int stream, dma_st* dma_cfg){
    volatile uint32_t* DMA_SxCR = (volatile uint32_t* )(base+0x10 + 0x18 * stream);
    volatile uint32_t* DMA_SxNDTR = (volatile uint32_t* )(base+0x14 + 0x18 * stream);
    volatile uint32_t* DMA_SxPAR = (volatile uint32_t* )(base+0x18 + 0x18 * stream);
    volatile uint32_t* DMA_SxM0AR = (volatile uint32_t* )(base+0x1C + 0x18 * stream);
    volatile uint32_t* DMA_SxM1AR = (volatile uint32_t* )(base+0x20 + 0x18 * stream);
    volatile uint32_t* DMA_SxFCR = (volatile uint32_t* )(base+0x24 + 0x18 * stream);
    uint32_t tmp;
    *DMA_SxCR = 0;  /* EN=0 */

    *DMA_SxNDTR = dma_cfg->cnt;
    *DMA_SxPAR = dma_cfg->paddr;
    *DMA_SxM0AR = dma_cfg->m0addr;
    *DMA_SxM1AR = dma_cfg->m1addr;

    tmp =  ((uint32_t)(dma_cfg->fifo_cfg.dmdis) << 2) | ((uint32_t)(dma_cfg->fifo_cfg.feie) << 7) | 
    ((uint32_t)(dma_cfg->fifo_cfg.fth) << 0);
    *DMA_SxFCR = tmp;

    tmp = ((uint32_t)(dma_cfg->cfg.chsel) << 25) | 
    ((uint32_t)(dma_cfg->cfg.mburst) << 23) | 
    ((uint32_t)(dma_cfg->cfg.pburst) << 21) | 
    ((uint32_t)(dma_cfg->cfg.ct) << 19) | 
    ((uint32_t)(dma_cfg->cfg.dbm) << 18) | 
    ((uint32_t)(dma_cfg->cfg.pl) << 16) | 
    ((uint32_t)(dma_cfg->cfg.pincos) << 15) | 
    ((uint32_t)(dma_cfg->cfg.msize) << 13) | 
    ((uint32_t)(dma_cfg->cfg.psize) << 11) | 
    ((uint32_t)(dma_cfg->cfg.minc) << 10) | 
    ((uint32_t)(dma_cfg->cfg.pinc) << 9) | 
    ((uint32_t)(dma_cfg->cfg.circ) << 8) | 
    ((uint32_t)(dma_cfg->cfg.dir) << 6) | 
    ((uint32_t)(dma_cfg->cfg.pfctrl) << 5) | 
    ((uint32_t)(dma_cfg->cfg.tcie) << 4) | 
    ((uint32_t)(dma_cfg->cfg.htie) << 3) | 
    ((uint32_t)(dma_cfg->cfg.teie) << 2) | 
    ((uint32_t)(dma_cfg->cfg.dmeie) << 1) | 0x01;
    *DMA_SxCR = tmp;
    
}

void dma_int_flag_clr(uint32_t base, int stream, dma_int_e it){
    volatile uint32_t* DMA_LIFCR = (volatile uint32_t* )(base+8);
    volatile uint32_t* DMA_HIFCR = (volatile uint32_t* )(base+0x0C);
    if(stream <= 3){
        if(stream <= 1){
            *DMA_LIFCR = (1u<<(it+stream*6));
        }else{
            *DMA_LIFCR = (1u<<(16 + it+(stream-2)*6));
        }
    }else{
        stream -= 4;
        if(stream <= 1){
            *DMA_HIFCR = (1u<<(it+stream*6));
        }else{
            *DMA_HIFCR = (1u<<(16 + it+(stream-2)*6));
        }
    }

}

int dma_int_is_set(uint32_t base, int stream, dma_int_e it){
    uint32_t ret;
    volatile uint32_t* DMA_LISR = (volatile uint32_t* )base;
    volatile uint32_t* DMA_HISR = (volatile uint32_t* )(base+4);
    if(stream <= 3){
        if(stream <= 1){
            ret = *DMA_LISR & (1u<<(it+stream*6));
        }else{
            ret = *DMA_LISR & (1u<<(16 + it+(stream-2)*6));
        }
    }else{
        stream -= 4;
        if(stream <= 1){
            ret = *DMA_HISR & (1u<<(it+stream*6));
        }else{
            ret = *DMA_HISR & (1u<<(16 + it+(stream-2)*6));
        }
    }
    if(ret !=0 ){
        return 1;
    }else{
        return 0;
    }
}
