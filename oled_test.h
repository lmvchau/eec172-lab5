/*
 * test.h
 *
 *  Created on: Jan 27, 2024
 *      Author: rtsang
 */

#ifndef OLED_OLED_TEST_H_
#define OLED_OLED_TEST_H_

// Color definitions
#define BLACK           0x0000
#define BLUE            0x001F
#define GREEN           0x07E0
#define CYAN            0x07FF
#define RED             0xF800
#define MAGENTA         0xF81F
#define YELLOW          0xFFE0
#define WHITE           0xFFFF


void testfastlines(unsigned int color1, unsigned int color2);
void testdrawrects(unsigned int color);
void testfillrects(unsigned int color1, unsigned int color2);
void testfillcircles(unsigned char radius, unsigned int color);
void testdrawcircles(unsigned char radius, unsigned int color);
void testtriangles();
void testroundrects();
void testlines(unsigned int color);
void lcdTestPattern(void);
void lcdTestPattern2(void);
void lcdPrintFontTable(void);
void lcdPrintHelloWorld(void);


#endif /* OLED_OLED_TEST_H_ */
