#include <RTC.h>


//Настройка тактирования часов
void RTC_init(void){

	// Включим LSI
	RCC->CSR |= RCC_CSR_LSION;
	// Ждем готовности LSI
	while ( !(RCC->CSR & RCC_CSR_LSIRDY) ){}

	// Включим тактирование модуля PWR
	RCC->APB1ENR |= RCC_APB1ENR_PWREN;
	// После сброса регистры RTC находятся в защищенном домене 
	// Для разрешения записи необходимо поставить флаг в PWR->CR
	PWR->CR |= PWR_CR_DBP;
	
  	// Выберем источник тактирования RTC( от низкоскоростного внутреннего источника LSI(40 kHz) )
	RCC->BDCR |= RCC_BDCR_RTCSEL_LSI;
	// Включим тактиование RTC
	RCC->BDCR |= RCC_BDCR_RTCEN;
	

	// Отключим защиту от записи
	RTC->WPR = 0xCA;
	RTC->WPR = 0x53;

	// Войдем в режим редактирования
	RTC->ISR |= RTC_ISR_INIT;
	// Ждем подтверждения входа в режим редактирования
	while( !(RTC->ISR & RTC_ISR_INITF) ){};	

	// Установим асинхронный предтелитель на 100(99+1).
	RTC->PRER = (uint32_t)(99 << 16);
	// Установим синхронный предтелитель на 400(399+1).
	RTC->PRER |= (uint32_t)399;
	
	// Выйти из режима редактирования
	RTC->ISR &= ~(RTC_ISR_INIT);
	
	// Включим защиту от записи
	RTC->WPR = 0xFF;
}


// Установка времени и даты часов реального времени. Все значения в BCD.
void RTC_change (RTC_struct *value){
	uint32_t TR, DR;
	
	// Отключим защиту от записи
	RTC->WPR = 0xCA;
	RTC->WPR = 0x53;

 	// Войдем в режим редактирования
	RTC->ISR |= RTC_ISR_INIT;
	
	// Ждем подтверждения входа в режим редактирования
	while( !(RTC->ISR & RTC_ISR_INITF) ){};	
	
	// Установим время
	TR = ( ((uint32_t)(value->hour) << 16) | ((uint32_t)(value->minute) << 8) | ((uint32_t)value->second) );
	RTC->TR = TR & RTC_TR_MASK;
	// Установим дату
	DR = (uint32_t)(value->year) << 16  | (uint32_t)(value->week) << 13 | (uint32_t)(value->mount) << 8 | (uint32_t)(value->date);
	RTC->DR = DR & RTC_DR_MASK;
	
 	// Выйти из режима редактирования
	RTC->ISR &= ~(RTC_ISR_INIT); 
	
	// Включим защиту от записи
	RTC->WPR = 0xFF;
	
	RTC->BKP0R |= RTC_SET; // Значит установили часы
}

void RTC_IRQHandler(void){
	if(RTC->ISR & RTC_ISR_ALRAF){ // Сработал Alarm
		
		GPIOA->ODR ^= GPIO_ODR_6;
		
		RTC->ISR &= ~(RTC_ISR_ALRAF);// Очичтим флаг прерывани ALARM
		EXTI->PR |= EXTI_PR_PR17; // Сбросим флаг прерывания линии EXTI
	}
}

void RTC_alarm(RTC_struct *value, uint8_t msk){
	uint32_t alr, ms;
	/* Разрешения прерывания */
	NVIC_EnableIRQ(RTC_IRQn);
	EXTI->IMR |= EXTI_IMR_MR17; // Разрешим прерывание 17 линиии EXTI
	EXTI->RTSR |= EXTI_RTSR_TR17; // Прерывание по восходящему фронту
	
	// Отключим защиту от записи
	RTC->WPR = 0xCA;
	RTC->WPR = 0x53;
	
	RTC->CR |= RTC_CR_ALRAIE; // Прерывание по alarm. Доступ только в защищенной зоне.
	
	// Отключить ALARM
	RTC->CR &= ~(RTC_CR_ALRAE); 
	// Ждем подтверждения входа в режим редактирования
	while( !(RTC->ISR & RTC_ISR_ALRAWF) ){};
	
	RTC-> ALRMAR = ( (msk & 0x01) << 7 ); // MSK1
	RTC-> ALRMAR |= ( (msk & 0x02) << 14 ); // MSK2
	RTC-> ALRMAR |= ( (msk & 0x04) << 21 ); // MSK3
	RTC-> ALRMAR |= ( (msk & 0x08) << 28 ); // MSK4

	
	alr = ( (uint32_t)(value->date) << 24) | ((uint32_t)(value->hour) << 16) | ((uint32_t)(value->minute) << 8) | ((uint32_t)(value->second)); 
	// установили время срабатывания
	RTC->ALRMAR |= alr;
	
	// Включить ALARM
	RTC->CR |= RTC_CR_ALRAE;
	// Включим защиту от записи
	RTC->WPR = 0xFF;
}

// Включение тактирования и первоначальная установка часов
void RTC_sets(RTC_struct *value){
		uint32_t TR, DR;
		// Включим LSI
		RCC->CSR |= RCC_CSR_LSION;
		// Ждем готовности LSI
		while ( !(RCC->CSR & RCC_CSR_LSIRDY) ){}
		
		// Включим тактирование модуля PWR
		RCC->APB1ENR |= RCC_APB1ENR_PWREN;
		// После сброса регистры RTC находятся в защищенном домене 
		// Для разрешения записи необходимо поставить флаг в PWR->CR
		PWR->CR |= PWR_CR_DBP;
		
	if( !(RTC->ISR & RTC_ISR_INITS) ){ //Часы не настроены	

		// Выберем источник тактирования RTC( от низкоскоростного внутреннего источника LSI(40 kHz) )
		RCC->BDCR |= RCC_BDCR_RTCSEL_LSI;
		// Включим тактиование RTC
		RCC->BDCR |= RCC_BDCR_RTCEN;
	
		// Отключим защиту от записи
		RTC->WPR = 0xCA;
		RTC->WPR = 0x53;

		// Войдем в режим редактирования
		RTC->ISR |= RTC_ISR_INIT;
		// Ждем подтверждения входа в режим редактирования
		while( !(RTC->ISR & RTC_ISR_INITF) ){};	
		
		// Установим асинхронный предтелитель на 100(99+1).
		RTC->PRER = (uint32_t)(99 << 16);
		// Установим синхронный предтелитель на 400(399+1).
		RTC->PRER |= (uint32_t)399;
		
		// Установим время
		TR = ( ((uint32_t)(value->hour) << 16) | ((uint32_t)(value->minute) << 8) | ((uint32_t)value->second) );
		RTC->TR = TR & RTC_TR_MASK;
		// Установим дату
		DR = (uint32_t)(value->year) << 16  | (uint32_t)(value->week) << 13 | (uint32_t)(value->mount) << 8 | (uint32_t)(value->date);
		RTC->DR = DR & RTC_DR_MASK;
		
		// Выйти из режима редактирования
		RTC->ISR &= ~(RTC_ISR_INIT); 

		
		// Включим защиту от записи
		RTC->WPR = 0xFF;
		
	}

}


// Считывание значений часов реального времени. Все значения в BCD.
void RTC_get(RTC_struct * value){
	uint32_t TR, DR;
	TR = RTC->TR;
	DR = RTC->DR;
	
	// Отдадим время
	value->hour = (uint8_t)(TR >> 16) & (uint8_t)0x3F; // Возьмем только шесть бит
	value->minute = (uint8_t)(TR >> 8) & (uint8_t)0x7F; // Возьмем семь бит
	value->second = (uint8_t)(TR) & (uint8_t)0x7F;
	// Отдадим дату
	value->year = (uint8_t)(DR >> 16);
	value->week = (uint8_t)(DR >> 13) & (uint8_t)0x7;// Три бита
	value->mount = (uint8_t)(DR >> 8) & (uint8_t)0x1F; // Пять бит
	value->date = (uint8_t)(DR) & (uint8_t)0x3F; // Шесть бит
	
}

// Перевод из двоичного в BCD(двузначное).
uint8_t RTC_ByteToBcd2(uint8_t Value){
	uint8_t bcdhigh = 0;
	while (Value >= 10)	{
		bcdhigh++;
		Value -= 10;
	}
	return  ((uint8_t)(bcdhigh << 4) | Value);
}

// Перевод из BCD(двузначное) в двоичную форму
uint8_t RTC_Bcd2ToByte(uint8_t Value){
	uint8_t tmp = 0;
	tmp = ((uint8_t)(Value & (uint8_t)0xF0) >> (uint8_t)0x4) * 10;
	return (tmp + (Value & (uint8_t)0x0F));
}

// Старший разряд двоичного BCD 
uint8_t RTC_Bcd_elder(uint8_t value){
	return (value >> 4);
}

// Младший разряд двоичного BCD
uint8_t RTC_Bcd_under(uint8_t value){
	return (value & (uint8_t)0x0F);
}