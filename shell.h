#ifndef SHELL_H
#define SHELL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define SHELL_CMD_LEN 64                                          /**< 命令缓冲区大小 */

typedef void (*shell_command_pf)(uint8_t *);                      /**< 命令回调函数   */
typedef uint32_t (*shell_read_pf)(uint8_t *buff, uint32_t len);   /**< 底层收接口     */
typedef void (*shell_write_pf)(uint8_t *buff, uint32_t len);      /**< 底层发接口     */

/**
 * \struct shell_cmd_cfg
 * 命令信息
*/
typedef struct
{
    uint8_t * name;          /**< 命令字符串   */
    shell_command_pf func;   /**< 命令回调函数 */ 
    uint8_t * helpstr;       /**< 命令帮助信息 */   
}shell_cmd_cfg;

/**
 * \fn shell_exec
 * 周期调用该函数,读取底层输入,并判断是否有命令进行处理
 * 非阻塞
*/
void shell_exec(void);

/**
 * \fn shell_set_itf
 * 设置底层输入输出接口,以及命令列表
 * 调用shell_exec_shellcmd之前,需要先调用该接口初始化
 * \param[in] input \ref shell_read_pf 输入接口
 * \param[in] output \ref shell_write_pf 输出接口
 * \param[in] cmd_list \ref shell_cmd_cfg 命令列表
 * \param[in] enableecho 0:不使能回显, 其他值:使能回显
*/
void shell_set_itf(shell_read_pf input, shell_write_pf output, shell_cmd_cfg* cmd_list, uint8_t enableecho);

#ifdef __cplusplus
}
#endif

#endif