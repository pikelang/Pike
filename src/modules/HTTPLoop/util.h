int aap_get_time(void);
int aap_swrite(int to, char *buf, int towrite);

#define H_EXISTS 0
#define H_INT    1
#define H_STRING 2


int aap_get_header(struct args *req, char *header, int operation, void *res);

