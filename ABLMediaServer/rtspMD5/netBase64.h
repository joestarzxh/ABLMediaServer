/* ---------------------------------------------------------- 
�ļ����ƣ�BASE64_API.h 
 
���ߣ��ؽ��� 
 
MSN��splashcn@msn.com 
 
��ǰ�汾��V1.1 
 
��ʷ�汾�� 
    V1.1    2010��05��11�� 
            ����BASE64�����Bug�� 
 
    V1.0    2010��05��07�� 
            �����ʽ�汾�� 
 
���������� 
    BASE64����ͽ��� 
 
�ӿں����� 
    Base64_Encode 
    Base64_Decode 
 
˵���� 
    1.  �ο�openssl-1.0.0�� 
    2.  �Ľ��ӿڣ���ʹ����ӦTCHAR�ַ����� 
    3.  ����EVP_DecodeBlock��������ʱδȥ������ֽڵ�ȱ�ݡ� 
 ------------------------------------------------------------ */  
#pragma once  
  
#include <windows.h>  
  
#ifdef  __cplusplus  
extern "C" {  
#endif  
  
void encodetribyte(unsigned char * in, unsigned char * out, int len);
int decodetribyte(unsigned char * in, unsigned char * out);
int Base64Encode(unsigned char * b64, const unsigned char * input, ULONG stringlen = 0);
int Base64Decode(unsigned char * output, const unsigned char * b64, ULONG codelen = 0);

#ifdef  __cplusplus  
}  
#endif  
