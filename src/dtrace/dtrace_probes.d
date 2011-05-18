provider pike {
  probe fn__start(char *fn, char *obj);
  probe fn__popframe();
  probe fn__done(char *fn);
};