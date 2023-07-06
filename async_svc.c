#include "stm32f407xx.h"

// HSI frequency
#define HSI_HZ ((uint32_t)16000000U)

// SysTick counter.
static volatile uint32_t TICK_COUNT = 0;

// Initialize the GPIO pins for LEDs.
void enable_leds(void) {
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIODEN;
    GPIOD->MODER &= ~(0xFFU << 24);
    GPIOD->MODER |=  (0x55  << 24);
}

// Switch green LED between on and off.
void toggle_green_led(void) {
    GPIOD->ODR ^= (1 << 12);
}

// Switch red LED between on and off.
void toggle_red_led(void) {
    GPIOD->ODR ^= (1 << 14);
}

// Set SVC priority higher than SysTick.
void set_svc_high_priority(void) {
    SCB->SHP[SVCall_IRQn + 16 - 4] = 32;
}

// Set SVC priority lower than SysTick.
void set_svc_low_priority(void) {
    SCB->SHP[SVCall_IRQn + 16 - 4] = 96;
}

// Set SysTick priority in the middle.
void set_systick_mid_priority(void) {
    SCB->SHP[SysTick_IRQn + 16 - 4] = 64;
}

// The SysTick exception handler. It raises the priority of SVC to above
// itself, increments the counter, and lowers the priority of SVC back to
// below itself.
void SysTick_Handler(void) {
    set_svc_high_priority();

    // *** SVC CAN UNEXPECTEDLY BE INVOKED RIGHT HERE! ***

    ++TICK_COUNT;
    set_svc_low_priority();
}

// The SVC handler. It turns on the red LED and spin if being invoked
// when the code was running using MSP. In this code example, the `run()`
// function will run with PSP, and exception handlers will run with MSP.
// If SVC handler sees itself being invoked when the code was already
// running with MSP, it means the SVC is nested above another exception
// handler, in which case we hang it. Otherwise, it simply returns and
// does nothing.
void __attribute__((naked)) SVC_Handler(void) {
    asm volatile (
        "mov r0, lr               \n"
        "b die_if_called_with_msp \n"
        :::
    );
}

// Determine if it is nested exception by examining the exception return
// pattern stored in `lr` register upon exception. Turn on red LED and spin
// if nested.
void die_if_called_with_msp(uint32_t lr) {
    if ((lr & 0x4) == 0) {
        toggle_red_led();
        while (1);
    }
}

// Initialize SysTick to fire at a given frequency in Hz.
void init_systick_in_hz(uint32_t hz) {
    SysTick_Config(HSI_HZ / hz);
}

// The entry function. It sets up the stacks and jumps to the `run()`
// function. MSP grows down from 0x2002_0000, while PSP grows down
// from 0x2001_0000. The `run()` functions will run with PSP, and the
// exception handlers will run with MSP.
int __attribute__((naked)) main(void) {
    asm volatile (
        // PSP grows down from 0x2001_0000
        "ldr r0, =#0x20010000 \n"
        "msr psp, r0          \n"
        // MSP grows down from 0x2002_0000
        "ldr r0, =#0x20020000 \n"
        "msr msp, r0          \n"
        // The `run()` functions will run with PSP
        "mrs r0, control      \n"
        "orr r0, #2           \n"
        "msr control, r0      \n"
        // Jump to `run()`
        "b   run              \n"
        :::
    );
}

int run(void) {
    // Let SysTick fire every 0.1ms.
    init_systick_in_hz(10000);

    // Let SVC have a lower priority than SysTick.
    set_svc_low_priority();
    set_systick_mid_priority();

    enable_leds();

    while(1) {
        // Wait for 1 second. Meanwhile, invoke SVC. Since we only invoke
        // SVC from here, running with PSP, the SVC handler should not spin,
        // but it will.
        uint32_t next_tick = TICK_COUNT + 10000;
        while (TICK_COUNT < next_tick)
            asm volatile ("svc #0");
        
        // If we are still running, toggle the green LED.
        toggle_green_led();
    }

    return 0;
}
