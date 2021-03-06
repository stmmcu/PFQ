/***************************************************************
 *
 * (C) 2011-15 Nicola Bonelli <nicola@pfq.io>
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

#include <pragma/diagnostic_push>

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/crc16.h>

#include <pragma/diagnostic_pop>

#include <lang/module.h>
#include <lang/headers.h>
#include <lang/misc.h>


#include <pf_q-sparse.h>


static ActionSkBuff
dummy(arguments_t args, SkBuff skb)
{
        const int data = GET_ARG(int,args);

	SkBuff nskb;

	printk(KERN_INFO "[PFQ/lang] dummy = %d\n", data);

        nskb = pfq_lang_copy_buff(skb);

	if (nskb == NULL) {
                printk(KERN_INFO "[PFQ/lang] clone error!!!\n");
                return Drop(skb);
	}

        printk(KERN_INFO "[PFQ/lang] packet cloned: %p -> %p\n", nskb, skb);

        return Pass(nskb);
}


static ActionSkBuff
dummy_vector(arguments_t args, SkBuff skb)
{
        const int *data = GET_ARRAY(int,args);
	size_t n, len = LEN_ARRAY(args);

	printk(KERN_INFO "[PFQ/lang] dummy: vector len: %zu...\n", len);

	for(n = 0; n < len; n++)
	{
		printk(KERN_INFO "[PFQ/lang]  data[%zu] = %d\n", n, data[n]);
	}

        return Pass(skb);
}


static ActionSkBuff
dummy_string(arguments_t args, SkBuff skb)
{
        const char *data = GET_ARG(const char *,args);

	printk(KERN_INFO "[PFQ/lang] dummy: vector string: %s\n", data);

        return Pass(skb);
}


static ActionSkBuff
dummy_strings(arguments_t args, SkBuff skb)
{
        const char **data = GET_ARRAY(const char *,args);
	size_t n, len = LEN_ARRAY(args);

	printk(KERN_INFO "[PFQ/lang] dummy: vector strings len: %zu...\n", len);

	for(n = 0; n < len; n++)
	{
		printk(KERN_INFO "[PFQ/lang] string[%zu]: %s\n", n, data[n]);
	}

        return Pass(skb);
}


static int
dummy_init(arguments_t args)
{
	printk(KERN_INFO "[PFQ/lang] %s :)\n", __PRETTY_FUNCTION__);
	return 0;
}

static int
dummy_fini(arguments_t args)
{
	printk(KERN_INFO "[PFQ/lang] %s :(\n", __PRETTY_FUNCTION__);
	return 0;
}



struct pfq_lang_function_descr dummy_functions[] = {

        { "dummy",         "CInt   -> SkBuff -> Action SkBuff",	  dummy, dummy_init,	     dummy_fini },
        { "dummy_vector",  "[CInt] -> SkBuff -> Action SkBuff",	  dummy_vector, dummy_init,  dummy_fini },
        { "dummy_string",  "String -> SkBuff -> Action SkBuff",	  dummy_string, dummy_init,  dummy_fini },
        { "dummy_strings", "[String] -> SkBuff -> Action SkBuff", dummy_strings, dummy_init, dummy_fini },

        { NULL }};

