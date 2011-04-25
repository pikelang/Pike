/* $Id$
 */

//! Assorted stronger or weaker randomnumber generators.
//! These devices try to collect entropy from the environment.
//! They differ in behaviour when they run low on entropy, /dev/random
//! will block if it can't provide enough random bits, while /dev/urandom
//! will degenerate into a reasonably strong pseudo random generator

#pike __REAL_VERSION__

static constant RANDOM_DEVICE = "/dev/random";
static constant PRANDOM_DEVICE = "/dev/urandom";

/* Collect somewhat random data from the environment. Unfortunately,
 * this is quite system dependent
 */
static constant PATH = "/usr/sbin:/usr/etc:/usr/bin/:/sbin/:/etc:/bin";

#ifndef __NT__
static constant SYSTEM_COMMANDS = ({
  "last -256", "arp -a", 
  "netstat -anv","netstat -mv","netstat -sv", 
  "uptime","ps -fel","ps aux", 
  "vmstat -s","vmstat -M", 
  "iostat","iostat -cdDItx"
});
#else
static constant SYSTEM_COMMANDS = ({
  "mem /c", "arp -a", "vol", "dir", "net view",
  "net statistics workstation","net statistics server",
  "net user"
});
#endif

// *****************************			
#define private
			
private object global_arcfour;

//	Executes several programs to generate some entropy from their output.
private string some_entropy()
{
#ifdef __NT__
  object ctx = Crypto.nt.CryptAcquireContext(0, 0, Crypto.nt.PROV_RSA_FULL,
					     Crypto.nt.CRYPT_VERIFYCONTEXT
					     /*|Crypto.nt.CRYPT_SILENT*/);
  if(!ctx)
    error( "Crypto.random: couldn't create crypto context.\n" );

  string res = ctx->CryptGenRandom(8192);

  if(!res)
    error( "Crypto.random: couldn't generate randomness.\n" );

  destruct(ctx);

  return res;
#else /* !__NT__ */
  string res;
  object parent_pipe, child_pipe;
  mapping env=getenv()+([]);

  parent_pipe = Stdio.File();
  child_pipe = parent_pipe->pipe();
  if (!child_pipe)
    error( "Crypto.random->popen: couldn't create pipe.\n" );

  object null=Stdio.File("/dev/null","rw");
  env["PATH"]=PATH;
  
  foreach(SYSTEM_COMMANDS, string cmd)
    {
      catch {
	Process.create_process(Process.split_quoted_string(cmd),
				       (["stdin":null,
					"stdout":child_pipe,
					"stderr":null,
					"env":env]));
      };
    }

  destruct(child_pipe);
  
  return parent_pipe->read();
#endif
}


//! A pseudo random generator based on the ordinary random() function.
class pike_random {

  //! Returns a string of length len with pseudo random values.
  string read(int len)
  {
#if 1 // major optimization /Hubbe
      return random_string(len);
#else
#if 1 // 30% optimization /Hubbe
    string ret="";
    if(len>=16384)
    {
      array x=allocate(16384,random);
      for(int e=0;e<(len/16384);e++) ret+=(string)x(256);
    }
    ret+=(string)allocate(len % 16384, random)(256);
    return ret;
#else
    if (len > 16384) return read(len/2)+read(len-len/2);
    return (string)allocate(len, random)(256);
#endif
#endif
  }
}

#if constant(Crypto.arcfour)
//! A pseudo random generator based on the arcfour crypto.
class arcfour_random {

  inherit Crypto.arcfour : arcfour;

  //! Initialize and seed the arcfour random generator.
  void create(string secret)
  {
    object hash = Crypto.sha();
    hash->update(secret);
    
    arcfour::set_encrypt_key(hash->digest());
  }

  //! Return a string of the next len random characters from the
  //! arcfour random generator.
  string read(int len)
  {
    if (len > 16384) return read(len/2)+read(len-len/2);
    return arcfour::crypt("\47" * len);
  }
}

#endif /* constant(Crypto.arcfour) */

//!
object reasonably_random()
{
  if (file_stat(PRANDOM_DEVICE))
  {
    object res = Stdio.File();
    if (res->open(PRANDOM_DEVICE, "r"))
      return res;
  }

  if (global_arcfour)
    return global_arcfour;

#if constant(Crypto.arcfour)  
  string seed = some_entropy();
  if (strlen(seed) > 2000)
    return (global_arcfour = arcfour_random(sprintf("%4c%O%s", time(), _memory_usage(), seed)));
#else /* !constant(Crypto.arcfour) */
  /* Not very random, but at least a fallback... */
  return global_arcfour = pike_random();
#endif /* constant(Crypto.arcfour) */
  error( "Crypto.randomness.reasonably_random: No source found.\n" );
}

//!
object really_random(int|void may_block)
{
  object res = Stdio.File();
  if (may_block && file_stat(RANDOM_DEVICE))
  {
    if (res->open(RANDOM_DEVICE, "r"))
      return res;
  }

  if (file_stat(PRANDOM_DEVICE))
  {
    if (res->open(PRANDOM_DEVICE, "r"))
      return res;
  }
    
  error( "Crypto.randomness.really_random: No source found.\n" );
}

