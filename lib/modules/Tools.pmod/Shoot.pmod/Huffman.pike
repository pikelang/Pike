#pike __REAL_VERSION__
inherit Tools.Shoot.Test;

constant name="Huffman";

string(8bit) data = Stdio.read_bytes(master()->_master_file_name);

int perform()
{
  string(8bit) h = Standards.HPack.huffman_encode(data);
  string(8bit) d = Standards.HPack.huffman_decode(h);
  return sizeof(data);
}
