/* randomness.pmod
 *
 * Assorted stronger or weaker randomnumber generators.
 */

/* These devices tries to collect entropy from the environment.
 * They differ in behaviour when they run low on entropy, /dev/random
 * will block if it can't provide enough random bits, while /dev/urandom
 * will degenerate into a reasonably strong pseudo random generator */

#define RANDOM_DEVICE "/dev/random"
#define PRANDOM_DEVICE "/dev/urandom"

/* Collect somewhat random data from the environment. Unfortunately,
 * this is quite system dependent */
#define PATH  "/usr/sbin:/usr/etc:/usr/bin/:/sbin/:/etc:/bin"

#ifndef __NT__
#define SYSTEM_COMMANDS ({ "last -256", "arp -a", \
                        "netstat -anv","netstat -mv","netstat -sv", \
                        "uptime","ps -fel","ps aux", \
			"vmstat -s","vmstat -M", \
			"iostat","iostat -cdDItx"})
#else
#define SYSTEM_COMMANDS ({ "mem /c", "arp -a", "vol", "dir", "net view", \
    "net statistics workstation","net statistics server", "net view" \
    "net user" })
#endif
			
#define PRIVATE
			
PRIVATE object global_rc4;

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


class pike_random {
  string read(int len)
  {
    return sprintf("%@c", Array.map(allocate(len), lambda(int dummy)
				    {
				      return random(256);
				    } ));
  }
}

class rc4_random {
  inherit Crypto.rc4 : rc4;

  void create(string secret)
  {
    object hash = Crypto.sha();
    hash->update(secret);
    
    rc4::set_encrypt_key(hash->digest());
  }

  string read(int len)
  {
    return rc4::crypt(replace(allocate(len), 0, "\47") * "");
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

