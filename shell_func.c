#include <stdio.h>
#include <string.h>

#include "shell.h"
#include "shell_func.h"
#include "xprintf.h"
#include "spi.h"
#include "spiflash_itf.h"
#include "uart.h"
#include "clock.h"
#include "xmodem.h"
#include "MLX90642_test.h"

static void helpfunc(uint8_t* param);

static void printmemfunc(uint8_t* param);
static void setmemfunc(uint8_t* param);
static void rxmemfunc(uint8_t* param);
static void sxmemfunc(uint8_t* param);

static void printflashfunc(uint8_t* param);
static void writeflashfunc(uint8_t* param);
static void rxspiflashfunc(uint8_t* param);
static void sxspiflashfunc(uint8_t* param);
static void restorespiflashfunc(uint8_t* param);
static void dumpspiflashfunc(uint8_t* param);

static void setbaudfunc(uint8_t* param);

static void mlx90642testfunc(uint8_t* param);

/**
 * 最后一行必须为0,用于结束判断
*/
const shell_cmd_cfg g_shell_cmd_list_ast[ ] = 
{
  { (uint8_t*)"help",         helpfunc,         (uint8_t*)"help"}, 

  { (uint8_t*)"printmem",     printmemfunc,     (uint8_t*)"printmem mode[hex/dec] addr[hex] len datasize[8/16/32] sig[1/0]"}, 
  { (uint8_t*)"setmem",       setmemfunc,       (uint8_t*)"setmem addr[hex] val[hex]"},
  { (uint8_t*)"rxmem",        rxmemfunc,        (uint8_t*)"rxmem addr[hex] len"}, 
  { (uint8_t*)"sxmem",        sxmemfunc,        (uint8_t*)"sxmem addr[hex] len"}, 

  { (uint8_t*)"printflash",   printflashfunc,   (uint8_t*)"printflash addr[hex] len"}, 
  { (uint8_t*)"writeflash",   writeflashfunc,   (uint8_t*)"writeflash addr[hex] hexstr"}, 
  { (uint8_t*)"rxspiflash",   rxspiflashfunc,   (uint8_t*)"rxspiflash addr[hex] len"}, 
  { (uint8_t*)"sxspiflash",   sxspiflashfunc,   (uint8_t*)"sxspiflash addr[hex] len"}, 
  { (uint8_t*)"restorespiflash",restorespiflashfunc,(uint8_t*)"restorespiflash ramaddr[hex] flashaddr[hex] len"}, 
  { (uint8_t*)"dumpspiflash",   dumpspiflashfunc,   (uint8_t*)"dumpspiflash flashaddr[hex] ramaddr[hex]  len"}, 

  { (uint8_t*)"setbaud",      setbaudfunc,      (uint8_t*)"setbaud baud"}, 

  { (uint8_t*)"mlx90642test",  mlx90642testfunc,  (uint8_t*)"mlx90642test num"}, 

  { (uint8_t*)0,		          0 ,               0},
};

static int ascii2uc(const char c, unsigned char *uc)
{
    if ((c >= '0') && (c <= '9')) {
        *uc = c - '0';
    } else if ((c >= 'a') && (c <= 'f')) {
        *uc = c - 'a' + 10;
    } else if ((c >= 'A') && (c <= 'F')) {
        *uc = c - 'A' + 10;
    } else {
        return -1;
    }

    return 0;
}

static uint32_t str2hex(const char* str, unsigned char *buff, uint32_t len)
{
  uint32_t num = 0;
  uint8_t hex = 0;
  while(1)
  {
    uint8_t tmp1;
    uint8_t tmp2;
    if(ascii2uc(*str++, &tmp1) < 0)
    {
      break;
    }
    if(ascii2uc(*str++, &tmp2) < 0)
    {
      break;
    }
    hex = tmp1*16 + tmp2;
    *buff++ = hex;
    num++;
    if(num >= len)
    {
      break;
    }
  }
  return num;
}

static uint8_t rxtx_buf[1029];

static uint32_t getms(void)
{
  return get_ticks();
}

static uint32_t io_read(uint8_t* buffer, uint32_t len)
{
  return uart_read(1,buffer, len);
}

static uint32_t io_getrxlen(void)
{
  return uart_getrxlen(1);
}

static void io_read_flush(void)
{
  uint8_t tmp;
  while(0 != uart_read(1,&tmp, 1));
}

static uint32_t io_write(uint8_t* buffer, uint32_t len)
{
  uart_send(1,buffer, len);
  return len;
}

static void helpfunc(uint8_t* param)
{
	  (void)param;
    unsigned int i;
    xprintf("\r\n");
    xprintf("**************\r\n");
    xprintf("*   SHELL    *\r\n");
    xprintf("*   V1.0     *\r\n");
    xprintf("**************\r\n");
    xprintf("\r\n");
    for (i=0; g_shell_cmd_list_ast[i].name != 0; i++)
    {
        xprintf("%02d.",i);
        xprintf("%-16s",g_shell_cmd_list_ast[i].name);
        xprintf("%s\r\n",g_shell_cmd_list_ast[i].helpstr);
    }
}

static void printmemfunc(uint8_t* param)
{
  uint32_t addr;
  uint32_t len;
  uint8_t mode[64];
  int datasize;
  uint8_t* tmp8_u;
  uint16_t* tmp16_u;
  uint32_t* tmp32_u;
  int8_t* tmp8_i;
  int16_t* tmp16_i;
  int32_t* tmp32_i;

  int sig;
#if 0
  if(5 == sscanf((const char*)param, "%*s %s %lx %ld %d %d", mode, &addr, &len, &datasize, &sig))
#else
  char* p =(char*)param;
  while(1){  /* 跳过%*s部分 */
    if((*p > 'z') || (*p < 'a')){
      break;
    }else{
      p++;
    }
  }
	while(1){  /* Skip leading spaces */
    if(*p != ' '){
      break;
    }else{
      p++;
    }
  }
  uint8_t* p_mode = mode;
  while(1){  /* mode部分 */
    if((*p > 'z') || (*p < 'a')){
      break;
      *p_mode = 0;
    }else{
      *p_mode = *p;
      p_mode++;
      p++;
    }
  }
  long tmp;
  xatoi(&p, &tmp);
  addr = tmp;
  xatoi(&p, &tmp);
  len = tmp;
  xatoi(&p, &tmp);
  datasize = tmp;
  xatoi(&p, &tmp);
  sig = tmp;
#endif
  {
    if(strncmp((const char*)mode,"hex", 3) == 0)
    {
      if(datasize == 8)
      {
        tmp8_u = (uint8_t*)addr;
        for(uint32_t i=0; i<len ;i++)
        {
          if(i%16 == 0)
          {
            xprintf("\r\n[%08x]:",addr+i*1);
          }
          xprintf("%02x ",tmp8_u[i]);
        }
        xprintf("\r\n");
      }
      else if(datasize == 16)
      {
        tmp16_u = (uint16_t*)addr;
        for(uint32_t i=0; i<len ;i++)
        {
          if(i%16 == 0)
          {
            xprintf("\r\n[%08x]:",addr+i*2);
          }
          xprintf("%04x ",tmp16_u[i]);
        }
        xprintf("\r\n");
      }
      else if(datasize == 32)
      {
        tmp32_u = (uint32_t*)addr;
        for(uint32_t i=0; i<len ;i++)
        {
          if(i%16 == 0)
          {
            xprintf("\r\n[%08x]:",addr+i*4);
          }
          xprintf("%08x ",tmp32_u[i]);
        }
        xprintf("\r\n");
      }
      else
      {
        xprintf("datasize must be 8/16/32\r\n");
      }
    }
    else if(strncmp((const char*)mode,"dec", 3) == 0)
    {
      if(datasize == 8)
      {
        if(sig == 0)
        {
          tmp8_u = (uint8_t*)addr;
          for(uint32_t i=0; i<len ;i++)
          {
            if(i%16 == 0)
            {
              xprintf("\r\n");
            }
            xprintf("%d ",tmp8_u[i]);
          }
          xprintf("\r\n");
        }
        else
        {
          tmp8_i = (int8_t*)addr;
          for(uint32_t i=0; i<len ;i++)
          {
            if(i%16 == 0)
            {
              xprintf("\r\n");
            }
            xprintf("%d ",tmp8_i[i]);
          }
          xprintf("\r\n");
        }
      }
      else if(datasize == 16)
      {
        if(sig == 0)
        {
          tmp16_u = (uint16_t*)addr;
          for(uint32_t i=0; i<len ;i++)
          {
            if(i%16 == 0)
            {
              xprintf("\r\n");
            }
            xprintf("%d ",tmp16_u[i]);
          }
          xprintf("\r\n");
        }
        else
        {
          tmp16_i = (int16_t*)addr;
          for(uint32_t i=0; i<len ;i++)
          {
            if(i%16 == 0)
            {
              xprintf("\r\n");
            }
            xprintf("%d ",tmp16_i[i]);
          }
          xprintf("\r\n");
        }
      }
      else if(datasize == 32)
      {
        if(sig == 0)
        {
          tmp32_u = (uint32_t*)addr;
          for(uint32_t i=0; i<len ;i++)
          {
            if(i%16 == 0)
            {
              xprintf("\r\n");
            }
            xprintf("%d ",tmp32_u[i]);
          }
          xprintf("\r\n");
        }
        else
        {
          tmp32_i = (int32_t*)addr;
          for(uint32_t i=0; i<len ;i++)
          {
            if(i%16 == 0)
            {
              xprintf("\r\n");
            }
            xprintf("%d ",tmp32_i[i]);
          }
          xprintf("\r\n");
        }
      }
      else
      {
        xprintf("datasize must be 8/16/32\r\n");
      }
    }
    else
    {
      xprintf("mode must be [hex/dec]\r\n");
    }
  }
}

static void setmemfunc(uint8_t* param)
{
  uint32_t addr;
  uint32_t val;

#if 0
  if(2 == sscanf((const char*)param, "%*s %lx %lx", &addr, &val))
#else
  char* p =(char*)param;
  while(1){  /* 跳过%*s部分 */
    if((*p > 'z') || (*p < 'a')){
      break;
    }else{
      p++;
    }
  }
	while(1){  /* Skip leading spaces */
    if(*p != ' '){
      break;
    }else{
      p++;
    }
  }
  long tmp;
  xatoi(&p, &tmp);
  addr = tmp;
  xatoi(&p, &tmp);
  val = tmp;
#endif
  {
    xprintf("setmem  %x %x\r\n",addr,val);
    if((addr % 4) ==0)
    {
      *(volatile uint32_t*)addr = val;
      xprintf("%x\r\n",*(volatile uint32_t*)addr);
    }
    else
    {
      xprintf("addr must be mul of 4\r\n");
    }
  }
}

static uint32_t mem_read(uint32_t addr, uint8_t* buffer, uint32_t len)
{
  memcpy(buffer, (uint8_t*)addr, len);
  return len;
}

static uint32_t mem_write(uint32_t addr, uint8_t* buffer, uint32_t len)
{
  memcpy((uint8_t*)addr, buffer, len);
  return len;
}

static void rxmemfunc(uint8_t* param)
{
  uint32_t addr;
  uint32_t len;
  int res = 0;
#if 0
  if(2 == sscanf((const char*)param, "%*s %x %d", &addr, &len))
#else
  char* p =(char*)param;
  while(1){  /* 跳过%*s部分 */
    if((*p > 'z') || (*p < 'a')){
      break;
    }else{
      p++;
    }
  }
  long tmp;
  xatoi(&p, &tmp);
  addr = tmp;
  xatoi(&p, &tmp);
  len = tmp;
#endif
  {
    xprintf("rxmem to 0x%x %d\r\n",addr,len);
    xmodem_cfg_st cfg=
      {
        .buffer = rxtx_buf,
        .crccheck = 0,
        .getms = getms,
        .io_read = io_read,
        .io_getrxlen = io_getrxlen,
        .io_read_flush = io_read_flush,
        .io_write = io_write,
        .start_timeout = 60,
        .packet_timeout = 1000,
        .ack_timeout = 1000,
        .mem_write = mem_write,
        .addr = addr,
        .totallen = len,
      };
      xmodem_init_rx(&cfg);
      while((res = xmodem_rx()) == 0);
      xprintf("res:%d\r\n",res);
  }
}

static void sxmemfunc(uint8_t* param)
{
  uint32_t addr;
  uint32_t len;
  int res = 0;
#if 0
  if(2 == sscanf((const char*)param, "%*s %x %d", &addr, &len))
#else
  char* p =(char*)param;
  while(1){  /* 跳过%*s部分 */
    if((*p > 'z') || (*p < 'a')){
      break;
    }else{
      p++;
    }
  }
  long tmp;
  xatoi(&p, &tmp);
  addr = tmp;
  xatoi(&p, &tmp);
  len = tmp;
#endif
  {
    xprintf("sxmem to 0x%x %d\r\n",addr,len);
    xmodem_cfg_st cfg=
    {
      .buffer = rxtx_buf,
      .plen = 1024,
      .getms = getms,
      .io_read = io_read,
      .io_getrxlen = io_getrxlen,
      .io_read_flush = io_read_flush,
      .io_write = io_write,
      .start_timeout = 60,
      .packet_timeout = 1000,
      .ack_timeout = 5000,
      .mem_read = mem_read,
      .addr = addr,
      .totallen = len,
    };
    xmodem_init_tx(&cfg);
    while((res = xmodem_tx()) == 0);
    xprintf("res:%d\r\n",res);
  }
}

static void printflashfunc(uint8_t* param)
{
  uint8_t buffer[16];
  uint32_t addr;
  uint32_t len;
#if 0
  if(3 == sscanf((const char*)param, "%*s %lx %ld", &addr, &len))
#else 
  char* p =(char*)param;
  while(1){  /* 跳过%*s部分 */
    if((*p > 'z') || (*p < 'a')){
      break;
    }else{
      p++;
    }
  }
	while(1){  /* Skip leading spaces */
    if(*p != ' '){
      break;
    }else{
      p++;
    }
  }
  long tmp;
  xatoi(&p, &tmp);
  addr = tmp;
  xatoi(&p, &tmp);
  len = tmp;
#endif
  {
    uint32_t toread;
    uint32_t read = 0;
    while(read < len)
    {
      toread = ((len-read) > sizeof(buffer)) ? sizeof(buffer) : (len-read);
      flash_itf_read(buffer, addr+read, toread);
      read += toread;
      xprintf("[%08x]",addr+read);
      for(uint32_t i=0; i<toread ;i++)
      {
        xprintf("%02x ",buffer[i]);
      }
      for(uint32_t i=toread; i<sizeof(buffer) ;i++)
      {
        xprintf("   ");
      }
      xprintf(":");
      for(uint32_t i=0; i<toread ;i++)
      {
        if((buffer[i] > 0x1F) && (buffer[i] < 0x7F)){
          xprintf("%c",buffer[i]);
        }else{
          xprintf("%c",'?');
        }
      }
      xprintf("\r\n");
    }
  }
}

static void writeflashfunc(uint8_t* param)
{
  uint8_t buffer[32+1];
  uint8_t hex[16];
	
  uint32_t addr;
  uint32_t len;
#if 0
  if(2 == sscanf((const char*)param, "%*s %x %s", &addr, buffer))
#else
  char* p =(char*)param;
  while(1){  /* 跳过%*s部分 */
    if((*p > 'z') || (*p < 'a')){
      break;
    }else{
      p++;
    }
  }
  long tmp;
  xatoi(&p, &tmp);
  addr = tmp;
	while(1){  /* Skip leading spaces */
    if(*p != ' '){
      break;
    }else{
      p++;
    }
  }
  uint8_t* p_str = buffer;
  while(1){  /* hexstr部分 */
    if(((*p >= '0') && (*p <= '9')) || 
       ((*p >= 'a') && (*p <= 'f')) || 
       ((*p >= 'A') && (*p <= 'F'))){
      *p_str = *p;
      p_str++;
      p++;
    }else{
      break;
      *p_str = 0;
    }
  }
#endif
  {
		len = str2hex((const char*)buffer, hex, strlen((char*)buffer));
		if(len>0){
			flash_itf_write(hex,addr,len);
		}else{
    }
    xprintf("write len=%d\r\n",len);
  }
}

static uint32_t flash_read(uint32_t addr, uint8_t* buffer, uint32_t len)
{
  flash_itf_read(buffer, addr, len);
  return len;
}

static uint32_t flash_write(uint32_t addr, uint8_t* buffer, uint32_t len)
{
  flash_itf_write(buffer, addr, len);
  return len;
}

static void rxspiflashfunc(uint8_t* param)
{
  uint32_t addr;
  uint32_t len;
  int res = 0;
#if 0
  if(2 == sscanf((const char*)param, "%*s %lx %d", &addr, &len))
#else 
  char* p =(char*)param;
  while(1){  /* 跳过%*s部分 */
    if((*p > 'z') || (*p < 'a')){
      break;
    }else{
      p++;
    }
  }
  long tmp;
  xatoi(&p, &tmp);
  addr = tmp;
  xatoi(&p, &tmp);
  len = tmp;
#endif
  {
    xmodem_cfg_st cfg=
    {
      .buffer = rxtx_buf,
      .crccheck = 0,
      .getms = getms,
      .io_read = io_read,
      .io_getrxlen = io_getrxlen,
      .io_read_flush = io_read_flush,
      .io_write = io_write,
      .start_timeout = 60,
      .packet_timeout = 1000,
      .ack_timeout = 1000,
      .mem_write = flash_write,
      .addr = addr,
      .totallen = len,
    };
    xmodem_init_rx(&cfg);
    while((res = xmodem_rx()) == 0);
    xprintf("res:%d\r\n",res);
  }
}

static void sxspiflashfunc(uint8_t* param)
{
  uint32_t addr;
  uint32_t len;
  int res = 0;
  #if 0
  if(2 == sscanf((const char*)param, "%*s %lx %d", &addr, &len))
  #else
  char* p =(char*)param;
  while(1){  /* 跳过%*s部分 */
    if((*p > 'z') || (*p < 'a')){
      break;
    }else{
      p++;
    }
  }
  long tmp;
  xatoi(&p, &tmp);
  addr = tmp;
  xatoi(&p, &tmp);
  len = tmp;
  #endif
  {
    xmodem_cfg_st cfg=
    {
      .buffer = rxtx_buf,
      .plen = 1024,
      .getms = getms,
      .io_read = io_read,
      .io_getrxlen = io_getrxlen,
      .io_read_flush = io_read_flush,
      .io_write = io_write,
      .start_timeout = 60,
      .packet_timeout = 1000,
      .ack_timeout = 1000,
      .mem_read = flash_read,
      .addr = addr,
      .totallen = len,
    };
    xmodem_init_tx(&cfg);
    while((res = xmodem_tx()) == 0);
    xprintf("res:%d\r\n",res);
  }
}

static void restorespiflashfunc(uint8_t* param)
{
  uint32_t flashaddr;
  uint32_t ramaddr;
  uint32_t len;
  #if 0
  if(3 == sscanf((const char*)param, "%*s %lx %lx %d", &ramaddr, &flashaddr &len))
  #else
  char* p =(char*)param;
  while(1){  /* 跳过%*s部分 */
    if((*p > 'z') || (*p < 'a')){
      break;
    }else{
      p++;
    }
  }
  long tmp;
  xatoi(&p, &tmp);
  ramaddr = tmp;
  xatoi(&p, &tmp);
  flashaddr = tmp;
  xatoi(&p, &tmp);
  len = tmp;
  #endif
  {
    xprintf("restore %x to %x len %d\r\n",ramaddr,flashaddr,len);
    flash_itf_write((uint8_t*)ramaddr,flashaddr,len);
  }
}

static void dumpspiflashfunc(uint8_t* param)
{
  uint32_t flashaddr;
  uint32_t ramaddr;
  uint32_t len;
  #if 0
  if(3 == sscanf((const char*)param, "%*s %lx %lx %d", &flashaddr, &ramaddr &len))
  #else
  char* p =(char*)param;
  while(1){  /* 跳过%*s部分 */
    if((*p > 'z') || (*p < 'a')){
      break;
    }else{
      p++;
    }
  }
  long tmp;
  xatoi(&p, &tmp);
  flashaddr = tmp;
  xatoi(&p, &tmp);
  ramaddr = tmp;
  xatoi(&p, &tmp);
  len = tmp;
  #endif
  {
    xprintf("dump %x to %x len %d\r\n",flashaddr,ramaddr,len);
    flash_itf_read((uint8_t*)ramaddr,flashaddr,len);
  }
}

static void setbaudfunc(uint8_t* param)
{
  uint32_t baud;
  #if 0
  if(1 == sscanf((const char*)param, "%*s %d", &baud))
  #else
  char* p =(char*)param;
  while(1){  /* 跳过%*s部分 */
    if((*p > 'z') || (*p < 'a')){
      break;
    }else{
      p++;
    }
  }
  long tmp;
  xatoi(&p, &tmp);
  baud = tmp;
  #endif
  {
    uart_init(1, baud);
  }
}

static void mlx90642testfunc(uint8_t* param)
{
  uint32_t num;
  #if 0
  if(1 == sscanf((const char*)param, "%*s %d", &num))
  #else
  char* p =(char*)param;
  while(1){  /* 跳过%*s部分 */
    if(*p == ' '){
      break;
    }else{
      p++;
    }
  }
  long tmp;
  xatoi(&p, &tmp);
  num = tmp;
  #endif
  {
    mlx90642_test(num);
  }
}
