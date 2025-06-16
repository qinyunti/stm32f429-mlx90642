#ifndef DMA_H
#define DMA_H

#include <stdint.h>

typedef enum{
    DMA_FIFO_FTH_1_4=0, /**<  00: 1/4 full FIFO */
    DMA_FIFO_FTH_1_2=1, /**<  01: 1/2 full FIFO */
    DMA_FIFO_FTH_3_4=2, /**<  10: 3/4 full FIFO */
    DMA_FIFO_FTH_FULL=3, /**<  11: full FIFO    */
} dma_fifo_fth_e;

typedef enum{
    DMA_MBURST_SINGLE = 0, 
    DMA_MBURST_INCR4 = 1,
    DMA_MBURST_INCR8 = 2,
    DMA_MBURST_INCR16 = 3,
} dma_mburst_e;

typedef enum{
    DMA_PBURST_SINGLE = 0, 
    DMA_PBURST_INCR4 = 1,
    DMA_PBURST_INCR8 = 2,
    DMA_PBURST_INCR16 = 3,
} dma_pburst_e;

typedef enum{
    DMA_PL_LOW = 0, 
    DMA_PL_MEDIUM = 1,
    DMA_PL_HIGH = 2,
    DMA_PL_VERYHIGH = 3,
} dma_pl_e;

typedef enum{
    DMA_PSIZE_BYTE = 0, 
    DMA_PSIZE_HALFWORD = 1,
    DMA_PSZIE_WORD = 2,
} dma_psize_e;

typedef enum{
    DMA_MSIZE_BYTE = 0, 
    DMA_MSIZE_HALFWORD = 1,
    DMA_MSZIE_WORD = 2,
} dma_msize_e;

typedef enum{
    DMA_PINCOS_PSIZE = 0, 
    DMA_PINCOS_FIXED4 = 1,
} dma_pincos_e;

typedef enum{
    DMA_MINC_FIXED = 0, 
    DMA_MINC_MSIZE = 1,
} dma_minc_e;

typedef enum{
    DMA_PINC_FIXED = 0, 
    DMA_PINC_PSIZE = 1,
} dma_pinc_e;

typedef enum{
    DMA_DIR_P2M = 0, 
    DMA_DIR_M2P = 1,
    DMA_DIR_M2M = 2,
} dma_dir_e;

typedef enum{
    DMA_PFCTRL_DMA = 0, 
    DMA_PFCTRL_P = 1,
} dma_pfctrl_e;

typedef enum{
    DMA_INT_TC = 5, 
    DMA_INT_HT = 4,
    DMA_INT_TE = 3,
    DMA_INT_MDE = 2,
    DMA_INT_FE = 0,
} dma_int_e;

typedef struct dma_fifo
{
    uint8_t feie;
    uint8_t dmdis;        /**< EN=0可写 */
    dma_fifo_fth_e fth;   /**< EN=0可写 */
} dma_fifo_st;

typedef struct dma_cfg
{
    uint8_t chsel; /**< 0~7 EN=0可写 */
    dma_mburst_e mburst; /**< EN=0可写 */
    dma_pburst_e pburst; /**< EN=0可写 */
    uint8_t ct;   /**< 0~1 EN=0可写 */
    uint8_t dbm;   /**< 0~1 EN=0可写 */
    dma_pl_e pl;   /**< EN=0可写 */
    dma_pincos_e pincos;  /**< EN=0可写 */
    dma_psize_e psize; /**< EN=0可写 */
    dma_msize_e msize; /**< EN=0可写 */
    dma_minc_e minc;  /**< EN=0可写 */
    dma_pinc_e pinc;  /**< EN=0可写 */
    uint8_t circ;
    dma_dir_e dir;
    dma_pfctrl_e pfctrl;
    uint8_t tcie;
    uint8_t htie;
    uint8_t teie;
    uint8_t dmeie;
} dma_cfg_st;

typedef struct dma{
    dma_cfg_st cfg;
    uint32_t cnt;           /**< stream is disabled可写 */
    uint32_t paddr;        /**< EN=0可写 */
    uint32_t m0addr;       /**< EN=0或者EN=1 CT=0可写 */
    uint32_t m1addr;      /**< EN=0或者EN=1 CT=0可写 */
    dma_fifo_st fifo_cfg;
} dma_st;

void dma_cfg(uint32_t base, int stream, dma_st* dma_cfg);
void dma_dis(uint32_t base, int stream);
void dma_int_flag_clr(uint32_t base, int stream, dma_int_e it);
int dma_int_is_set(uint32_t base, int stream, dma_int_e it);

#endif