//!
//! Mix-in for @[Connection] for implementing handshake packet
//! fragmentation and defragmentation for DTLS.
//!

//! Number of passed sequence numbers to keep track of.
//! @rfc{4347:4.1.2.5@}:
//!   A minimum window size of 32 MUST be supported, but a window size
//!   of 64 is preferred and SHOULD be employed as the default. Another
//!   window size (larger than the minimum) MAY be chosen by the receiver.
private constant window_size = 64;

//! Bitmask representing sequence numbers for accepted
//! received packets in the interval
//! [@expr{next_seq_num-window_size@}..@expr{next_seq_num-2@}].
//! @note
//!   The packet with seqence number @expr{next_seq_num-1@} is
//!   implicitly known to have been received.
int sequence_mask = 0;

//! Check whether @[num] is a valid seqence number for a new packet.
int valid_seq_nump(int num)
{
  if (num < next_seq_num-(window_size)) return 0;
  if (num >= next_seq_num) return 1;
  if (num == next_seq_num-1) return 0;
  return !(sequence_mask & (1<<(num + window_size - next_seq_num)));
}

//! Mark seqence number @[num] as seen and accepted.
//!
//! This will cause @[valid_seq_nump()] to return @expr{0@} for it
//! if it shows up again.
void mark_seq_num(int num)
{
  if (num < next_seq_num-(window_size)) return;
  if (num == next_seq_num-1) return;
  if (num < next_seq_num) {
    sequence_mask |= (1<<(num + window_size - next_seq_num));
  } else {
    int delta = 1 + num - next_seq_num;
    if (delta < window_size) {
      sequence_mask >>= delta;
      sequence_mask |= 1 << (window_size-(delta + 1));
    } else {
      sequence_mask = 0;
    }
    next_seq_num = num + 1;
  }
}

void print_sequence_set()
{
  werror("([ ");
  if (next_seq_num > 0) {
    int mask = 1;
    for (int i = next_seq_num-(window_size); i+1 < next_seq_num; i++) {
      if ((sequence_mask & mask) && (i >= 0)) {
	werror("%d, ", i);
      }
      mask <<= 1;
    }
    werror("%d ", next_seq_num - 1);
  }
  werror("])\n");
}

string(8bit)|int(-1..1) got_data(string(8bit) data);

extern State current_read_state;

//! Receive handler for DTLS.
//!
//! This function keeps track of received packets, discards duplicates
//! and defragments and serializes handshake packets.
//!
//! @seealso
//!   @[Connection()->got_data()]
string(8bit)|int(-1..1) got_packet(string(8bit) data)
{
  int content_type, version, seq_num;

  if ((sscanf(data, "%1c&2c&8c", content_type, version, seq_no) < 3) ||
      ((version & 0xff00) != 0xfe00) ||
      !valid_seq_nump(seq_num)) {
    // Ignore packet.
    return "";
  }
  mark_seq_num(seq_num);

  if (content_type != PACKET_handshake) return got_data(data);

  if (read_state)
  {
  }
}
