void init_main_efuns(void);
void init_main_programs(void);
void exit_main(void);
void init_files_efuns(void);
void init_files_programs(void);
void exit_files(void);
void init_image_efuns(void);
void init_image_programs(void);
void exit_image(void);
void init_math_efuns(void);
void init_math_programs(void);
void exit_math(void);
void init_pipe_efuns(void);
void init_pipe_programs(void);
void exit_pipe(void);
void init_regexp_efuns(void);
void init_regexp_programs(void);
void exit_regexp(void);
void init_spider_efuns(void);
void init_spider_programs(void);
void exit_spider(void);
void init_sprintf_efuns(void);
void init_sprintf_programs(void);
void exit_sprintf(void);

struct module module_list UGLY_WORKAROUND={
  { "main", init_main_efuns, init_main_programs, exit_main, 0 }
 ,{ "files", init_files_efuns, init_files_programs, exit_files, 0 }
 ,{ "image", init_image_efuns, init_image_programs, exit_image, 0 }
 ,{ "math", init_math_efuns, init_math_programs, exit_math, 0 }
 ,{ "pipe", init_pipe_efuns, init_pipe_programs, exit_pipe, 0 }
 ,{ "regexp", init_regexp_efuns, init_regexp_programs, exit_regexp, 0 }
 ,{ "spider", init_spider_efuns, init_spider_programs, exit_spider, 0 }
 ,{ "sprintf", init_sprintf_efuns, init_sprintf_programs, exit_sprintf, 0 }
};
