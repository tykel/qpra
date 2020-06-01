#include <stdlib.h>
#include <string.h>

#include "core/cart/cart.h"
#include "log.h"

int core_cart_init(struct core_cart **pcart, struct core_cpu *cpu)
{
   struct core_cart *cart = NULL;

   cart = malloc(sizeof(*cart));
   if (cart == NULL) {
       LOGE("Could not allocate cart core; exiting");
       return 0;
   }
   *pcart = cart;

   cart->mem = malloc(256);

   return 1;
}

int core_cart_load(struct core_cart *cart, uint8_t *mem)
{
   memcpy(cart->mem, mem, 256);
}

int core_cart_destroy(struct core_cart *cart)
{
   free(cart->mem);
   free(cart);
   return 1;
}

#ifdef _DEBUG
#ifndef _DEBUG_MEMORY
#define _DEBUG_MEMORY
#endif
#endif

uint8_t core_cart_readb(struct core_cart *cart, uint16_t a)
{
#ifdef _DEBUG_MEMORY
    LOGD("core.cart: read byte @ $%04x", a);
#endif
    return cart->mem[a - 0xfe00];
}

void core_cart_writeb(struct core_cart *cart, uint16_t a, uint8_t v)
{
#ifdef _DEBUG_MEMORY
    LOGW("core.cart: wrote %02x @ $%04x", v, a);
#endif
    cart->mem[a - 0xfe00] = v;
}

uint16_t core_cart_readw(struct core_cart *cart, uint16_t a)
{
    uint8_t hi, lo;
    hi = core_cart_readb(cart, a);
    lo = core_cart_readb(cart, a+1);
    return hi << 8 | lo;
}

void core_cart_writew(struct core_cart *cart, uint16_t a, uint16_t v)
{
    core_cart_writeb(cart, a, v & 0xff);
    core_cart_writeb(cart, a+1, v >> 8);
}

