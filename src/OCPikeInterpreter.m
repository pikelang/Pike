#import "OCPikeInterpreter.h"
#import <Foundation/NSString.h>
#import <Foundation/NSBundle.h>
#import <Foundation/NSException.h>

PMOD_EXPORT OCPikeInterpreter * sharedInstance = nil;

struct Pike_interpreter_struct * Pike_interpreter_pointer;
 
static void set_master(const char *file)
{
  if (strlen(file) >= MAXPATHLEN*2 ) {
    fprintf(stderr, "Too long path to master: \"%s\" (limit:%d)\n",
            file, MAXPATHLEN*2 );
    exit(1);
  }
//  strcpy(master_location, file);
}

 void set_default_master(void)
{
}

@implementation OCPikeInterpreter

+(OCPikeInterpreter *)sharedInterpreter
{
  @synchronized(self)
  {
    if ( sharedInstance == nil )
      [[self alloc] init];
	
  }
  return sharedInstance;
}

+ (id)allocWithZone:(NSZone *)zone
{
  @synchronized(self) {
    if (sharedInstance == nil) {
      sharedInstance = [super allocWithZone:zone];
      return sharedInstance;
    }
  }
 
  return nil;
}

- (id)copyWithZone:(NSZone *)zone
{
  return self;
}

- (id)retain
{
  return self;
}
 
- (void)release
{
  // do nothing
}
 
- (id)autorelease
{
  return self;
}
 
- (NSUInteger)retainCount
{
  return NSUIntegerMax; // This is sooo not zero
}

- (struct Pike_interpreter_struct *) getInterpreter
{
  return pike_get_interpreter_pointer();
}

- (void)setMaster:(id) master
{
	
  if ([master length] >= MAXPATHLEN*2 ) 
  {
    fprintf(stderr, "Too long path to master: \"%s\" (limit:%d)\n",
            [master UTF8String], MAXPATHLEN*2 );
	    exit(1);
   }
   master_location = [master copy];

}

void shared_interpreter_cleanup(int exitcode)
{
  printf("exiting interpreter.\n");	
}

- (BOOL)startInterpreter
{
   JMP_BUF back;
   int num = 0;
   struct object *m;
   char ** argv = NULL;
   char * master;	
   id this_bundle;

   objc_registerThreadWithCollector();

   this_bundle = [NSBundle bundleForClass: [self class]];
   if(!this_bundle || this_bundle == nil)
   {
	   NSException * exception = [NSException exceptionWithName:@"Error finding bundle!" reason:@"bundleForClass: returned nil." userInfo: nil];
  	 @throw exception;
   }

   if(!master_location)
   {
     id masterPath;
     masterPath = [[this_bundle resourcePath] stringByAppendingString: @"/lib/master.pike"];     
     [self setMaster: masterPath];   
   }

    master = (char *)[master_location UTF8String];
    init_pike(argv, master);
    init_pike_runtime(shared_interpreter_cleanup);

    Pike_interpreter_pointer = get_interpreter_pointer();

    add_pike_string_constant("__embedded_resource_directory",
        master,
        strlen(master));
								
    add_pike_string_constant("__master_cookie",
                             master,
                            strlen(master));

    if(SETJMP(jmploc))
    {
      if(throw_severity == THROW_EXIT)
      {
        num=throw_value.u.integer;
      }else{
	      if (throw_value.type == T_OBJECT &&
	          throw_value.u.object->prog == master_load_error_program &&
	          !get_master()) {  
	        /* Report this specific error in a nice way. Since there's no
	         * master it'd be reported with a raw error dump otherwise. */
	        struct generic_error_struct *err;

	        dynamic_buffer buf;
	        dynbuf_string s;
	        struct svalue t;

		move_svalue (Pike_sp++, &throw_value);
		mark_free_svalue (&throw_value);
	        err = (struct generic_error_struct *)
	          get_storage (Pike_sp[-1].u.object, generic_error_program);

	        t.type = PIKE_T_STRING;
	        t.u.string = err->error_message;

	        init_buf(&buf);
	        describe_svalue(&t,0,0);
	        s=complex_free_buf(&buf);

	        fputs(s.str, stderr);
	        free(s.str);
	      } 
	      else
	        call_handle_error();
	    }
	  }else{


	    if ((m = load_pike_master())) {
	      back.severity=THROW_EXIT;
		}
   return YES;
}
  return NO;
}

- (BOOL)stopInterpreter
{
  UNSETJMP(jmploc);
  pike_do_exit(0);	
  return YES;	
}

- (struct program *)compileString: (id)code
{
  struct program * p = NULL;
  push_text([code UTF8String]);
  f_utf8_to_string(1);
  f_compile(1);
  if(Pike_sp[-1].type==T_PROGRAM)
  {
    p = Pike_sp[-1].u.program;
    add_ref(p);
  }
  pop_n_elems(1);
  return p;	
}

- (struct svalue *)evalString: (id)expression
{
    int i;
    struct object * o;
    struct program * p;
    struct svalue * s;
    struct svalue * res;
    id desc, c;

    objc_registerThreadWithCollector();
    c = [[NSMutableString alloc] initWithCapacity: 200];

    [c setString: @"mixed foo(){ return("];
    [c appendString: expression];
    [c appendString: @");}"];

    p = [self compileString: c];

    if(!p) return NULL;
 
	i=find_identifier("foo", p);

    o = low_clone(p);

    apply_low(o, i, 0);

    s = malloc(sizeof(struct svalue));

    if(!s) 
    {
      printf("unable to allocate svalue!\n");
    }

    res = Pike_sp-1;

    assign_svalue_no_free(s, res); 
    if(o)
      free_object(o);
    if(p)
      free_program(p);
    pop_stack();

    return s;
}

- (BOOL)isStarted
{
  return (is_started?YES:NO);
}

@end /* OCPikeInterpreter */

/*

	following is a simple example of how to use OCPikeInterpreter to embed a pike interpreter into your application,
	this code relies on GC present in Mac OSX 10.5 +

         make framework
         cp -rf Pike.framework /Library/Frameworks

         gcc -I . -fobjc-gc -c test.m -o test.o
         gcc test.o -o test -framework Pike  -framework Foundation

*/

/*

#import <Pike/OCPikeInterpreter.h>
#import <Foundation/NSString.h>
#import <objc/objc-auto.h>

struct Pike_interpreter_struct * Pike_interpreter_pointer;

int main()
{
  id i;
  struct svalue * sv;

  // required for console mode objective c applications
  objc_startCollectorThread();

  // these 3 lines set up and start the interpreter.
  i = [OCPikeInterpreter sharedInterpreter];
  [i startInterpreter];
  Pike_interpreter_pointer = [i getInterpreter];

  // ok, now that we have things set up, let's use it.
  // first, an example of calling pike c level apis directly.  
  f_version(0);
  printf("%s\n", Pike_sp[-1].u.string->str);
  pop_stack();

  // next, we'll demonstrate one of the convenience functions available
  sv = [i evalString: @"1+2"];
  printf("type: %d, value: %d\n", sv->type, sv->u.integer);
  free_svalue(sv);

  // finally, we clean up.
  [i stopInterpreter];

  return 0;
}

*/
