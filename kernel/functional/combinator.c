/***************************************************************
 *
 * (C) 2011-13 Nicola Bonelli <nicola.bonelli@cnit.it>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * The full GNU General Public License is included in this distribution in
 * the file called "COPYING".
 *
 ****************************************************************/

#include <linux/kernel.h>
#include <linux/module.h>

#include <linux/pf_q-module.h>

static
bool or(expression_t *p1, expression_t *p2, struct sk_buff const *skb)
{
        return p1->ptr(skb, p1) || p2->ptr(skb, p2);
}


static
bool and(expression_t *p1, expression_t *p2, struct sk_buff const *skb)
{
        return p1->ptr(skb, p1) && p2->ptr(skb, p2);
}


static
bool xor(expression_t *p1, expression_t *p2, struct sk_buff const *skb)
{
        return p1->ptr(skb, p1) != p2->ptr(skb, p2);
}


struct pfq_combinator_fun_descr combinator_functions[] = {

        { "or",         or  },
        { "and",        and },
        { "xor",        xor },

        { NULL, NULL}};
