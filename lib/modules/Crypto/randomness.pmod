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
#define SYSTEM_COMMANDS "last & netstat -anv & netstat -mv & netstat -sv & " \
                        "ping -s 255.255.255.255 256 10 & " \
			"ping -c 10 -s 256 255.255.255.255 & " \
                        "uptime & ps -fel & ps aux & " \
			"vmstat -s & vmstat -M & " \
			"iostat & iostat -cdDItx &"
			
#define PRIVATE
			
PRIVATE object global_rc4;

PRIVATE string some_entropy()
{
  object parent_pipe, child_pipe;

  parent_pipe = Stdio.File();
  child_pipe = parent_pipe->pipe();
  if (!child_pipe)
    throw( ({ "Crypto.random->popen: couldn't create pipe\n", backtrace() }) );

  if (!fork())
  { /* Child */
    catch {
      object stderr = Stdio.File();
      object stdin = Stdio.File();
      
      destruct(parent_pipe);
      
      stderr->open("/dev/null", "w");
      stderr->dup2(Stdio.File("stderr"));
      
      stdin->open("/dev/null", "r");
      stdin->dup2(Stdio.File("stdin"));
    
      child_pipe->dup2(Stdio.File("stdout"));
      catch(exece("/bin/sh", ({ "-c", SYSTEM_COMMANDS }), ([ "PATH" : PATH ]) ));
    };
    exit(17);
  } else {
    /* Parent */
    string res;
    destruct(child_pipe);
    res = parent_pipe->read(0x7fffffff);
    return res;
  }
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
