#ifndef _bmp_H
#define _bmp_H
#include <stdio.h>

#pragma pack(1)
typedef struct 
{
	uint16_t bfType;        //�ļ����ͣ�BMP��ʽΪ�ַ���BM
	uint32_t bfSize;		//ͼƬ��С����λΪKB
	uint16_t bfReserved1;	//����λ
	uint16_t bfReserved2;	//����λ
	uint32_t bfOffBits;  	//���ļ�ͷ��ʵ��ͼ������֮����ֽ�ƫ����
} BMP_FileHeaderTypeDef;

typedef struct 
{
	uint32_t bitSize;		 //BMP_InfoHeaderTypeDef�ṹ������Ҫ���ֽ���
	uint32_t biWidth;		 //ͼƬ���ȣ�����λ��λ
	int32_t  biHeight;		 //ͼƬ�߶ȣ�����Ϊ��λ����Ϊ��������Ϊ����
	uint16_t biPlanes;		 //��ɫƽ��������Ϊ1
	uint16_t biBitCount;	 //������/���ء���ֵΪ��1��4��8��16��24��32
	uint32_t biCompression;  //����ѹ������
	uint32_t biSizeImage;	 //ͼ���С
	uint32_t biXPelsPerMeter;//ˮƽ�ֱ���
	uint32_t biYPelsPerMeter;//��ֱ�ֱ���
	uint32_t biClrUsed;		 //��ɫ������
	uint32_t biClrImportant; //��Ҫ��ɫ������
		
}BMP_InfoHeaderTypeDef;

#pragma pack()
typedef struct
{
	BMP_FileHeaderTypeDef fileHeader;
	BMP_InfoHeaderTypeDef infoHeader;
		
}BMP_HeaderTypeDef;
//#pragma pack();

void BMP_ReadHeader(uint8_t *header, BMP_HeaderTypeDef *bmp);
BMP_HeaderTypeDef BMP_Picture(uint8_t *dir , uint8_t * buf ,uint32_t len);



#endif