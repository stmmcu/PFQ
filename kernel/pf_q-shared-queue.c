/***************************************************************
 *
 * (C) 2011-15 Nicola Bonelli <nicola@pfq.io>
 *             Andrea Di Pietro <andrea.dipietro@for.unipi.it>
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
#include <linux/vmalloc.h>
#include <linux/printk.h>
#include <linux/kthread.h>
#include <linux/mm.h>
#include <linux/pf_q.h>

#include <pragma/diagnostic_pop>

#include <pf_q-memory.h>
#include <pf_q-shared-queue.h>
#include <pf_q-shmem.h>


int
pfq_shared_queue_enable(struct pfq_sock *so, unsigned long user_addr)
{
	if (!so->shmem.addr) {

		struct pfq_shared_queue * mapped_queue;
		size_t n;
                int i;

		/* alloc queue memory */

		if (user_addr) {
			if (pfq_hugepage_map(&so->shmem, user_addr, pfq_shared_memory_size(so)) < 0)
				return -ENOMEM;
		}
		else {
			if (pfq_shared_memory_alloc(&so->shmem, pfq_shared_memory_size(so)) < 0)
				return -ENOMEM;
		}

		/* initialize queues headers */

		mapped_queue = (struct pfq_shared_queue *)so->shmem.addr;

		/* initialize Rx queue */

		mapped_queue->rx.data      = 0;
		mapped_queue->rx.len       = so->opt.rx_queue_len;
		mapped_queue->rx.size      = pfq_mpsc_queue_mem(so)/2;
		mapped_queue->rx.slot_size = so->opt.rx_slot_size;

		so->opt.rxq.base_addr = so->shmem.addr + sizeof(struct pfq_shared_queue);

		/* reset Rx slots */

		for(i = 0; i < 2; i++)
		{
			char * raw = so->shmem.addr + sizeof(struct pfq_shared_queue) + i * mapped_queue->rx.size;
			char * end = raw + mapped_queue->rx.size;
			const int rst = !i;
			for(;raw < end; raw += mapped_queue->rx.slot_size)
				((struct pfq_pkthdr *)raw)->commit = rst;
		}

		/* initialize TX queues */

		mapped_queue->tx.size  = pfq_spsc_queue_mem(so)/2;

		mapped_queue->tx.prod.index = 0;
		mapped_queue->tx.prod.off0  = 0;
		mapped_queue->tx.prod.off1  = 0;
		mapped_queue->tx.cons.index = 0;
		mapped_queue->tx.cons.off   = 0;

		so->opt.txq.base_addr = so->shmem.addr + sizeof(struct pfq_shared_queue) + pfq_mpsc_queue_mem(so);


		/* initialize TX async queues */

		for(n = 0; n < Q_MAX_TX_QUEUES; n++)
		{
			mapped_queue->tx_async[n].size  = pfq_spsc_queue_mem(so)/2;

			mapped_queue->tx_async[n].prod.index = 0;
			mapped_queue->tx_async[n].prod.off0  = 0;
			mapped_queue->tx_async[n].prod.off1  = 0;
			mapped_queue->tx_async[n].cons.index = 0;
			mapped_queue->tx_async[n].cons.off   = 0;

			so->opt.txq_async[n].base_addr = so->shmem.addr + sizeof(struct pfq_shared_queue)
				+ pfq_mpsc_queue_mem(so)
				+ pfq_spsc_queue_mem(so) * (1 + n);
		}

		/* commit queues */

		smp_wmb();

		atomic_long_set(&so->opt.rxq.addr, (long)&mapped_queue->rx);
		atomic_long_set(&so->opt.txq.addr, (long)&mapped_queue->tx);

		for(n = 0; n < Q_MAX_TX_QUEUES; n++)
		{
			atomic_long_set(&so->opt.txq_async[n].addr, (long)&mapped_queue->tx_async[n]);
		}

		pr_devel("[PFQ|%d] Rx queue: len=%zu slot_size=%zu caplen=%zu, mem=%zu bytes\n",
			 so->id,
			 so->opt.rx_queue_len,
			 so->opt.rx_slot_size,
			 so->opt.caplen,
			 pfq_mpsc_queue_mem(so));

		pr_devel("[PFQ|%d] Tx queue: len=%zu slot_size=%zu maxlen=%d, mem=%zu bytes\n",
			 so->id,
			 so->opt.tx_queue_len,
			 so->opt.tx_slot_size,
			 xmit_slot_size,
			 pfq_spsc_queue_mem(so));

		pr_devel("[PFQ|%d] Tx async queues: len=%zu slot_size=%zu maxlen=%d, mem=%zu bytes (%d queues)\n",
			 so->id,
			 so->opt.tx_queue_len,
			 so->opt.tx_slot_size,
			 xmit_slot_size,
			 pfq_spsc_queue_mem(so) * Q_MAX_TX_QUEUES, Q_MAX_TX_QUEUES);
	}

	return 0;
}


int
pfq_shared_queue_disable(struct pfq_sock *so)
{
	size_t n;

	if (so->shmem.addr) {

		atomic_long_set(&so->opt.rxq.addr, 0);

		for(n = 0; n < Q_MAX_TX_QUEUES; n++)
		{
			atomic_long_set(&so->opt.txq_async[n].addr, 0);
		}

		msleep(Q_GRACE_PERIOD);

		pfq_shared_memory_free(&so->shmem);

		so->shmem.addr = NULL;
		so->shmem.size = 0;

		pr_devel("[PFQ|%d] Tx/Rx queues disabled.\n", so->id);
	}

	return 0;
}
