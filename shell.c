#include <stdint.h>
#include "shell.h"

shell_read_pf s_input_pf = 0;      /* 输入接口指针     */
shell_write_pf s_output_pf = 0;    /* 输出接口指针     */
shell_cmd_cfg* s_cmd_cfg_pst = 0;  /* 命令列表指针     */
uint8_t s_enableecho_u8 = 0;       /* 是否使能echo标志 */
static uint8_t  s_cmd_buf_au8[SHELL_CMD_LEN]="\r"; /* 命令缓冲区 */
static uint32_t s_cmd_buf_index_u32 = 0;               /* 当前命令缓冲区中字符数 */

/**
 * 输出字符接口
*/
static void shell_putchar(uint8_t val)
{
    uint8_t tmp;
    if(s_output_pf != 0)
    {
        tmp = val;
        s_output_pf(&tmp, 1);
    }
}

/**
 * 输出字符串接口
*/
static void shell_putstring(char* str)
{
    uint32_t len = 0;
    uint8_t*p = (uint8_t*)str;
    while(*str++)
    {
        len++;
    }
    s_output_pf(p, len);
}

/**
 * 读字符接口
*/
static int shell_getchar(uint8_t *data)
{
    if(s_input_pf == 0)
    {
        return -1;
    }
	if(0 == s_input_pf(data, 1))
    {
		return -1;
	}
	else
	{
        return 0;
	}
}

/**
 * 判断命令字符串的长度
 * 命令字符串不能有空格
*/
static uint32_t shell_cmd_len(uint8_t *cmd)
{
    uint8_t *p = cmd;
    uint32_t len = 0;
    while((*p != ' ') && (*p != 0)) 
    {
        p++;
        len++;
    }
    return len;
}

/**
 * 判断两个字符串是否相等,相等返回0
*/
static int shell_cmd_check(uint8_t *cmd, uint8_t *str)
{
    uint32_t len1 = shell_cmd_len(cmd);
    uint32_t len2 = shell_cmd_len(str);
    if(len1 != len2)
    {
        return -1;
    }
    for(uint32_t i=0; i<len1; i++)
    {
        if(*cmd++ != *str++)
        {
            return -1;
        }
    }
    return 0;
}

/**
 * 读取一行命令
*/
static uint32_t shell_read_line(void)
{
    uint8_t ch;
    uint32_t count;
    /* 初始打印sh> */
    if(s_cmd_buf_au8[0]=='\r')
    {
        shell_putstring("sh>\r\n");
        s_cmd_buf_au8[0] = 0;
    }

    /* 非阻塞读取一个字符 */
    if(shell_getchar(&ch) !=0 )
    {
        return 0;
    }

    /* 遇到除了退格之外的不可打印字符,则认为收到一行命令 
     * 退格需要单独处理,需要删除一个字符
    */
    if((ch == '\r' || ch == '\n' || ch < ' ' || ch > '~') && (ch != '\b'))
    {
        if(s_cmd_buf_index_u32==0)
        {
            /* 缓冲区没有数据就收到了非打印字符串,则打印提示sh> */
            shell_putstring("sh>\r\n");
        }
        else
        {
            /* 收到了非打印字符,且缓冲区有数据则认为收到了一行
             * 返回缓冲区数据长度,并清零计数,打印回车换行
             * 并且添加结束符0
            */
            count = s_cmd_buf_index_u32;
            s_cmd_buf_au8[s_cmd_buf_index_u32]=0;
            s_cmd_buf_index_u32 =0;
            shell_putstring("\r\n");
            return count;
        }
    }
    else 
    {
        if(ch == '\b') 
        {
            /* 退格处理,注意只有有数据才会删除一个字符，添加结束符 */
            if(s_cmd_buf_index_u32 != 0) 
            {
                s_cmd_buf_index_u32--;
                shell_putchar('\b');
                shell_putchar(' ');
                shell_putchar('\b');
                s_cmd_buf_au8[s_cmd_buf_index_u32]= '\0';
            }
        } 
        else 
        {
            /* 可打印字符，添加到缓冲区
             * 如果数据量已经到了缓冲区大小-1,则也认为是一行命令
             * -1是保证最后有结束符0空间
            */
            if(s_enableecho_u8 != 0)
            {
                shell_putchar(ch);
            }
            s_cmd_buf_au8[s_cmd_buf_index_u32++] = ch;
            if(s_cmd_buf_index_u32>=(sizeof(s_cmd_buf_au8)-1))
            {
                count = s_cmd_buf_index_u32;
                s_cmd_buf_au8[s_cmd_buf_index_u32]=0;
                s_cmd_buf_index_u32 =0;
                shell_putstring("\r\n");
                return count;
            }
        } 
    } 
    return 0;
}

/**
 * 搜寻命令列表处理命令
*/
static int shell_exec_cmdlist(uint8_t* cmd)
{
    int i;
    if(s_cmd_cfg_pst == 0)
    {
        return -1;
    }
    for (i=0; s_cmd_cfg_pst[i].name != 0; i++)
    {
        if (shell_cmd_check(cmd, s_cmd_cfg_pst[i].name) == 0) 
        {
            s_cmd_cfg_pst[i].func(cmd);
            return 0;
        }            
    } 
    if(s_cmd_cfg_pst[i].name == 0)
    {
        shell_putstring("unkown command\r\n");
        return -1;
    }
    return 0;
}

/**
 * 对外接口,周期执行
*/
void shell_exec(void)
{
    if(shell_read_line() > 0)
    {
        shell_exec_cmdlist(s_cmd_buf_au8);
    }
}

/**
 * 对外接口,初始化配置接口
*/
void shell_set_itf(shell_read_pf input, shell_write_pf output, shell_cmd_cfg* cmd_list, uint8_t enableecho)
{
    s_input_pf = input;
    s_output_pf = output;
    s_cmd_cfg_pst = cmd_list;
    s_enableecho_u8 = enableecho;
}