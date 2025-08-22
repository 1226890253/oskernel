//
// Created by ljw on 25-8-10.
//

#ifndef APIC_H
#define APIC_H
#include "types.h"
void print_CPUID();
bool check_APIC_Timer_status();
bool check_APIC_TSC();

void apic_eoi();
void apic_singleCore_timer_init();
void apic_timer_isr_c();
void apic_error_isr_c();
#endif //APIC_H
