#include "LPC17xx.h"
#include "math.h"

#define LED_PIN_START 0
#define LED_PIN_END 7
#define BUTTON_PIN 10
#define TIMER_INTERVAL_MS 125
#define LONG_PRESS_THRESHOLD 16 // 16 * TIMER_INTERVAL_MS = 2 segundos

volatile uint32_t timerTicks = 0;
volatile uint32_t buttonPressedTicks = 0;
volatile uint32_t ledDirection = 1; // 1 para esquerda, -1 para direita 
volatile uint32_t button_pressed_flag = 0;

void SysTick_Handler(void) {
    // Verifica se o botão está pressionado
    if (!(LPC_GPIO2->FIOPIN & (1 << BUTTON_PIN))) {
      buttonPressedTicks = buttonPressedTicks + ledDirection;
				
			if(abs(buttonPressedTicks) >= LONG_PRESS_THRESHOLD)
					timerTicks = timerTicks + ledDirection;
				
			button_pressed_flag = 1;
    } else if (button_pressed_flag){
			ledDirection *= -1;
			button_pressed_flag = 0;
		}
}

void updateLEDs() {
    uint8_t currentLED = (timerTicks / (1000 / TIMER_INTERVAL_MS)) % 8;

    LPC_GPIO2->FIOCLR = 0xFF; // Apaga todas as LEDs
    LPC_GPIO2->FIOSET = (1 << currentLED); // Acende a LED correspondente
}

int main() {
    // Configura os pinos das LEDs como saída
    uint8_t i;
    for (i = LED_PIN_START; i <= LED_PIN_END; i++)
        LPC_GPIO2->FIODIR |= (1 << i);

    // Configura o pino do botão como entrada
    LPC_GPIO2->FIODIR &= ~(1 << BUTTON_PIN);
		
		 // Configuração do SysTick como timer
    SysTick_Config(SystemCoreClock / (1000 / TIMER_INTERVAL_MS));

    while (1)
        updateLEDs();

    return 0;
}