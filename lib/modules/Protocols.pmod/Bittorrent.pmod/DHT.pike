#pike __REAL_VERSION__

//
//! DHT implementation for bittorrent
//! Implemented BEPs*:
//! [X] BEP005 DHT Protocol
//! [ ] BEP032 IPv6 extension for DHT
//! [ ] BEP033 DHT scrape
//! [ ] BEP042 DHT security extensions
//! [ ] BEP043 Read-only DHT nodes
//! [ ] BEP044 Storing arbitrary data in the DHT
//! [ ] BEP045 Multiple-address operation for the Bittorrent DHT
//!
//! *) See http://bittorrent.org/beps/bep_0000.html for a list of all BEPs
//!

#ifdef NODE_STATE_DEBUG
#define NODE_STATE_CHANGE werror
#else
#define NODE_STATE_CHANGE(x ...)
#endif /* NODE_STATE_DEBUG */

#ifdef DEBUG_DHT
void dwerror(string fmt, mixed ... args) {
  werror("DHT DEBUG: " + fmt, @args);
}
#else
#define dwerror(X ...)
#endif

// Error codes for use in callbacks
enum DHT_ERROR {
  DHT_GENERIC_ERROR  = 201,
  DHT_SERVER_ERROR   = 202,
  DHT_PROTOCOL_ERROR = 203,
  DHT_UNKNOWN_METHOD = 204,
  DHT_QUERY_TIMEOUT  = 205, // Not part of BEP005!
};

// System wide bucket size (The K constant)
constant BUCKET_SIZE = 8;

// Maximum number of buckets we may create.
constant MAX_BUCKETS = 160;

//! Our global ID for this DHT router, expressed as a 20 byte hash.
protected string my_node_id = UNDEFINED;

//! Indicates if the DHT instance is up and running or not.
int is_running = 0;

//! The UDP port on which we listen for messages.
Stdio.UDP port;

// Some statistics that we collect. This is not a stable API at the
// moment.
mapping stats = ([]);

Routingtable rt;

// Callbacks called when a nodes has been added or removed from a
// bucket, added as a candidate, promoted from candidate to peer or
function(DHTNode, object /* Bucket */:void) node_added_cb, node_removed_cb,
  candidate_added_cb, candidate_promoted_cb, candidate_removed_cb;

//
//! Abstraction for the routing table.
class Routingtable {
  //! Node ID that this routing table belongs to
  protected string my_node_id;

  //! Lookup table for nodess so we can quickly find out if any given
  //! hash is already in our table somewhere.
  mapping(string:DHTNode) nodes_by_hash = set_weak_flag(([]), Pike.WEAK);
  mapping(string:DHTNode) nodes_by_endpoint = set_weak_flag(([]), Pike.WEAK);

  //! Buckets in our routing table
  mapping(string:Bucket) bucket_by_uuid = ([]);
  array(Bucket) buckets = ({ Bucket() });


  protected class Bucket {
    string uuid;

    // Live nodes in the bucket.
    array(object) nodes = ({});

    // Candidate nodes, i.e. nodes we know about and would like to
    // insert, but couldn't because the bucket was full.
    array(object) candidates = ({});

    //
    //! returns a mapping with overall information about the bucket.
    mapping info() {
      return ([
	"uuid"       : uuid,
	"nodes"      : sizeof(nodes),
	"candidates" : sizeof(candidates),
      ]);
    }

    //
    //! Returns a mapping with details about the bucket, including a
    //! complete list of live and candidate nodes.
    mapping details() {
      return ([
	"uuid"       : uuid,
	"nodes"      : nodes->node_info(),
	"candidates" : candidates->node_info(),
      ]);
    }

    protected void create() {
      uuid = (string)Standards.UUID.make_version4();
      bucket_by_uuid[uuid] = this;
    }

    protected string _sprintf(int t) {
      return sprintf("Bucket(%d/%d %s)", sizeof(nodes), sizeof(candidates), uuid);
    }

    //! Adds a node to the bucket as a candidate with the option of
    //! suppressing notifications. Returns 0 on success.
    protected int low_add_candidate(DHTNode n, void|int dont_notify) {
      if (sizeof(candidates) <= BUCKET_SIZE) {
	candidates += ({ n });
	nodes_by_hash[n->node_id] = n;
	if (!dont_notify && functionp(candidate_added_cb)) {
	  mixed err = catch {
	      candidate_added_cb(n, this);
	    };
	}
	if (n->state != n->DHT_ACTIVE) {
	  n->ping();
	}
	return 0;
      }
      return -1;
    }

    //! Adds a node to the bucket as a live node with the option to
    //! surpress notifications. Returns 0 on success.
    int low_add_node(DHTNode n, void|int dont_notify) {
      if (!is_full()) {
	nodes += ({ n });
	nodes_by_hash[n->node_id] = n;
	nodes_by_endpoint[n->endpoint()] = n;
	if (!dont_notify && functionp(node_added_cb)) {
	  mixed err = catch {
	      node_added_cb(n, this);
	    };
	}
	if (n->state != n->DHT_ACTIVE) {
	  n->ping();
	}
	return 0;
      }
      return -1;
    }

    //
    //! Attempts to add a node to the bucket either as a live node or as
    //! a candidate if the bucket is already full. Optionally supresses
    //! notifications of the new node.
    //! Returns 0 if the node was successfully added.
    int add_node(DHTNode n, void|int dont_notify) {
      if (low_add_node(n, dont_notify)) {
	return low_add_candidate(n, dont_notify);
      }
      return 0;
    }

    //
    //! Remove a node from this bucket and optionally surpress
    //! notification of the event.
    //! Returns 0 if the node was successfully removed.
    int remove_node(DHTNode n, void|int dont_notify) {
      m_delete(nodes_by_hash, n->node_id);
      m_delete(nodes_by_endpoint, n->endpoint());

      if (has_value(nodes, n)) {
	nodes -= ({ n });
	// Make sure we cancel the check node callout
	n->cancel_check_node();

	if (!dont_notify && functionp(node_removed_cb)) {
	  mixed err = catch {
	      node_removed_cb(n, this);
	    };
	}
	return 0;
      } else if (has_value(candidates, n)) {
	candidates -= ({ n });

	// Make sure we cancel the check node callout
	n->cancel_check_node();

	if (!dont_notify && functionp(candidate_removed_cb)) {
	  mixed err = catch {
	      candidate_removed_cb(n, this);
	    };
	}
	return 0;
      }
      return -1;
    }

    //
    //! Attempts to promote nodes if there is space and we have
    //! candidates. Called by add_node() before adding a node to ensure
    //! we upgrade viable candidates before adding a new node. This
    //! ensures that new nodes starts their life in the candidates
    //! section until there is space for them.
    void promote_nodes() {
      // Sort the candidate list based so that the most recent peer
      // we've spoken to is first in the list.
      sort(candidates->age, candidates);
      foreach(candidates, DHTNode c) {

	if (c->state != DHTNode.DHT_ACTIVE) continue;
	if (is_full()) break;

	candidates -= ({ c });
	nodes += ({ c });
	if (functionp(candidate_promoted_cb)) {
	  mixed err = catch {
	      candidate_promoted_cb(c, this);
	    };
	}
      }
    }

    //
    //! Used to evict bad nodes from the bucket. Used by add_node()
    //! before attempting to add nodes the bucket.
    void evict_bad_nodes() {
      foreach(nodes, DHTNode n) {
	if (n->state == DHTNode.DHT_BAD) {
	  nodes -= ({ n });
	  m_delete(nodes_by_hash, n->node_id);
	  m_delete(nodes_by_endpoint, n->endpoint());
	  destruct(n);
	}
      }
      foreach(candidates, DHTNode n) {
	if (n->state == DHTNode.DHT_BAD) {
	  candidates -= ({ n });
	  m_delete(nodes_by_hash, n->node_id);
	  m_delete(nodes_by_endpoint, n->endpoint());
	  destruct(n);
	}
      }
    }

    //
    //! Returns 1 if the bucket is full of live nodes.
    int(0..1) is_full() {
      return (sizeof(nodes) >= BUCKET_SIZE);
    }
  } /* class Bucket */


  //! Splits the given bucket into two by pushing a new bucket to the
  //! end of the bucket array. All nodes in the given bucket are removed
  //! and re-added to redistribute them. Candidate nodes are also
  //! readded and then a separate promotion pass is done. The promotion
  //! pass can be inhibited by setting dont_promote to 1.
  int split_bucket(Bucket b, void|int dont_promote) {
    if (sizeof(buckets) >= MAX_BUCKETS) {
      return -1;
    }

    // Add a new bucket to the tree
    Bucket nb = Bucket();
    buckets += ({ nb });

    // Redistribute nodes we know of from the old bucket.
    foreach(b->nodes, DHTNode n) {
      b->remove_node(n);
      add_node(n);
    }

    // Redistribute the candidates for the bucket too but don't promote
    // anyone yet.
    foreach(b->candidates, DHTNode n) {
      b->remove_node(n);
      Bucket b = bucket_for(n);
      b->low_add_candidate(n);
    }

    if (!dont_promote) {
      // Now we perform the promotions as needed
      b->promote_nodes();
      nb->promote_nodes();
    }

    return 0;
  }


  int bucket_index_for(string hash) {
    int target_bucket_index = distance_exp(my_node_id, hash);
    target_bucket_index = min(MAX_BUCKETS-1 - target_bucket_index, sizeof(buckets)-1);
    return target_bucket_index;
  }

  //! Calculate and return the bucket in which DHTNode n should belong
  //! to.
  Bucket bucket_for(string|DHTNode n) {
    if (objectp(n)) {
      n = n->node_id;
    }
    int target_bucket_index = bucket_index_for(n);
    Bucket target_bucket = buckets[target_bucket_index];
    return target_bucket;
  }

  //! Callback method that determines if a peer is allowed into our
  //! routing table or not. Return 1 to allow the peer or 0 to ignore
  //! it.
  function(DHTNode:int) allow_node_in_routing_table;

  //! Attepmts to add a node to our routing table. If a node with the
  //! same hash and/or endpoint already exists, that node is returned
  //! instead. If this node is inserted into the routing table, it is
  //! returned. If the node could not be inserted at all, UNDEFINED is
  //! returned.
  DHTNode add_node(string|DHTNode n, void|string ip, void|int port) {
    if (!objectp(n)) {
      // We got passed hash, ip, port
      if (n && nodes_by_hash[n]) {
	// We already know about the peer so let's ignore this
	return nodes_by_hash[n];
      }
      n = DHTNode(n, ip, port);
    } else {
      if (n->node_id && nodes_by_hash[n->node_id]) {
	return nodes_by_hash[n->node_id];
      }
    }

    if (DHTNode n2 = nodes_by_endpoint[sprintf("%s:%d", ip||"", port)]) {
      // We already know this specific peer as another hash
      return n2;
    }

    // Don't allow our own peer id to be inserted.
    if (n->node_id == my_node_id) {
      return UNDEFINED;
    }

    if (functionp(allow_node_in_routing_table)) {
      mixed err = catch {
	  int allowed_peer = allow_node_in_routing_table(n);
	  if (!allowed_peer) return UNDEFINED;
	};
    }

    object target_bucket = bucket_for(n);
    target_bucket->evict_bad_nodes();
    target_bucket->promote_nodes();
    if (target_bucket == buckets[-1] && target_bucket->is_full()) {
      split_bucket(target_bucket);
      // Recalculate the target bucket
      target_bucket = bucket_for(n);
    }
    if (target_bucket->add_node(n)) {
      return UNDEFINED;
    }
    // Make sure we start the periodic checking of the node.
    n->check_node();
    return n;
  }

  //
  //! Serialize the routing table into basic types so that it may be
  //! encoded for storage
  array serialize() {
    array res = ({});
    foreach(buckets, Bucket b) {
      foreach(b->nodes, DHTNode n) {
	mapping ns = n->serialize();
	ns->type_hint = "node";
	res += ({ ns });
      }
      foreach(b->candidates, DHTNode n) {
	mapping ns = n->serialize();
	ns->type_hint = "candidate";
	res += ({ ns });
      }
    }
    return res;
  }

  //
  //! Deserialize an array created by @[serialize].
  void deserialize(array(mapping) nodes) {
    foreach(nodes, mapping n) {
      DHTNode node = add_node(n->node_id, n->address, n->port);
    }
  }

  //
  //! Iterate over the given routingtable, copying the nodes as we go
  //! along.
  void copy_from(this_program rt) {
    foreach(rt->buckets, Bucket b) {
      foreach(b->nodes, DHTNode n) {
	add_node(n);
      }
      foreach(b->candidates, DHTNode n) {
	add_node(n);
      }
    }
  }

  protected void create(string my_node_id) {
    this_program::my_node_id = my_node_id;
  }
} /* class Routingtable */

//
//! Utility class that represents a node that we learned of
//! somehow. Node objects are not part of the DHT yet but can be used
//! to create fullfledged DHTNode objects.
class Node {

  string node_id, address, token;
  int port;

  //
  //! Return basic info about the node as a mapping.
  //! All values are human readable.
  mapping(string:mixed) node_info() {
      mapping res =  ([
	"ip"      : address,
	"port"    : port,
      ]);

      if (node_id) {
	res->node_id = node_id;
      }

      if (token) {
	res->token = token;
      }

      return res;
  }

  //
  //! Like @[node_info] but encodes node id and token as hex-strings.
  mapping(string:mixed) node_info_human() {
    mapping res = node_info();

    if (res->node_id)
	res->node_id = String.string2hex(res->node_id);

      if (stringp(res->token))
	res->token = String.string2hex(res->token);

      return res;
  }

  array(string|int) node_info_array() {
    return ({ node_id, address, port });
  }

  protected mixed cast(string type) {
    switch(type) {
    case "array":
      return node_info_array();

    case "mapping":
      // In the mapping version, we assume we want the node_id in
      // plain text.
      mapping res = node_info();

      return res;

    case "string":
      do {
	array tmp = array_sscanf(address,"%d.%d.%d.%d");
	string ret = sprintf("%c%c%c%c%2c",
			     @tmp,
			     port);
	if (node_id)
	  return node_id + ret;

	return ret;
      } while(0);

    default:
      throw("Unable to cast to " + type);
    }
  }

  //
  //! Returns just the IP and port as an 8-bit string.
  string endpoint_compact() {
    array tmp = array_sscanf(address,"%d.%d.%d.%d");
    string ret = sprintf("%c%c%c%c%2c",
			 @tmp,
			 port);
    return ret;
  }

  //
  //! Returns the node endpoint as plain text
  string endpoint() {
    return sprintf("%s:%d", address, port);
  }

  protected string _sprintf(int t) {
    return sprintf("Node(%s %s)",
		   endpoint(),
		   (node_id?String.string2hex(node_id):"<unknown hash>"));
  }

  protected void create(void|string node_id, string address, int port, void|string token) {
    this_program::node_id = node_id;
    this_program::address = address;
    this_program::port = port;
    this_program::token = token;
  }
}


//
//
//! Represents a node in our routing table. These nodes also have a
//! state compared to the Node class above along with some other fancy
//! stuff.
class DHTNode {
  inherit Node; // We are a superset of the Node, so use it as a base.

  // Possible states for nodes in the DHT
  enum DHT_NODE_STATE {
    DHT_UNKNOWN = -1,
    DHT_BAD     = 0,
    DHT_ACTIVE  = 1,
  };

  // Mapping state id => plain text
  constant state_names = ([
    DHT_UNKNOWN : "UNKNOWN",
    DHT_BAD     : "BAD",
    DHT_ACTIVE  : "ACTIVE",
  ]);

  // Nodes start out as unknown
  int state = DHT_UNKNOWN;

  // Number of ping failures since last communication with the node
  int ping_fails = 0;

  // Time of last outgoing ping to the node
  int last_ping;

  // Outstanding ping requests. We need to track them so we can
  // process timeouts.
  protected mapping pings_inflight = ([]);

  // Time of last activity seen from the node
  int last_response;

  // We attempt to ping the node if we have not heard from it within
  // the last ten minutes. BEP005 specifies that after 15 minutes, a
  // good node shoudl become questionable so we try to prevent this
  // from happening.
  constant check_interval = 600;
  protected mixed check_node_callout;


  //
  //! Time since last response seen from this node
  int `age() {
    return time() - last_response;
  }

  //
  //! Returns the node state as a string.
  string node_state() {
    return state_names[state] || "INVALID";
  }

  //
  //! Returns the compact node info for this node.
  string compact_node_info() {
    	array tmp = array_sscanf(address,"%d.%d.%d.%d");
	return sprintf("%20s%c%c%c%c%2c",
		       node_id,
		       @tmp,
		       port);
  }


  //
  //! Extend the Node::node_info() method to include info relevant to
  //! a DHTNode.
  mapping node_info() {
    mapping res = ::node_info();
    res +=  ([
	"state_name"    : node_state(),
	"state"         : state,
	"last_ping"     : last_ping,
	"ping_fails"    : ping_fails,
	"last_response" : last_response,
	"age"           : age,
      ]);
    return res;
  }

  //
  //! Human readable output version of the @[node_info] method.
  mapping node_info_human() {
    mapping res = node_info();
    if (res->node_id) res->node_id = String.string2hex(res->node_id);
    if (res->token) res->token = String.string2hex(res->token);
    return res;
  }

  //
  //! Call when we see activity from this node to ensure it returns to
  //! good standing. Will set the node state to DHT_ACTIVE, update
  //! last_response and set ping_fails to 0.
  void activity() {
    last_response = time();
    if (state != DHT_ACTIVE) {
      NODE_STATE_CHANGE("Node %s %s => ACTIVE\n",
			(node_id && String.string2hex(node_id)) || "<unknown hash>",
		       node_state());
    }
    state = DHT_ACTIVE;
    ping_fails = 0;
  }

  //
  //! Removes an outstanding ping TXID from the pings_inflight
  //! mapping.
  protected void remove_txid(string txid) {
    mixed res = m_delete(pings_inflight, txid);
  }

  //
  //! Called when a ping request to this peer times out. We set the
  //! state to DHT_BAD and leave it at that.
  protected void ping_timeout(string txid) {
    ping_fails++;
    if (ping_fails >= 5) {
      if (state == DHT_UNKNOWN) {
        NODE_STATE_CHANGE("Node %x UNKNOWN => BAD\n", node_id);
	state = DHT_BAD;
      }
    } else {
      // If the node failed to respond in a timely manner, we change
      // its state unknown and retry.
      //
      // This handles both the case where a ping just times out and
      // also ensures that we keep nodes we have not heard from as
      // unknown after 15 minutes in accordance with BEP005. (We may
      // set it to inactive after 10+timeout minutes though!)
      if (state == DHT_ACTIVE) {
        NODE_STATE_CHANGE("Node %x ACTIVE => UNKNOWN\n", node_id);
      }
      state = DHT_UNKNOWN;

      // Try the peer again
      ping();
    }

    call_out(remove_txid, 20.0); // Allow a grace time for this pong
				 // to arrive, in case it just took
				 // the peer a really long time to
				 // respond.
  }

  //
  //! Ping the node to see if it is still responding.
  void ping() {
    last_ping = time();
    if (!address) {
      dwerror("Attempting to ping DHT node without address\n");
    }

    string txid = send_ping(address, port, pong);
    pings_inflight[txid] = call_out(ping_timeout, 10.0, txid);
  }

  // Callback when we get a PONG response from the remote node.
  protected void pong(mapping data, string source_ip, int source_port) {
    string txid = data->t;
    mixed co = m_delete(pings_inflight, txid);
    if (co) {
      remove_call_out(co);
    }

    if (data->y == "e") {
      // An error occured
      NODE_STATE_CHANGE("Node %x => BAD because of error response to ping\n",
                        node_id);
      state = DHTNode.DHT_BAD;
      return;
    }

    // Adjust peer ID for this peer if we don't know it.
    if (!node_id) node_id = data->r->id;

    if (node_id != data->r->id) {
      // Node has changed it's node id!
    }

    activity();
  }

  // Periodic check of the node to make sure our routing table stays
  // up-to-date with active nodes.
  void check_node() {
    cancel_check_node();
    // If we've seen recent activity, we just schedule a new check later.
    if (last_ping && age < check_interval) {
      check_node_callout = call_out(check_node, check_interval-age);
      return;
    }

    check_node_callout = call_out(check_node, check_interval / 2 + random(check_interval));
    ping();
  }

  //
  //! Removes any outstanding callouts to check the node.
  void cancel_check_node() {
    check_node_callout && remove_call_out(check_node_callout);
    check_node_callout = 0;
  }

  protected void _destruct() {
    cancel_check_node();
  }

  protected string _sprintf(int t) {
    return sprintf("DHTNode(%s:%d)",
		   address, port);
  }
}

//! Base class for operations that need to iterate over the DHT in
//! some way like get_peers and find_node.
class DHTOperation(string target_hash, function done_cb, mixed ... done_cb_args) {

  //
  // Variables controling behaviour of this op

  //! Desired number of results before the op considers itself done
  int desired_results = 1;

  //! Maximum number of concurrent requests allowed
  int(0..) max_outstanding_requests = 32;

  //! Maximum number of requests that may be generated before bailing
  //! out.
  int max_no_requests = 200;

  //! Timeout for the query
  float|int query_timeout_period = 10;

  //
  // Variables part of the API

  //! Number of requests generated by this query
  int reqs = 0;

  //! Result of the operation to be inspected by the done
  //! callback. Content of the result array varies depending on the
  //! concrete implementation of the operation
  array result = ({});

  //! Result counter - may differ from actual number of objects in the
  //! result array in some cases. For example in the get_peers query,
  //! closest nodes may be added in addition to any peers found.
  int  result_count = 0;


  //
  // Internal variables
  protected int start_time;                                        // Time when the query started executing
  protected mapping(string:Node) seen_nodes = ([]);                // Nodes we've already traversed
  protected mapping(string:mapping) transactions_in_flight = ([]); // Outstanding transactions
  protected array(Node) nodes_to_query = ({});                     // Nodes in line to be queried

  protected int i_am_done = 0;                                     // Indicates that the Op is done

  //
  // This method should be overridden by the actual handlers for the
  // operation.
  protected void got_response(mapping resp) {
    throw("NOT IMPLEMENTED");
  }

  //
  //! This method should be overridden by the actual handlers for the
  //! operation.
  protected mapping generate_query() {
    throw("NOT IMPLEMENTED");
  }


  //
  //! Add a node to the list of nodes to query.
  void add_node_to_query(Node n) {
    nodes_to_query += ({ n });
  }

  //
  //! Callback when we get responses. This is private to the
  //! DHTOperation class and should not be overridden. Instead override
  //! the got_response method declared above.
  private void got_response_cb(mapping resp) {
    string txid = resp->t;
    mapping node_info = m_delete(transactions_in_flight, txid);

    if (!node_info) {
      // Response to a transaction we (no longer) expected a response to
      dwerror("Ignoring unknown TXID %x\n", txid);
      return;
    }

    node_info->timeout && remove_call_out(node_info->timeout);
    node_info->timeout = 0;

    if (resp->y == "e") {
      // This was an error response!
      switch(resp->e) {
      case DHT_QUERY_TIMEOUT: // Timeout as defined by us.
      case DHT_UNKNOWN_METHOD:
      case DHT_PROTOCOL_ERROR:
      case DHT_SERVER_ERROR:
      case DHT_GENERIC_ERROR:
	// FIXME: Do we need to handle any of the errors here?
      default:
	break;
      }
    } else {
      // Attempt to add responding peer to our routing table!
      add_node(resp->r->id, node_info->ip, node_info->port);
    }

    got_response(resp);

    // Sort the queue of nodes to query so that we ask closer nodes
    // first.
    nodes_to_query =
      Array.sort_array(nodes_to_query,
		       lambda(Node a, Node b) {
			 int ad = distance_exp(a->node_id, target_hash);
			 int bd = distance_exp(b->node_id, target_hash);
			 return ad>bd;
		       });


    if (is_done()) {
      done();
    } else {
      run();
    }
  }

  //
  //! Called when a transaction ID has been in flight for too long and
  //! we want to stop waiting for an answer. We call @[run] to
  //! ensure we continue processing requests if more are needed.
  private void query_timeout(string txid) {
    dwerror("%x timed out in %x\n", txid, my_node_id);
    m_delete(transactions_in_flight, txid);
    dwerror("TXIDs left: %O\n", indices(transactions_in_flight));
    run();
  }

  //
  //! Create a request to the peer info given. The peer info array is
  //! expected to contain peer hash, peer ip, peer port.
  private void do_query_peer(Node peer) {
    mapping q = generate_query();
    string txid = send_dht_query(peer->address, peer->port, q, got_response_cb);
    if (!txid) {
      dwerror("Failed to send query to remote node %s:%d\n", peer->address, peer->port);
      return;
    }

    mixed timeout = call_out(query_timeout, query_timeout_period, txid);

    reqs++;

    transactions_in_flight[txid] = peer->node_info() + ([
      "time"      : time(),
      "timeout"   : timeout,
    ]);

    seen_nodes[peer->node_id] = peer;
  }

  //
  //! Processes the queue of nodes to query and calls @[done] if we
  //! have enough results or there are no transactions in flight.
  //! This is the method called to initiate a query.
  private void run() {
    Node next_peer;

    do {
      if (is_done()) {
	done();
	return;
      }

      // Check that we have not hit the request limit
      if (max_outstanding_requests &&
	  sizeof(transactions_in_flight) >= max_outstanding_requests) {
	return;
      }


      if (!sizeof(nodes_to_query)) {
	return;
      }

      next_peer = nodes_to_query[0];
      nodes_to_query = nodes_to_query[1..];

      // Ignore nodes we already asked to avoid loops
      if (seen_nodes[next_peer->node_id]) {
	continue;
      }

      if (want_more_results()) {
	do_query_peer(next_peer);
      } else {
      }
    } while(1);
  }

  //! This method returns 1 if we want more results. This will control
  //! if we send out more requests or if we just let the ones in
  //! flight finish before calling the @[done] callback.
  int want_more_results() {
    return !desired_results || (result_count < desired_results);
  }

  //! This method will return 1 if we consider ourselves done. This
  //! should result in the @[done] method being called. Typically, we
  //! are done if there are no transactions in flight.
  int is_done() {
    int res = 1;

    // We are waiting for responses to transactions in flight.
    if (want_more_results() && sizeof(transactions_in_flight)) {
      res = 0;
    }

    // If we want more results and have more nodes to query, we are not done.
    if (want_more_results() && sizeof(nodes_to_query)) {
      res = 0;
    }

    if (reqs > max_no_requests) {
      res = 1;
    }

    return res;
  }

  //
  //! Internal done callback called when the operation finishes. The
  //! main purpose of this callback is to simply call the application
  //! level callback but in some cases it can also be used to modify
  //! the result before calling the application callback.
  protected void done() {
    i_am_done = 1;
    if (functionp(done_cb)) {
      done_cb && done_cb(this, @done_cb_args);
      done_cb = UNDEFINED;
    }
  }

  //
  //! Execute the DHTOperation by ensuring there are nodes in the
  //! nodes_to_query array as well as calling @[run]. If this method
  //! is overridden, it should always call the parent execute methods!
  this_program execute() {
    start_time = time(1);

    // If we don't have any nodes in the nodes_to_query array, we add
    // the closes nodes we know of as a startingpoint.
    if (!nodes_to_query || !sizeof(nodes_to_query)) {
      object b = rt->bucket_for(target_hash);
      nodes_to_query += b->nodes + b->candidates;
    }
    run();
    return this;
  }
} /* DHTOperation */


//
//
//! FindNode implements the find_node query on the DHT.
//!
//! Upon completion, the callback given at instance creation will be
//! called and the result can be found in the results array.
//!
//! For this operation, the results array will contain Node
//! objects. In some cases these objects may also be DHTNode objects,
//! but the callback must not expect this.
class FindNode {
  inherit DHTOperation;

  //! Override the default number of desired results
  int desired_results = 1;

  //
  //! Callback called by parent to generate the query for this
  //! operation
  protected mapping generate_query() {
    return ([
      "q" : "find_node",
      "a" : ([
	"target" : target_hash,
	"id"     : my_node_id,
      ]),
    ]);
  }

  //
  // Response handler for find_node responses.
  protected void got_response(mapping resp) {
    if (resp->y == "e") {
      // We are just notified of an error
      return;
    }

    if (resp->r->nodes) {
      // If we get nodes, the peer didn't know the target hash so it
      // just gave us it's routing bucket for the target hash.
      array compact_node_info = resp->r->nodes/26 - ({ "", 0 });
      foreach(compact_node_info, string cnf) {
	array node_info = array_sscanf(cnf, "%20s%4c%2c");
	if (node_info[0] == target_hash) {
	  node_info[1] = NetUtils.ip_to_string(node_info[1]);
	  Node n = Node(@node_info);
	  result += ({ n });
	  result_count++;
	  done();
	  return;
	}

	node_info[1] = NetUtils.ip_to_string(node_info[1],0);
	add_node_to_query(Node(@node_info));
      }
    }
  } /* got_response */

  //
  //! Execute method that also checks if we have the targets in our routing table.
  this_program execute() {
    if (rt->nodes_by_hash[target_hash]) {
      object res = rt->nodes_by_hash[target_hash];
      result += ({ res });
      result_count = sizeof(result);
    }

    return ::execute();
  }
} /* FindNode */


//
//
//! The GetPeers class is used to initiate queries to the DHT where
//! peers for a hash is desired. Upon completion, the done_db will be
//! called with the GetPeers instance as the first argument and the
//! done_cb_args as the following arguments.
//!
//! The done_cb function is expected to examine the results array to
//! find out what the result of the query actually contains. For this
//! query, the result array will be an array with two elements; the
//! first one containing any responses from nodes that knows of peers
//! and the second one will contain the closest nodes to the target
//! hash that we saw during the query.
class GetPeers {
  inherit DHTOperation;

  //! Sorted array of the closest K nodes we've seen in the query.
  array closest_nodes = ({});

  //! Override the default number of desired results
  int desired_results = 50;

  //
  //! Implementation of @[DHTNode:generate_query] that returns a
  //! get_peers query.
  protected mapping generate_query() {
    mapping q = ([
      "q" : "get_peers",
      "a" : ([
	"info_hash" : target_hash,
	"id"        : my_node_id,
      ]),
    ]);

    return q;
  }

  //
  //! Add the closest nodes we know of and call the callback.
  void done() {
    array tmp = ({});
    foreach(closest_nodes - ({ 0 }), Node n) {
      mapping m = n->node_info();
      tmp += ({
	m + ([ "values" : ({}) ]),
      });
    }

    // Nest the result into the format we want.
    result = ({ result, tmp });

    // Call the default done callback to continue processing.
    ::done();
  }

  //
  // Response handler for get_peers responses.
  protected void got_response(mapping resp) {
    if (resp->y == "e") {
      // We are just notified of an error
      return;
    }

    // The node that responded should be in the list of seen nodes, so
    // let's look it up and assign the token to it if we got one.
    Node responder = seen_nodes[resp->r->id];
    if (responder && resp->r->token)
      responder->token = resp->r->token;

    // Keep a list of the 8 closest nodes.
    closest_nodes += ({ responder });
    closest_nodes = Array.uniq(closest_nodes);
    closest_nodes =
      Array.sort_array(closest_nodes - ({ 0 }),
		       lambda(object a, object b) {
			 int ad = distance_exp(a->node_id, target_hash);
			 int bd = distance_exp(b->node_id, target_hash);
			 return ad < bd;
		       })[0..7];

    // First process additional nodes.
    if (resp->r->nodes) {
      // If we get nodes, the peer didn't know the target hash so it
      // just gave us it's routing bucket for the target hash.
      array compact_node_info = resp->r->nodes/26 - ({ "", 0 });
      foreach(compact_node_info, string cnf) {
	array node_info = array_sscanf(cnf, "%20s%4c%2c");
	node_info[1] = NetUtils.ip_to_string(node_info[1],0);
	Node n = Node(@node_info);
	add_node_to_query(n);
      }
    }

    // Then see if we have results in this response.
    if (resp->r->values) {
      if (stringp(resp->r->values)) {
	// Apparently we may get a string for a single host result.
	resp->r->values = ({ resp->r->values });
      }

      array tmp = ({});
      foreach(resp->r->values, string cnf) {
	array node_info = array_sscanf(cnf, "%4c%2c");
	if (sizeof(node_info) != 2) {
	  continue;
	}
	node_info[0] = NetUtils.ip_to_string(node_info[0],0);
	tmp += ({ Node(UNDEFINED, @node_info) });
      }

      // Add the results to our results array.
      result += ({
	([
	  "token"   : resp->r->token,
	  "node_id" : resp->r->id,
	  "values"  : tmp,
	])
      });

      // And increase the result count accordingly
      result_count += sizeof(tmp);
    }
  } /* got_response() */
} /* GetPeers */


//
//! Calculate the distance between two hashes using XOR. Fails unless
//! both h1 and h2 are strings of the same length.
string distance(string h1, string h2) {
  if (!h1 || !h2 || (sizeof(h1) != sizeof(h2))) {
    return UNDEFINED;
  }

  return h1 ^ h2;
}

//
//! Calculate the order of magnitude of the distance. Basically count leading zeros...
int distance_exp(string h1, string h2) {
  if (!h1 || !h2 || (sizeof(h1) != sizeof(h2))) {
    return UNDEFINED;
  }

  foreach(h1; int i; int c) {
    int t = c ^ h2[i];
    if (!t) continue;

    int bit = (19-i) * 8;
    for(int b=7; b >= 0; b--) {
      if (t >= (1<<b)) {
	return bit+b;
      }
    }

    return bit;
  }

  return 0;
}


//
//
// Low-level query handling
//

//
//! Keep track of callbacks by transaction id
mapping(string:mixed) callbacks_by_txid = ([]);
mapping(string:mixed) request_timeouts = ([]);

//
//! Generates suitable transaction ids making sure to not collide with
//! existing ones.
string generate_txid() {
  string ret;
  do {
    ret = random_string(4);
  } while(callbacks_by_txid[ret]);
  return ret;
}

//
//! Do the actual sending part... No timeout handling or etherwise -
//! just send the message.
//!
//! Returns the TXID if the message was sent successfully and
//! UNDEFINED if not.
int send_dht_request(string to, int dstport, mapping data) {
  int res = port->send(to, dstport, .Bencoding.encode(data));
  if (res != -1) {
    return data->t;
  }
  return UNDEFINED;
}

//
//! Sends a DHT query and calls the callback when a response is
//! recieved or when a timeout occurs.
string send_dht_query(string to, int dstport, mapping data, void|function response_callback, mixed ... args) {
  if (!is_running) {
    return UNDEFINED;
  }

  data = data | ([]);

  // Force a query
  if (!data->y) data->y = "q";

  // Generate a txid for this transaction and save the callback
  data->t = generate_txid();
  callbacks_by_txid[data->t] = ({ response_callback, args });
  // Set up a timeout handler so that we can clear out requests that
  // go unanswered for a prolonged period of time. Callers should set
  // up their own timeouts with shorter timers for the actual expected
  // timeout.
  request_timeouts[data->t] = call_out(read_timeout, 60, data->t);

  int res = send_dht_request(to, dstport, data);
  if (!res) {
    // Request failed
    m_delete(callbacks_by_txid, data->t);
    remove_call_out(m_delete(request_timeouts, data->t));
    functionp(response_callback) &&
      response_callback(([
			  "t" : data->t,
			  "y" : "e",
			  "e" : DHT_SERVER_ERROR,
			]),
			@args);


  }
  return data->t;
}

//
//! Sends a PING to a remote port and calls the callback cb if we get
//! a response.
string send_ping(string ip, int port, function(mapping, string, int:void) cb) {
  string txid = send_dht_query(ip, port,
			       ([
				 "q" : "ping",
				 "a" : ([
				   "id" : my_node_id,
				 ]),
			       ]),
			       cb);
  return txid;
}



//
//! Internal timeout method to ensure we don't wait forever on
//! responses from nodes that are no longer available.
//!
//! Note: The timeout is not propagated to higher levels, so callers
//! cannot rely on the send_dht_query() callback to propagate this.
protected void read_timeout(string txid) {
  m_delete(request_timeouts, txid);
  [function cb, array args] = m_delete(callbacks_by_txid, txid);
  functionp(cb) && cb(([
			"t" : txid,
			"r" : "e",
			"e" : 205, // <= We define 205 as timeout
		      ]),
		      @args);
}


//
//! Mapping of query names to handlers. Allows for extensions to be
//! implemented.
mapping(string:function) command_handlers = ([]);

//
//! Called when we recieve a datagram on the UDP port we are listening
//! to.
protected void read_callback(mapping(string:int|string) datagram, mixed ... extra) {
  // Datagram will contain data, ip and port.
  mapping decoded_data = .Bencoding.decode(datagram->data);

  if (!mappingp(decoded_data)) {
    // We ignore data we can't decode.
    return;
  }

  string packet_type = decoded_data->y || "";
  string txid = decoded_data->t;

  switch(packet_type) {
  case "r": // response
    do {
      if (!txid) {
      // Not our transaction - ignore.
	return;
      }

      // Disable timeout for this request
      mixed timeout = m_delete(request_timeouts, txid);
      timeout && remove_call_out(timeout);

      // FIXME: We should probably allow for the callback to be called when an error is returned too...
      if (decoded_data->y == "r") {
	// Check if we have the peer in our routing table. If we do, we
	// register activity on the peer.
	DHTNode peer = rt->nodes_by_hash[decoded_data->r && decoded_data->r->id];
	if (peer) {
	  peer->activity();
	}

	// This is a response to something we did so call the callback if we have one.
	if (has_index(callbacks_by_txid, txid)) {
	  [function cb, array args] = m_delete(callbacks_by_txid, decoded_data->t);
	  functionp(cb) && cb(decoded_data, @args);
	}
      }
    } while(0);
    break;

  case "q": // Query
    do {
      function cmd_handler = command_handlers[decoded_data->q];
      if (functionp(cmd_handler)) {
	cmd_handler(decoded_data, datagram->ip, datagram->port);
      } else {
	handle_unknown_method(decoded_data, datagram->ip, datagram->port);
      }
    } while(0);
    break;

  default:
    // This is an unknown packet type - ignore.
  }
}


//
//
// Token handling for DHT announces
//
// Conforms to BEP005.
//

// Tokens we may expect to see in the future,
mapping(string:Token) active_tokens = ([]);
mapping(string:Token) tokens_by_endpoint = ([]); // Helper lookup table
int token_lifetime = 600; // Time a token remains valid without any announces on it. (BEP005)

//
// Internal variables for token seeding
protected int last_token_seed_time;
protected string token_seed;

class Token {
  string token; // Token in string form
  protected mixed timeout;
  protected string endpoint;
  protected string ip;
  protected int port;

  //
  //! Invalidates a token if it exists,
  Token invalidate() {
    this_program t = m_delete(active_tokens, token);
    m_delete(tokens_by_endpoint, endpoint);
    timeout && remove_call_out(timeout);

    return this;
  }

  //
  //! Refreshes a token's lifetime to the configured token_lifetime value.
  //! Note: If called on an old token, this will violate recommendations in BEP005.
  this_program refresh() {
    timeout && remove_call_out(timeout);
    timeout = call_out(invalidate, token_lifetime);
    return this;
  }

  //
  //! Checks if a token is valid for a given node. This is done by
  //! checking that we handed out the token to the IP/port that attempts
  //! to use it.
  int valid_for(string ip, int port) {
    // Verify that all parameters match
    if (ip != this_program::ip) {
      return 0;
    }

    if (port != this_program::port) {
      return 0;
    }

    return 1;
  }


  //! Generate a new token for the given IP/port. Optionally don't
  //! apply SHA1 to the token for debugging purposes.
  protected void create(string ip, int port, void|int dont_sha1) {
    endpoint = sprintf("%s:%d", ip, port);
    string tmp_ip = NetUtils.string_to_ip(ip) || ip;
    token = tmp_ip + sprintf("%2c", port) + token_seed;

    // For debugging purposes, we allow SHA1 hashing of the token to be
    // disabled. Should not be used for production.
    if (!dont_sha1) {
      token = Crypto.SHA1.hash(token);
    }

    this_program::ip = ip;
    this_program::port = port;

    active_tokens[token] = refresh();
    tokens_by_endpoint[endpoint] = this;
  }

  // In sprintf, we turn the token into hex for readability.
  protected string _sprintf(int to) {
    switch(to) {
    case 's':
      return String.string2hex(token);
    case 'O': // Fallthru
    default:
      return sprintf("Token(%s)", endpoint);
    }
  }

  // Casting tokens to strings probably means we want to use the token
  // value, so let's ensure that works.
  protected mixed cast(string to) {
    switch(to) {
    case "string":
      return token;
    }
  }
} /* class Token */

//
//! Returns a token object for the given ip/port if one exists.
Token token_by_endpoint(string ip, int port) {
  string endpoint = sprintf("%s:%d", ip, port);
  return tokens_by_endpoint[endpoint];
}

//
//! Generate a token for a specific node. If it fails to generate a
//! unique token for this request, it will return UNDEFINED, otherwise
//! a string token is returned.
Token generate_token_for(string ip, int port, void|int dont_sha1) {
  string endpoint = sprintf("%s:%d", ip, port);

  // As per recommendation in BEP005, we change the seed every 5
  // minutes.
  if (time() - last_token_seed_time > 300) {
    last_token_seed_time = time();
    token_seed = random_string(20);
  }

  Token token;

  // Check if we can reuse an existing token...
  if (token = token_by_endpoint(ip, port)) {
    Token t = active_tokens[(string)token];

    if (!t) {
      // Apparently something was off and we had no real token...
      m_delete(tokens_by_endpoint, endpoint);
    } else {
      // Refresh lifetime for the token
      // NOTE: This is in violation of BEP005.
      return token->refresh();
    }
  }

  token = Token(ip, port, dont_sha1);
  dwerror("Generated token %O\n", token);
  return token->refresh();
}


//
//
// Peer handling for hashes

//
//! Peers we know of.
mapping(string:mapping) peers_by_hash = ([]);

 //! Maximum number hashes we store data for in this node.
int PEER_TABLE_MAX_SIZE = 2000;

//! Maximum number of peers per hash that we store.
int MAX_PEERS_PER_HASH = 100;

//
//! Information about the peeer for a hash
protected class Peer {
  inherit Node;

  // Announcing node endpoint
  string source_ip;
  int source_port;
}

//
//! Insert a peer for a hash in our table of known peers. Ignores the
//! request if the given node is already a peer for the given hash.
void add_peer_for_hash(Peer n, string hash, string ip, int port) {
  mapping peers_for_hash = peers_by_hash[hash];

  if (peers_for_hash) {
    // Make sure we remove the old entry if there is one - port may have changed etc.
    peers_for_hash->peers =
      filter(peers_for_hash->peers || ({}),
	     lambda(Peer p) {
	       // Apparently node may be 0, so we
	       // need to guard against that.
	       int res = p &&
		 p->source_ip != ip &&
		 p->source_port != port;
	       return res;
	     });
  }

  // Trim the peers by hash until it is capable of accepting this new hash.
  while (PEER_TABLE_MAX_SIZE > 0 && sizeof(peers_by_hash) >= PEER_TABLE_MAX_SIZE) {
    // Remove the hash with the lowest number of peers
    array hashes = indices(peers_by_hash);
    array pc = sizeof(values(peers_by_hash)->peers[*]);
    sort(pc, hashes);
    if (hashes[0] != hash) {
      m_delete(peers_by_hash, hashes[0]);
    } else {
      // We are adding a peer to the hash with the lowest number of peers.
      break;
    }
  }

  // Add an entry for the hash if needed.
  if (!peers_for_hash) {
    peers_for_hash = peers_by_hash[hash] = ([
      "added" : time(),
      "peers" : ({})
    ]);
  }

  // Only add the peer to the hash if we have room compared to our
  // configured max number.
  if (sizeof(peers_for_hash) < MAX_PEERS_PER_HASH) {
    peers_for_hash->peers += ({ n });
    peers_for_hash->updated = time();
  }
}

//
//
// Handlers for incoming DHT operations
//

//
//! Returns peers to the requestor or our closest nodes if we don't
//! know of any peers.
void handle_get_peers(mapping data, string ip, int port) {
  string txid = data->t;
  mapping d = data->a;
  string target_hash = d->info_hash;
  mapping resp = ([
    "t" : txid,
    "y" : "r",
    "r" : ([
      "id" : my_node_id,
      "token" : (string)generate_token_for(ip, port)
    ]),
  ]);

  dwerror("DTH instance %x was asked for peers of %x\n",
          my_node_id, target_hash);

  if (mapping known_peers = peers_by_hash[target_hash]) {
    // We know of peers for this hash, so return what we know.
    array peers = known_peers->peers->endpoint_compact();
    resp->r->values = peers*"";
  } else {
    // We don't know of peers for this hash, so return nodes we know
    // that *MIGHT* know more...
    object /* Bucket */ b = rt->bucket_for(target_hash);
    array(DHTNode) tmp = b->nodes;
    array(string) nodes = tmp->compact_node_info();
    resp->r->nodes = nodes * "";
  }

  // Fire off the response
  send_dht_request(ip, port, resp);
}

//
//! Handles other peers announcing to us for safekeeping.
void handle_announce_peer(mapping data, string ip, int port) {
  string token_string = data->a->token;
  int announce_port;

  dwerror("Got announce request from %s:%d\n", ip, port);
  // Check if we are to use the sending port or not (which is what
  // gets passed into the port parameter)
  if (!data->a->implied_port) {
    announce_port = data->a->port;
  } else {
    announce_port = port;
  }

  Token token = active_tokens[token_string];

  if (!token || !token->valid_for(ip, port)) {
    // Request with invalid token -- ignore it.
    dwerror("Token is invalid: %O (%q)\n", token, token_string);
    dwerror("%O\n", String.string2hex(indices(active_tokens)[*]));
    return;
  }

  // Add the sender to our list of peers for this hash.
  Peer peer = Peer(UNDEFINED, ip, announce_port);
  peer->source_ip = ip;
  peer->source_port = port;
  add_peer_for_hash(peer, data->a->info_hash, ip, port);
  dwerror("Peer %s for %x added in DHT instance %x\n",
         peer, data->a->info_hash, my_node_id);
}

//
//! Handles request for nodes closer to a hash.
void handle_find_node(mapping data, string ip, int req_port) {
  string target_hash = data->a->target;
  mapping resp = ([
    "t" : data->t,
    "y" : "r",
    "r" : ([
      "id" : my_node_id,
    ]),
  ]);

  // Our node id would not be in our own routing table, so special
  // care is taken if we get a request for ourselves.
  // This shouldn't happen except in very rare cases though.
  if (target_hash == my_node_id) {
    string addr = port->query_address();
    string tmp = (string)array_sscanf("%d.%d.%d.%d %d", addr);
    if (sizeof(tmp) != 6) {
      // Handle error
      dwerror("Failed to create compact node info for myself!\n");
      return;
    }

    resp->r->nodes = my_node_id + tmp;
  } else {
    object b = rt->bucket_for(target_hash);
    array(DHTNode) tmp = b->nodes;
    array(string) nodes = tmp->compact_node_info();
    resp->r->nodes = nodes * "";
  }

  stats->find_node++;
  send_dht_request(ip, req_port, resp);
}

//
//! Handles PONG responses to incoming PINGs.
void handle_ping(mapping data, string ip, int port) {
  stats->ping++;
  send_dht_request(ip, port,
		   ([
		     "t" : data->t,
		     "y" : "r",
		     "r" : ([ "id" : my_node_id ]),
		   ]));
}


//
//! Responds to unknown methods. Currently does nothing at all.
void handle_unknown_method(mapping data, string ip, int port) {
}

//
//! Gateway into the routing table for now. This is in preparation for BEP045.
DHTNode add_node(string|DHTNode n, void|string ip, void|int port) {
  return rt->add_node(n, ip, port);
}

// Tracks announce requests in progress.
protected multiset(DHTOperation) announces_in_progress = (<>);

// FIXME: Handle the response and allow it to propagate back to the
// application?
//
//! This is the internal announce callback - it is called for each node
//! that we should send an announcement to.
protected void announce_to(string peer_ip, int peer_port, string token,
			   string info_hash, int announced_port,
			   void|int implied_port) {

  mapping(string:string|int) data = ([
    "implied_port" : !!implied_port,
    "info_hash"    : info_hash,
    "port"         : announced_port,
    "token"        : token,
    "id"           : my_node_id,
  ]);

  string txid =
    send_dht_query(peer_ip, peer_port, ([
		     "q" : "announce_peer",
		     "a" : data,
		   ]),
		   lambda(mapping r) {
		   });
    dwerror("Sent announce to %s:%d TXID: %x\n", peer_ip, peer_port, txid);
}

//
// This callback is called by the GetPeers instance after it is
// done. This callback does the initial processing of the response and
// then calls @[announce_to] to send the actual announcement.
protected void announce_to_nodes(object q, int announced_port) {
  if (!q->result || !sizeof(q->result)) {
    dwerror("announce_to_nodes got empty result from get_peers - unable to announce!\n");
    return;
  }

  // q->result is an array(mapping) of peers per node. Each mapping
  // represents a node and the peers from that node are in the values
  // key.
  foreach(q->result[1], mapping p) {
    if (!p->token || !p->ip || !p->port) {
      dwerror("No token for for %s:%d - skipping\n", p->ip||"<unknown ip>", p->port);
      continue;
    }
    announce_to(p->ip, p->port, p->token,
		q->target_hash, announced_port);
  }

  // We are done, so remove the announcement object from the in-progress multiset.
  announces_in_progress[q] = 0;
}

//
//! Announce a hash to the world on the given port.
//!
//! This is done by executing a get_peers request to the DHT and then
//! announcing to the K closest nodes to that hash.
void announce(string hash, int port) {
  object q = GetPeers(hash, announce_to_nodes, port);
  announces_in_progress[q] = 1;
  q->execute();
}

//
//! Returns some basic information about this DHT instance.
mapping info() {
  mapping ret = ([
    "node_id" : String.string2hex(my_node_id),
    "buckets" : sizeof(rt->buckets),
  ]);

  return ret;
}

//
//! Sets the node id of this instance. This has implications for the
//! routing table, so we need to reinitialize it if this happens...
void set_node_id(string my_node_id) {
  this_program::my_node_id = my_node_id;

  // Create a new routing table and add all nodes from the old one to
  // it.
  Routingtable rt = Routingtable(my_node_id);
  Routingtable old_rt = this_program::rt;
  if (old_rt) {
    rt->copy_from(old_rt);
  }

  this_program::rt = rt;
  destruct(old_rt);
}

//
//! Allows outsiders to examine the node id of this instance.
string get_node_id() { return my_node_id; };

//
//! Start up the DHT instance.
void start(int port, void|string bind_address) {
  if (is_running) return 0;
  is_running = 1;
  this_program::port = Stdio.UDP();
  this_program::port->bind(port, bind_address, 1);
  this_program::port->set_nonblocking(read_callback);
}

//
//! Stop the DHT instance. When the instance is stopped, it will close
//! its port and therefore stop responding to queries. It will not
//! destroy the DHT routing table or other states that could be reused
//! if the DHT instance is started back up.
void stop() {
  if (!is_running) return;
  is_running = 0;
  port->set_blocking();
  port->close();
  destruct(port);
}

//
//! Generate a 160 bit node id.
string generate_node_id() {
  return Crypto.SHA1.hash(random_string(20));
}

//
//! Create a new DHT instance with an optional predefined node id.
protected void create(void|string my_node_id) {
  set_node_id(my_node_id || generate_node_id());
  rt = Routingtable(my_node_id);

  // Setup default command handlers.
  command_handlers = ([
    "ping"          : handle_ping,
    "get_peers"     : handle_get_peers,
    "find_node"     : handle_find_node,
    "announce_peer" : handle_announce_peer,
  ]);
}

protected void _destruct() {
  // Do some cleanup before we die...
  stop();
}
