#pike 8.1

inherit Crypto.Random : pre;

string(8bit) blocking_random_string(int len)
{
  return pre::rnd_func(len);
}
