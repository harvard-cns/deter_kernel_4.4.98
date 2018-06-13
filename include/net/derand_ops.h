#ifndef _NET_DERAND_OPS_H
#define _NET_DERAND_OPS_H

#include <net/derand.h>
#include <net/sock.h>
#include <linux/socket.h>
#include <linux/jiffies.h>

#if DERAND_ENABLE

struct derand_record_ops{
	/* Create a recorder at server side */
	void (*server_recorder_create)(struct sock*);

	/* Create a recorder at client side */
	void (*client_recorder_create)(struct sock*);

	/* Destruct a recorder */
	void (*recorder_destruct)(struct sock*);

	/* A new tcp_sendmsg sockcall. Return the sockcall ID */
	u32 (*new_sendmsg)(struct sock *sk, struct msghdr *msg, size_t size);

	/* A new tcp_sendpage sockcall. Return the sockcall ID */
	u32 (*new_sendpage)(struct sock *sk, int offset, size_t size, int flags);

	/* A new tcp_recvmsg sockcall. Return the sockcall ID */
	u32 (*new_recvmsg)(struct sock *sk, struct msghdr *msg, size_t len, int nonblock, int flags, int *addr_len);

	/* A new event that a sockcall acquire a spinlock. sc_id: sockcall ID */
	void (*sockcall_lock)(struct sock *sk, u32 sc_id);

	/* A new packet */
	void (*incoming_pkt)(struct sock *sk);

	/* A write timer */
	void (*write_timer)(struct sock *sk);

	/* A delayed ack timer */
	void (*delack_timer)(struct sock* sk);

	/* A keepalive timer */
	void (*keepalive_timer)(struct sock *sk);

	/* A tasklet */
	void (*tasklet)(struct sock *sk);

	/* A read to jiffies */
	void (*read_jiffies)(const struct sock *sk, unsigned long v, int id);

	/* A read to tcp_time_stamp */
	void (*read_tcp_time_stamp)(const struct sock *sk, u32 v, int id);

	/* A call to tcp_under_memory_pressure */
	void (*tcp_under_memory_pressure)(const struct sock *sk, bool ret);

	/* A call to sk_under_memory_pressure */
	void (*sk_under_memory_pressure)(const struct sock *sk, bool ret);

	/* A call to sk_memory_allocated */
	void (*sk_memory_allocated)(const struct sock *sk, long ret);

	/* A call to sk_sockets_allocated_read_positive */
	void (*sk_sockets_allocated_read_positive)(struct sock *sk, int ret);

	/* A call to skb_mstamp_get */
	void (*skb_mstamp_get)(struct sock *sk, struct skb_mstamp *cl, int loc);

	/* TODO: We left out orphan socket related things */
};

extern struct derand_record_ops derand_record_ops_default;
extern struct derand_record_ops derand_record_ops;

/* A read to jiffies. ID is diff for each location of read in the code */
static inline unsigned long derand_jiffies(const struct sock *sk, int id){
	unsigned long res;
	// TODO check if in replay mode

	// not in replay mode
	res = jiffies;
	if (sk->recorder && derand_record_ops.read_jiffies)
		derand_record_ops.read_jiffies(sk, res, id);
	return res;
}

/* A read to tcp_time_stamp. ID is diff for each location of read in the code */
static inline u32 derand_tcp_time_stamp(const struct sock *sk, int id){
	u32 res;
	// TODO check if in replay mode
	
	// not in replay mode
	res = (u32)(jiffies);
	if (sk->recorder && derand_record_ops.read_tcp_time_stamp)
		derand_record_ops.read_tcp_time_stamp(sk, res, id);
	return res;
}

extern int tcp_memory_pressure;

/* Copy of tcp_under_memory_pressure */
static inline bool derand_copied_tcp_under_memory_pressure(const struct sock *sk)
{
	if (mem_cgroup_sockets_enabled && sk->sk_cgrp)
		return !!sk->sk_cgrp->memory_pressure;

	return tcp_memory_pressure;
}

/* A read to tcp_under_memory_pressure */
static inline bool derand_tcp_under_memory_pressure(const struct sock *sk){
	bool ret;
	// TODO check if in replay mode
	
	// not in replay mode
	ret = derand_copied_tcp_under_memory_pressure(sk);

	// if in record mode
	if (sk->recorder && derand_record_ops.tcp_under_memory_pressure)
		derand_record_ops.tcp_under_memory_pressure(sk, ret);
	return ret;
}

/* a read to sk_under_memory_pressure */
static inline bool derand_sk_under_memory_pressure(const struct sock *sk){
	bool ret;
	// TODO check if in replay mode
	
	// not in replay mode
	ret = sk_under_memory_pressure(sk);

	// if in record mode
	if (sk->recorder && derand_record_ops.sk_under_memory_pressure)
		derand_record_ops.sk_under_memory_pressure(sk, ret);
	return ret;
}

/* A call to sk_memory_allocated */
static inline long derand_sk_memory_allocated(const struct sock *sk){
	long ret;
	// TODO check if in replay mode

	// not in replay mode
	ret = sk_memory_allocated(sk);

	// if in record mode
	if (sk->recorder && derand_record_ops.sk_memory_allocated)
		derand_record_ops.sk_memory_allocated(sk, ret);
	return ret;
}

/* A call to sk_sockets_allocated_read_positive */
static inline int derand_sk_sockets_allocated_read_positive(struct sock *sk){
	int ret;
	// TODO check if in replay mode

	// not in replay mode
	ret = sk_sockets_allocated_read_positive(sk);

	// if in record mode
	if (sk->recorder && derand_record_ops.sk_sockets_allocated_read_positive)
		derand_record_ops.sk_sockets_allocated_read_positive(sk, ret);
	return ret;
}

/* A call to skb_mstamp_get */
static inline void derand_skb_mstamp_get(struct sock *sk, struct skb_mstamp *cl, int loc){
	// TODO check if in replay mode

	// not in replay mode
	skb_mstamp_get(cl);

	// if in record mode
	if (sk->recorder && derand_record_ops.skb_mstamp_get)
		derand_record_ops.skb_mstamp_get(sk, cl, loc);
}

#endif /* DERAND_ENABLE */

#endif /* _NET_DERAND_OPS_H */
