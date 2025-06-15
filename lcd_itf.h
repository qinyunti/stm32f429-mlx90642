#ifndef LCD_ITF_H
#define LCD_ITF_H

#ifdef __cplusplus
    extern "C"{
#endif

#include <stdint.h>

/**
 * \fn lcd_itf_init
 * 初始化
 * \retval 0 成功
 * \retval 其他值 失败
*/
int lcd_itf_init(void);

/**
 * \fn lcd_itf_deinit
 * 解除初始化
 * \retval 0 成功
 * \retval 其他值 失败
*/
int lcd_itf_deinit(void);

/**
 * \fn lcd_itf_sync
 * 刷新显示
 * \retval 0 成功
 * \retval 其他值 失败
*/
int lcd_itf_sync(void);

/**
 * \fn lcd_itf_set_pixel
 * 写点
 * \param[in] x x坐标位置
 * \param[in] y y坐标位置
 * \param[in] rgb565 颜色
*/
void lcd_itf_set_pixel(uint16_t x, uint16_t y, uint16_t rgb565);

/**
 * \fn lcd_itf_set_pixel_0
 * 写点
 * \param[in] offset 偏移位置
 * \param[in] rgb565 颜色
*/
void lcd_itf_set_pixel_0(uint32_t offset, uint16_t rgb565);

/**
 * \fn lcd_itf_get_pixel
 * 读点
 * \param[in] x x坐标位置
 * \param[in] y y坐标位置
 * \return rgb565颜色
*/
uint16_t lcd_itf_get_pixel(uint16_t x, uint16_t y);


/**
 * \fn lcd_itf_fill_direct
 * 直接填充区域
 * \param[in] x x开始坐标位置
 * \param[in] w 宽度
 * \param[in] y y开始坐标位置
 * \param[in] h 高度
 * \param[in] buffer rgb565颜色缓存区
*/
void lcd_itf_fill_direct(uint16_t x, uint16_t w, uint16_t y, uint16_t h, uint16_t* buffer);

/**
 * \fn lcd_itf_set_pixel_direct
 * 直接写点
 * \param[in] x x坐标位置
 * \param[in] y y坐标位置
 * \param[in] rgb565 颜色
*/
void lcd_itf_set_pixel_direct(uint16_t x, uint16_t y, uint16_t rgb565);

#ifdef __cplusplus
    }
#endif

#endif

