#pike __REAL_VERSION__

void sanitize(array(string) f) {
    for (int i = 0; i < sizeof(f); i++) {
        if (f[i][0] == '"' && f[i][-1] == '"') {
            f[i] = f[i][1..sizeof(f[i])-2];
	}
    }
}

//! Parsing function for geoip databases in the format used my
//! @url{http://www.maxmind.com/@}.
void parse_maxmind(string line, object tree) {
    array(string) f;
    f = line / ",";
    sanitize(f);
    tree[f[0]] = Geography.Countries[f[4]];
}

//! Parsing function for geoip databases in the format used my
//! @url{http://software77.net/@}.
void parse_77(string line, object tree) {
    array(string) f;
    if (line[0] == '#') return;

    sanitize(f=line/",");
    tree[ADT.CritBit.get_ipv4((int)f[0])] = Geography.Countries[f[4]];
}

//! Base class for GeoIP lookups. Use @[Geography.GeoIP.IPv4].
class IP {
    object tree;

    void parse(string file_name, function(string,object:array) parse_line) {
	Stdio.FILE f = Stdio.FILE(file_name, "r");

	while (string line = f->gets()) {
	    line = String.trim_all_whites(line);
	    if (!sizeof(line)) continue;
	    parse_line(line, tree);
	}
    }

    //! Returns the geographical location of the given ip address @expr{ip@}.
    //! When this object has been created using one of the standard parsing
    //! functions the locations are instances of
    //! @[Geography.Countries.Country].
    mixed from_ip(string ip) {
	return tree[ip] || tree[tree->previous(ip)];
    }
}

//! Class for GeoIP lookups of ipv4 addresses.
//! Uses @[ADT.CritBit.IPv4Tree] objects internally to map IPv4 addresses to
//! a geographical region.
class IPv4 {
    inherit IP;

    //! @decl void create(string file_name, function(string,ADT.CritBit.IPv4Tree:void) fun)
    //! @decl void create(ADT.CritBit.IPv4Tree tree)
    //!
    //! Objects of this class can either be created from a file
    //! @expr{file_name@} with an optional parsing function @expr{fun@}.
    //! When @expr{fun@} is omitted, it defaults to
    //! @[Geography.GeoIP.parse_maxmind]. @expr{fun@} will be called for each line in
    //! @expr{file_name@} and the critbit tree to add the entry to.
    //!
    //! Alternatively, an instance of @[ADT.CritBit.IPv4Tree] can be passed.
    //! @expr{tree@} is expected to map the first address of each range to
    //! its geographical location.
    void create(string|object file_name, void|function(string,object:void) parse_line) {
	if (objectp(file_name)) {
	    tree = file_name;
	    return;
	}
	tree = ADT.CritBit.IPv4Tree();
	parse(file_name, parse_line||parse_maxmind);
    }
}
