#define LCD_DATA_HI     (GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3 )
#define LCD_E           GPIO_Pin_6
#define LCD_RS          GPIO_Pin_7

#define         de1     100
#define         de2     30              // == 1us, should work downto 300ns!
#define         de3     0x550           // does not work for 480

void delay(long loops)       {
  while (loops-- > 0)   {}; 
}

void LCD_nib(char nib)  {
  GPIOD->ODR &= ~LCD_DATA_HI;           // clear data lines
  GPIOD->ODR |= nib;                    // set data lines
  delay(de1);   GPIOD->ODR |= LCD_E;    delay(de2);     GPIOD->ODR &= ~LCD_E;   // wiggle E
}

void LCD_char (char ch) {
  GPIOD->ODR |= LCD_RS;                 // use data register
  LCD_nib((ch & 0xf0) >> 4);            delay(de3);     // send MS nibble
  LCD_nib( ch & 0x0f);                  delay(de3);     // send LS nibble
}

void LCD_cmd (char ch) {
  GPIOD->ODR &= ~LCD_RS;                // use command register
  LCD_nib((ch & 0xf0) >> 4);            delay(de3);     // send MS nibble
  LCD_nib( ch & 0x0f);                  delay(de3);     // send LS nibble
}

void LCD_string(char* str, char pos) {
  LCD_cmd ((pos & 0x7f) + 0x80);        // set position on screen
  while (*str != '\0') {                // send all characters in a string
    LCD_char (*str++);
  };
}

void LCD_clear (void)   {
  LCD_cmd (0x01);                       // clear screen command
  delay(0x10000);                       // 1.6ms to clear the display
}

void Bin2String (int arg1, char *ch)     {
int i, j, s = 0;
  if (arg1 < 0) {s = 1; arg1 = -arg1;};
  for (i=0; i<27; i++)  {                       // 36us to convert bin -> BCD
    for (j=6; j>=0; j--) ch[j] <<= 1;
    for (j=6; j>0; j--) {if (ch[j] & 0x10) { ch[j-1]++; ch[j] &= 0x0f;};};
    arg1 <<= 1;
    if (arg1 & 0x08000000) ch[6]++; 
    if (i < 26)
      for (j=6; j>=0; j--) {
        if (ch[j] >= 0x05)   ch[j] += 3;
      };
  };
  for (j=6; j>=0; j--) ch[j+1] = ch[j] + '0';     ch[8] = '\0';
  ch[0] = ' '; if (s == 1) ch[0] = '-';
}

void LCD_uInt32 (int arg1, char pos, char LZB)   {
char ch[10] = {0,0,0,0,0,0,0,0,0,0};
int j;
  Bin2String(arg1, ch);
  for (j=1; j<=8; j++) ch[j-1] = ch[j];
  j = 0; if (LZB != 0) while (ch[j] == '0' && j < 6) ch[j++] = ' ';
  LCD_string (ch, pos);                         // 750us to print
}

void LCD_sInt32 (int arg1, char pos, char LZB)   {
char ch[10] = {0,0,0,0,0,0,0,0,0,0};
int j;
  Bin2String(arg1, ch);
  j = 1; if (LZB != 0) while (ch[j] == '0' && j < 6) ch[j++] = ' ';
  LCD_string (ch, pos);                         // 750us to print
}

void LCD_uInt16 (int arg1, char pos, char LZB)   {
char ch[10] = {0,0,0,0,0,0,0,0,0,0};
int j;
  Bin2String(arg1, ch);
  for (j=3; j<=8; j++) ch[j-3] = ch[j];
  j =0; if (LZB != 0) while (ch[j] == '0' && j < 4) ch[j++] = ' ';
  LCD_string (ch, pos);                         // 550us to print
}

void LCD_sInt16 (int arg1, char pos, char LZB)   {
char ch[10] = {0,0,0,0,0,0,0,0,0,0};
int j;
  Bin2String(arg1, ch);
  for (j=3; j<=8; j++) ch[j-2] = ch[j];
  j = 1; if (LZB != 0) while (ch[j] == '0' && j <= 4) ch[j++] = ' ';
  LCD_string (ch, pos);                         // 550us to print
}

void LCD_sInt3DG (int arg1, char pos, char LZB)   {
int j;
char ch[10] = {0,0,0,0,0,0,0,0,0,0};
  Bin2String(arg1, ch);
  for (j=5; j<=8; j++) ch[j-4] = ch[j];
  j = 1; if (LZB != 0) while (ch[j] == '0' && j <= 2) ch[j++] = ' ';
  LCD_string (ch, pos);                         // 
}

void LCD_init (void) {
  RCC->AHB1ENR |=  0x0008;      // Enable clock for GPIOD
  GPIOD->MODER |=  0x00005055;  // MODE Register: lower 4 bits and two more are outputs
  GPIOD->ODR   &= ~(0x00cf);    // Output Data Register: all zeros
  
  GPIOD->ODR &= ~LCD_RS;        // use command register
  LCD_nib(0x03);        delay(0x28000);
  LCD_nib(0x03);        delay(0x2000);
  LCD_nib(0x03);        delay(0x1000);
  LCD_nib(0x02);        delay(0x1000);
  LCD_cmd(0x28);
  LCD_cmd(0x0c);
  LCD_cmd(0x01);
  LCD_cmd(0x06);
  delay(0x10000);       
};
