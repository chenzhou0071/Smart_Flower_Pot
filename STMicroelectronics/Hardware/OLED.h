#ifndef __OLED_H
#define __OLED_H

void OLED_Init(void);
void OLED_Clear(void);
void OLED_ShowChar(uint8_t Line, uint8_t Column, char Char);
void OLED_ShowString(uint8_t Line, uint8_t Column, char *String);
void OLED_ShowNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);
void OLED_ShowSignedNum(uint8_t Line, uint8_t Column, int32_t Number, uint8_t Length);
void OLED_ShowHexNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);
void OLED_ShowBinNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);

/* 表情类型枚举 */
typedef enum {
	EMOJI_SMILE = 0,	/* 笑脸 */
	EMOJI_CRY			/* 哭脸 */
} EmojiType;

/* 表情显示函数 */
void OLED_ShowEmoji(uint8_t Row, uint8_t Col, EmojiType type);

#endif
