#pike 7.5

Crypto.SHA1_Algorithm sha = Crypto.SHA1();

string name() { return "SHA"; }

function update = sha->update;
function digest = sha->digest;
