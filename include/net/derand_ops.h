#ifndef _NET_DERAND_OPS_H
#define _NET_DERAND_OPS_H

#include <net/derand.h>
#include <net/sock.h>
#include <linux/socket.h>
#include <linux/jiffies.h>

#if DERAND_ENABLE

struct derand_record_ops{
	/* Create a recorder at server side */
	void (*server_recorder_create)(struct sock*, struct sk_buff *skb);

	/* Create a recorder at client side */
	void (*client_recorder_create)(struct sock*, struct sk_buff *skb);

	/* Destruct a recorder */
	void (*recorder_destruct)(struct sock*);

	/* A new tcp_sendmsg sockcall. Return the sockcall ID */
	u32 (*new_sendmsg)(struct sock *sk, struct msghdr *msg, size_t size);

	/* A new tcp_sendpage sockcall. Return the sockcall ID */
	u32 (*new_sendpage)(struct sock *sk, int offset, size_t size, int flags);

	/* A new tcp_recvmsg sockcall. Return the sockcall ID */
	u32 (*new_recvmsg)(struct sock *sk, struct msghdr *msg, size_t len, int nonblock, int flags, int *addr_len);

	/* A new tcp_splice_read sockcall. Return sockcall ID */
	u32 (*new_splice_read)(struct sock *sk, size_t len, unsigned int flags);

	/* A new tcp_close sockcall. Return the sockcall ID */
	u32 (*new_close)(struct sock *sk, long timeout);

	/* A new event that a sockcall acquire a spinlock. sc_id: sockcall ID */
	void (*sockcall_lock)(struct sock *sk, u32 sc_id);

	/* For replay: hook before acquiring lock */
	void (*sockcall_before_lock)(struct sock *sk, u32 sc_id);

	/* A new packet */
	void (*incoming_pkt)(struct sock *sk);

	/* For replay: hook before acquiring lock for incoming_pkt */
	void (*incoming_pkt_before_lock)(struct sock *sk);

	/* A write timer */
	void (*write_timer)(struct sock *sk);

	/* For replay: hook before acquiring lock for write_timer */
	void (*write_timer_before_lock)(struct sock *sk);

	/* A delayed ack timer */
	void (*delack_timer)(struct sock* sk);

	/* For replay: hook before acquiring lock for delack_timer */
	void (*delack_timer_before_lock)(struct sock *sk);

	/* A keepalive timer */
	void (*keepalive_timer)(struct sock *sk);

	/* For replay: hook before acquiring lock for keepalive_timer */
	void (*keepalive_timer_before_lock)(struct sock *sk);

	/* A tasklet */
	void (*tasklet)(struct sock *sk);

	/* For replay: hook before acquiring lock for tasklet*/
	void (*tasklet_before_lock)(struct sock *sk);

	/* For monitoring network actions on incoming packets */
	void (*mon_net_action)(struct sock *sk, struct sk_buff *skb);

	/* For recording the seq of the fin (i.e., last data seq + 1) */
	void (*record_fin_seq)(struct sock *sk);

	/* For deciding whether the listening socket is to replay or not */
	int (*to_replay_server)(struct sock *sk);

	/***********************
	 * shared values
	 **********************/
	/* A read to jiffies */
	void (*read_jiffies)(const struct sock *sk, unsigned long v, int id);

	unsigned long (*replay_jiffies)(const struct sock *sk, int id);

	/* A read to tcp_time_stamp */
	void (*read_tcp_time_stamp)(const struct sock *sk, unsigned long v, int id);

	unsigned long (*replay_tcp_time_stamp)(const struct sock *sk, int id);

	/* A call to tcp_under_memory_pressure */
	void (*tcp_under_memory_pressure)(const struct sock *sk, bool ret);

	bool (*replay_tcp_under_memory_pressure)(const struct sock *sk);

	/* A call to sk_under_memory_pressure */
	void (*sk_under_memory_pressure)(const struct sock *sk, bool ret);

	bool (*replay_sk_under_memory_pressure)(const struct sock *sk);

	/* A call to sk_memory_allocated */
	void (*sk_memory_allocated)(const struct sock *sk, long ret);

	long (*replay_sk_memory_allocated)(const struct sock *sk);

	/* A call to sk_sockets_allocated_read_positive */
	void (*sk_sockets_allocated_read_positive)(struct sock *sk, int ret);

	int (*replay_sk_socket_allocated_read_positive)(struct sock *sk);

	/* A call to skb_mstamp_get */
	void (*skb_mstamp_get)(struct sock *sk, struct skb_mstamp *cl, int loc);
	// we do not need special replay function for mstamp, because the above one can return value to cl

	/* For a call to skb_still_in_host_queue */
	void (*record_skb_still_in_host_queue)(const struct sock *sk, bool ret);
	bool (*replay_skb_still_in_host_queue)(const struct sock *sk, const struct sk_buff *skb); // we need skb, because the replay should wait for skb_fclone_busy(sk, skb) to be false (if false)

	/* Following are for debug */
	/* A general event */
	void (*general_event)(const struct sock *sk, int loc, u64 data);

	/* TODO: We left out orphan socket related things */

	/******************************************
	 * log
	 *****************************************/
	int (*log)(const struct sock *sk, const char *fmt, ...);
};

extern struct derand_record_ops derand_record_ops_default;
extern struct derand_record_ops derand_record_ops;

/* A read to jiffies. ID is diff for each location of read in the code */
static inline unsigned long derand_jiffies(const struct sock *sk, int id){
	unsigned long res;
	// check if in replay mode
	if (sk->replayer && derand_record_ops.replay_jiffies)
		return derand_record_ops.replay_jiffies(sk, id);

	// not in replay mode
	res = jiffies;
	if (sk->recorder && derand_record_ops.read_jiffies)
		derand_record_ops.read_jiffies(sk, res, id);
	return res;
}

/* A read to tcp_time_stamp. ID is diff for each location of read in the code */
static inline u32 derand_tcp_time_stamp(const struct sock *sk, int id){
	unsigned long res;
	// check if in replay mode
	if (sk->replayer && derand_record_ops.replay_tcp_time_stamp)
		return (u32)derand_record_ops.replay_tcp_time_stamp(sk, id);
	
	// not in replay mode
	res = jiffies;
	if (sk->recorder && derand_record_ops.read_tcp_time_stamp)
		derand_record_ops.read_tcp_time_stamp(sk, res, id);
	return (u32)res;
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
	// check if in replay mode
	if (sk->replayer && derand_record_ops.replay_tcp_under_memory_pressure)
		return derand_record_ops.replay_tcp_under_memory_pressure(sk);
	
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
	// check if in replay mode
	if (sk->replayer && derand_record_ops.replay_sk_under_memory_pressure)
		return derand_record_ops.replay_sk_under_memory_pressure(sk);
	
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
	// check if in replay mode
	if (sk->replayer && derand_record_ops.replay_sk_memory_allocated)
		return derand_record_ops.replay_sk_memory_allocated(sk);

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
	// check if in replay mode
	if (sk->replayer && derand_record_ops.replay_sk_socket_allocated_read_positive)
		return derand_record_ops.replay_sk_socket_allocated_read_positive(sk);

	// not in replay mode
	ret = sk_sockets_allocated_read_positive(sk);

	// if in record mode
	if (sk->recorder && derand_record_ops.sk_sockets_allocated_read_positive)
		derand_record_ops.sk_sockets_allocated_read_positive(sk, ret);
	return ret;
}

/* A call to skb_mstamp_get */
static inline void derand_skb_mstamp_get(struct sock *sk, struct skb_mstamp *cl, int loc){
	// check if in replay mode
	if (sk->replayer && derand_record_ops.skb_mstamp_get){
		derand_record_ops.skb_mstamp_get(sk, cl, loc);
		return;
	}

	// not in replay mode
	skb_mstamp_get(cl);

	// if in record mode
	if (sk->recorder && derand_record_ops.skb_mstamp_get)
		derand_record_ops.skb_mstamp_get(sk, cl, loc);
}

/* A call to skb_still_in_host_queue */
#define derand_skb_still_in_host_queue(sk, skb, call) \
	({ \
		bool ret; \
		if (sk->replayer && derand_record_ops.replay_skb_still_in_host_queue) \
			ret = derand_record_ops.replay_skb_still_in_host_queue(sk, skb); \
		else { \
			ret = (call); \
			if (sk->recorder && derand_record_ops.record_skb_still_in_host_queue) \
				derand_record_ops.record_skb_still_in_host_queue(sk, ret); \
		} \
		ret; \
	})

/* A general event */
static inline void derand_general_event(const struct sock *sk, int loc, u64 data){
	if (derand_record_ops.general_event){
		if (sk->recorder || sk->replayer)
			derand_record_ops.general_event(sk, loc, data);
	}
}

#endif /* DERAND_ENABLE */

#endif /* _NET_DERAND_OPS_H */
