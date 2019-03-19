#pike __REAL_VERSION__
inherit .Huffman;

constant name="Huffman (binary)";

string(8bit) data = Crypto.Random.random_string(16384);
