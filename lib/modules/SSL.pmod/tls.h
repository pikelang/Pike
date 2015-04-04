
#ifdef SSL3_DEBUG
#define SSL3_DEBUG_MSG(X ...)  werror(X)
#else
#define SSL3_DEBUG_MSG(X ...)
#endif

#ifdef SSL3_DEBUG_CRYPT
#define SSL3_DEBUG_CRYPT_MSG(X ...)  werror(X)
#else
#define SSL3_DEBUG_CRYPT_MSG(X ...)
#endif

#define COND_FATAL(X,Y,Z) if(X) {          \
    send_packet(alert(ALERT_fatal, Y, Z)); \
    return -1;                             \
  }
