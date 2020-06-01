/*
 * core/cart/cart.h -- Emulator cart functions (header).
 *
 * Defines the structures used to represent the game cart.
 * Declares all the cart-related functions.
 *
 */

#ifndef QPRA_CART_H
#define QPRA_CART_H

#include <stdint.h>

struct core_cpu;
struct core_mmu;

struct core_cart {
   struct core_cpu *cpu;
   struct core_mmu *mmu;

   /* Array backing cart memory. */
   uint8_t *mem;

   /* Pointer to cart persistent memory. */
   uint8_t (*persistent)[256];
};

int core_cart_init(struct core_cart **pcart, struct core_cpu *cpu);
int core_cart_load(struct core_cart *cart, uint8_t *mem);
int core_cart_destroy(struct core_cart *cart);

uint8_t core_cart_readb(struct core_cart *cart, uint16_t a);
void core_cart_writeb(struct core_cart *cart, uint16_t a, uint8_t v);
uint16_t core_cart_readw(struct core_cart *cart, uint16_t a);
void core_cart_writew(struct core_cart *cart, uint16_t a, uint16_t v);

#endif
