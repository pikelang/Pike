/* $Id: randomness.pmod,v 1.11 1999/08/25 17:32:54 grubba Exp $
 */

//! module Crypto
//! submodule randomness
//!	Assorted stronger or weaker randomnumber generators.

/* These devices tries to collect entropy from the environment.
 * They differ in behaviour when they run low on entropy, /dev/random
 * will block if it can't provide enough random bits, while /dev/urandom
 * will degenerate into a reasonably strong pseudo random generator */

static constant RANDOM_DEVICE = "/dev/random";
static constant PRANDOM_DEVICE = "/dev/urandom";

/* Collect somewhat random data from the environment. Unfortunately,
 * this is quite system dependent */
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
			
#define PRIVATE
			
PRIVATE object global_rc4;

// method string some_entropy()
//	Executes several programs to generate some entropy from their output.
PRIVATE string some_entropy()
{
  string res;
  object parent_pipe, child_pipe;
  mapping env=getenv()+([]);

  parent_pipe = Stdio.File();
  child_pipe = parent_pipe->pipe();
  if (!child_pipe)
    throw( ({ "Crypto.random->popen: couldn't create pipe\n", backtrace() }) );


#ifndef __NT__
  object null=Stdio.File("/dev/null","rw");
  env["PATH"]=PATH;
#else
  object null=Stdio.File("nul:","rw");
#endif
  
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
}


//! class pike_random
//!	A pseudo random generator based on the ordinary random() function.
class pike_random {
  //! method string read(int len)
  //!	Returns a string of length len with pseudo random values.
  string read(int len)
  {
    if (len > 16384) return read(len/2)+read(len-len/2);
    return (string)allocate(len, random)(256);
  }
}

//! class rc4_random
//!	A pseudo random generator based on the rc4 crypto.
class rc4_random {
  inherit Crypto.rc4 : rc4;

  //! method void create(string secret)
  //!	Initialize and seed the rc4 random generator.
  void create(string secret)
  {
    object hash = Crypto.sha();
    hash->update(secret);
    
    rc4::set_encrypt_key(hash->digest());
  }

  //! method string read(int len)
  //!	Return a string of the next len random characters from the
  //!	rc4 random generator.
  string read(int len)
  {
    if (len > 16384) return read(len/2)+read(len-len/2);
    return rc4::crypt("\47" * len);
  }
}


object reasonably_random()
{
  if (file_stat(PRANDOM_DEVICE))
  {
    object res = Stdio.File();
    if (res->open(PRANDOM_DEVICE, "r"))
      return res;
  }

  if (global_rc4)
    return global_rc4;
  
  string seed = some_entropy();
  if (strlen(seed) > 2000)
    return (global_rc4 = rc4_random(sprintf("%4c%O%s", time(), _memory_usage(), seed)));
  
  throw( ({ "Crypto.randomness.reasonably_random: No source found\n", backtrace() }) );
}

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
    
  throw( ({ "Crypto.randomness.really_random: No source found\n", backtrace() }) );
}

