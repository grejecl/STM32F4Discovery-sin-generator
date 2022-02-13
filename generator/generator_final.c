#include "stm32f4xx.h"
#include "stm32f4xx_rcc.c"
#include "stm32f4xx_rng.c"
#include "stm32f4xx_dac.c"
#include "stm32f4xx_gpio.c"
#include "stm32f4xx_tim.c"
#include "stm32f4xx_exti.c"
#include "stm32f4xx_syscfg.c"
#include "dd.h"
#include "LCD2x16.c"
#include "math.h"

int Buffer[16], DDSTable[4096], ptrTable, ptrBuffer = 0, k = 262, Am = 150, AmNoise = 0, prs = 1.0;

//// initialize port E, Gpio_Pin_8 to GPIO_Pin_15 as outputs
void GPIOEinit (void)   {
GPIO_InitTypeDef  GPIO_InitStructure;
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, ENABLE);
  GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_8  | GPIO_Pin_9  | GPIO_Pin_10 | GPIO_Pin_11;
  GPIO_InitStructure.GPIO_Pin  |= GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
  GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
  GPIO_Init(GPIOE, &GPIO_InitStructure); 
}

void SWITCHinit (void)  {
GPIO_InitTypeDef        GPIO_InitStructure;
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, ENABLE);
  GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_3 | GPIO_Pin_4 | 
                                  GPIO_Pin_5 | GPIO_Pin_6;
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IN;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP; 
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
  GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_DOWN;
  GPIO_Init(GPIOE, &GPIO_InitStructure);  
}

// DAC init function
void DACinit (void)     {
DAC_InitTypeDef         DAC_InitStructure;
GPIO_InitTypeDef        GPIO_InitStructure;
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
  GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_4 | GPIO_Pin_5;
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AN;
  GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;  
  GPIO_Init(GPIOA, &GPIO_InitStructure);  
  
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_DAC, ENABLE);
  DAC_InitStructure.DAC_OutputBuffer                 = DAC_OutputBuffer_Enable;
  DAC_InitStructure.DAC_Trigger                      = DAC_Trigger_None;
  DAC_InitStructure.DAC_WaveGeneration               = DAC_WaveGeneration_None;
  DAC_Init(DAC_Channel_1, &DAC_InitStructure); 
  DAC_Init(DAC_Channel_2, &DAC_InitStructure);
  
  DAC_Cmd(DAC_Channel_1, ENABLE);   
  DAC_Cmd(DAC_Channel_2, ENABLE);
}

// RNG init function
void RNGinit(void)      {
  RCC_AHB2PeriphClockCmd(RCC_AHB2Periph_RNG, ENABLE);
  
  NVIC_EnableIRQ(HASH_RNG_IRQn);                // Enable RNG IRQ in NVIC
  RNG->CR = RNG_CR_IE | RNG_CR_RNGEN;           // Enable RNG IRQ or enable RNG
}

// Timer 5 init function - time base
void TIM5init_TimeBase_ReloadIRQ (int interval)  {
TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM5,  ENABLE);
  TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
  TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
  TIM_TimeBaseInitStructure.TIM_Period = interval;
  TIM_TimeBaseInitStructure.TIM_Prescaler = 0;
  TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0;
  TIM_TimeBaseInit(TIM5, &TIM_TimeBaseInitStructure);

  NVIC_EnableIRQ(TIM5_IRQn);                 // Enable IRQ for TIM5 in NVIC 
  TIM_ITConfig(TIM5, TIM_IT_Update, ENABLE); // Enable IRQ on update for Timer5

  TIM_Cmd(TIM5, ENABLE);
}   

//Initilialize Button B1 - PA0
void GPIOAinit (void)   {
GPIO_InitTypeDef  GPIO_InitStructure;
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
  GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_0;
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IN;
  GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
  GPIO_Init(GPIOA, &GPIO_InitStructure); 
}

// IRQ Button B1 - PA0 interupt config
void IRQinit_EXTI (void) {
EXTI_InitTypeDef        EXTI_InitStructure;

  RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);

  SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOA, EXTI_PinSource0);
  
  EXTI_InitStructure.EXTI_Line   |= EXTI_Line0;
  EXTI_InitStructure.EXTI_LineCmd = ENABLE;
  EXTI_InitStructure.EXTI_Mode    = EXTI_Mode_Interrupt;
  EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;  
  EXTI_Init(&EXTI_InitStructure);
  
  NVIC_EnableIRQ(EXTI0_IRQn);                   // Enable IRQ for ext. signals, line 0
  NVIC_SetPriority(EXTI0_IRQn, 51);           // Must have lower priority than DAC
}

int main ()  {    
  
  int sw, Fp;
  for (ptrTable = 0; ptrTable <= 4095; ptrTable++)
    DDSTable[ptrTable] = (int)(1850.0 * sin((float)ptrTable / 2048.0 * 3.14159265));
  
  GPIOAinit();
  GPIOEinit();
  SWITCHinit();
  LCD_init();
  LCD_string("Frq=", 0x00); LCD_string("Hz", 0x0e); LCD_string("Am,Ns=", 0x40);
  DACinit();  
  RNGinit();
  IRQinit_EXTI();
  TIM5init_TimeBase_ReloadIRQ(336);           // 336 == 4 us == 250 kHz

  while (1) {                                 // endless loop
    sw = GPIOE->IDR;
    
    if ((sw & S370) && (sw & S371) && (AmNoise > 0)) AmNoise--;    // Change noise amplitude
    if ((sw & S372) && (sw & S373) && (AmNoise < 255)) AmNoise++;  //
    if ((sw & S371) && (k  < 5243)) k++;       // Change frequency
    if ((sw & S370) && (k  >     2)) k--;       //
    if ((sw & S373) && (Am <   255)) Am++;      // Change signal amplitude
    if ((sw & S372) && (Am >     1)) Am--;      //
    
    Fp = (int)(250.0e3 * (float)k / (prs * 65535.9));     // Frequency on LCD
    LCD_uInt16(Fp, 0x09, 1); LCD_uInt16(Am, 0x46, 1); LCD_uInt16(AmNoise, 0x4b, 1);
    
//    for (j = 0; j<200; j++){};      // waste some time
  };
}

// IRQ function for Timer5
void TIM5_IRQHandler(void)       {
  GPIOE->BSRRL = BIT_8;             
  
  TIM_ClearITPendingBit(TIM5, TIM_IT_Update); // clear interrupt flag

  int noise = 0;
  for(int i = 0; i < 16; i++) {                         // Sum all values in Buffer for averaging   
    noise += (Buffer[i & 15]);                   
  }

  ptrTable = (ptrTable + k) & 0xffff;                   
  int Out1 = ((Am * DDSTable[  ptrTable >> 4]) / 256                    // Substract 32*2048 from noise and divide by 32
              + 2048 + ((AmNoise * ((noise - 32768) / 16)) / 256));     // to get Gaussian noise N(0, sigma)   
  if (Out1 > 4095) Out1 = 4095;
  if (Out1 < 0) Out1 = 0;
  
  DAC->DHR12R1 = Out1;
  DAC->DHR12R2 = (Am * DDSTable[((ptrTable >> 4) + 1024) & 4095]) / 256 + 2048;
  
  GPIOE->BSRRH = BIT_8;
}

// IRQ function for RNG
void HASH_RNG_IRQHandler(void)  {   
  Buffer[(ptrBuffer++ & 15)] = (RNG->DR & 0xfff);       // Increment pointer and store 12-bits of random number
}

// IRQ function for Button B1 - PA0
void EXTI0_IRQHandler (void)   {
  if (prs == 1){
    TIM5->PSC = 39;
    prs = 40;
  }
  else {
    TIM5->PSC = 0;
    prs = 1;
  }
  EXTI_ClearITPendingBit(EXTI_Line0);
  EXTI_ClearFlag(EXTI_Line0);
}
