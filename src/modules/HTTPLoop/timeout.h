void aap_init_timeouts(void);
void aap_exit_timeouts(void);
void aap_remove_timeout_thr(int *hmm);
int *aap_add_timeout_thr(THREAD_T thr, int secs);
extern MUTEX_T aap_timeout_mutex;
