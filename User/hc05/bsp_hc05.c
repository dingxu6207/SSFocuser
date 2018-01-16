#include "Bsp_hc05.h"
#include "Bsp_usart_blt.h"
#include <string.h>
#include <stdio.h>


void HC05_Init(void)
{
	BLT_USART_Config();		
}

 /**
  * @brief  从stream中获取一行字符串到line中
	* @param  line,存储获得行的字符串数组
	* @param  stream，原字符串数据流
	* @param 	max_size，stream的大小 
  * @retval line的长度，若stream中没有‘\0’，'\r'，'\n'，则返回0
  */
int get_line(char* line, char* stream ,int max_size)  
{  
		char *p;	
        int len = 0;  
		
		p=stream;
        while( *p != '\0' && len < max_size )
		{  
        	line[len++] = *p;  
			p++;
			
        	if('\n' == *p || '\r'==*p)  
            break;  
        }
	
		if(*p != '\0' && *p != '\n' && *p != '\r')
			return 0;
  
      
    	line[len] = '\0';  
    	return len;  
} 

void HC05_SendString(char* str)
{
	
		//BLTUsart_SendString(USART2,(uint8_t *)str);
	BLTUsart_SendString(USART2,str);

}


