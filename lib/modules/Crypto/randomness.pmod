// $Id: randomness.pmod,v 1.22 2003/01/04 00:36:06 nilsson Exp $

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
#endif

static RandomSource global_arcfour;
static int(0..1) goodseed;

#ifdef __NT__
static nt_random_string(int len) {
  object ctx = Crypto.nt.CryptAcquireContext(0, 0, Crypto.nt.PROV_RSA_FULL,
					     Crypto.nt.CRYPT_VERIFYCONTEXT
					     /*|Crypto.nt.CRYPT_SILENT*/);
  if(!ctx)
    error( "Couldn't create crypto context.\n" );

  string res = ctx->CryptGenRandom(len);

  if(!res)
    error( "Couldn't generate randomness.\n" );

  destruct(ctx);
  return res;
}
#endif

//! Executes several programs (last -256, arp -a, netstat -anv, netstat -mv,
//! netstat -sv, uptime, ps -fel, ps aux, vmstat -s, vmstat -M, iostat,
//! iostat -cdDItx) to generate some entropy from their output. On Microsoft
//! Windows the Windows cryptographic routines are called to generate random
//! data.
string some_entropy()
{
#ifdef __NT__
  return nt_random_string(8192);
#else /* !__NT__ */
  mapping env = getenv();
  env->PATH = PATH;

  Stdio.File parent_pipe = Stdio.File();
  Stdio.File child_pipe = parent_pipe->pipe();
  if (!child_pipe)
    error( "Couldn't create pipe.\n" );

  Stdio.File null = Stdio.File("/dev/null","rw");

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

//! Virtual class for randomness source object.
class RandomSource {

  //! Returns a string of length len with (pseudo) random values.
  string read(int(0..) len) { return random_string(len); }
}

// Compatibility
class pike_random {
  inherit RandomSource;
}

#ifdef __NT__
static class NTSource {
  string read(int(0..) len) { return nt_random_string(len); }
}
#endif

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

//! Returns a reasonably random random-source.
RandomSource reasonably_random()
{
#ifdef __NT__
  return NTSource();
#endif

  if (file_stat(PRANDOM_DEVICE))
  {
    object res = Stdio.File();
    if (res->open(PRANDOM_DEVICE, "r"))
      return res;
  }

  if (global_arcfour)
    return global_arcfour;

  string seed = some_entropy();
#if constant(Crypto.arcfour)
  if (strlen(seed) < 2001)
    seed = random_string(2001); // Well, we're only at reasonably random...
  return (global_arcfour = arcfour_random(sprintf("%4c%O%s", time(),
						  _memory_usage(), seed)));
#else /* !constant(Crypto.arcfour) */

  // Not very random, but at least a fallback...
  if(!goodseed) {
    random_seed( time() + Array.sum( (array)seed ) );
    goodseed = 1;
  }
  return global_arcfour = pike_random();
#endif /* constant(Crypto.arcfour) */
}

//! Returns a really random random-source.
RandomSource really_random(int|void may_block)
{
#ifdef __NT__
  return NTSource();
#endif
  Stdio.File res = Stdio.File();
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

  error( "No source found.\n" );
}
