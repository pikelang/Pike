#pike 7.5

Crypto.MD5_Algorithm md5 = Crypto.MD5();

string identifier() { return "*\206H\206\367\r\2\5"; }
string name() { return "MD5"; }

function update = md5->update;
function digest = md5->digest;
