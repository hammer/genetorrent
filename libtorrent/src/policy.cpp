/*

Copyright (c) 2003, Arvid Norberg
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in
      the documentation and/or other materials provided with the distribution.
    * Neither the name of the author nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.

*/

#include "libtorrent/pch.hpp"

#ifdef _MSC_VER
#pragma warning(push, 1)
#endif

#include <boost/bind.hpp>
#include <boost/utility.hpp>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include "libtorrent/peer_connection.hpp"
#include "libtorrent/web_peer_connection.hpp"
#include "libtorrent/policy.hpp"
#include "libtorrent/torrent.hpp"
#include "libtorrent/socket.hpp"
#include "libtorrent/alert_types.hpp"
#include "libtorrent/invariant_check.hpp"
#include "libtorrent/time.hpp"
#include "libtorrent/aux_/session_impl.hpp"
#include "libtorrent/piece_picker.hpp"
#include "libtorrent/broadcast_socket.hpp"
#include "libtorrent/peer_info.hpp"
#include "libtorrent/random.hpp"
#include "libtorrent/extensions.hpp"

#ifdef TORRENT_DEBUG
#include "libtorrent/bt_peer_connection.hpp"
#endif

namespace
{
	using namespace libtorrent;

	struct match_peer_endpoint
	{
		match_peer_endpoint(tcp::endpoint const& ep)
			: m_ep(ep)
		{}

		bool operator()(policy::peer const* p) const
		{ return p->address() == m_ep.address() && p->port == m_ep.port(); }

		tcp::endpoint const& m_ep;
	};

#if defined TORRENT_DEBUG || TORRENT_RELEASE_ASSERTS
	struct match_peer_connection
	{
		match_peer_connection(peer_connection const& c) : m_conn(c) {}

		bool operator()(policy::peer const* p) const
		{ return p->connection == &m_conn; }

		peer_connection const& m_conn;
	};

	struct match_peer_connection_or_endpoint
	{
		match_peer_connection_or_endpoint(peer_connection const& c) : m_conn(c) {}

		bool operator()(policy::peer const* p) const
		{
			return p->connection == &m_conn
				|| (p->ip() == m_conn.remote()
					&& p->connectable);
		}

		peer_connection const& m_conn;
	};
#endif

}

namespace libtorrent
{
	// returns the rank of a peer's source. We have an affinity
	// to connecting to peers with higher rank. This is to avoid
	// problems when our peer list is diluted by stale peers from
	// the resume data for instance
	int source_rank(int source_bitmask)
	{
		int ret = 0;
		if (source_bitmask & peer_info::tracker) ret |= 1 << 5;
		if (source_bitmask & peer_info::lsd) ret |= 1 << 4;
		if (source_bitmask & peer_info::dht) ret |= 1 << 3;
		if (source_bitmask & peer_info::pex) ret |= 1 << 2;
		return ret;
	}

	// the case where ignore_peer is motivated is if two peers
	// have only one piece that we don't have, and it's the
	// same piece for both peers. Then they might get into an
	// infinite loop, fighting to request the same blocks.
	void request_a_block(torrent& t, peer_connection& c)
	{
		if (t.is_seed()) return;
		if (c.no_download()) return;
		if (t.upload_mode()) return;
		if (c.is_disconnecting()) return;

		// don't request pieces before we have the metadata
		if (!t.valid_metadata()) return;

		// don't request pieces before the peer is properly
		// initialized after we have the metadata
		if (!t.are_files_checked()) return;

		TORRENT_ASSERT(t.valid_metadata());
		TORRENT_ASSERT(c.peer_info_struct() != 0 || c.type() != peer_connection::bittorrent_connection);
		int num_requests = c.desired_queue_size()
			- (int)c.download_queue().size()
			- (int)c.request_queue().size();

#ifdef TORRENT_VERBOSE_LOGGING
		c.peer_log("*** PIECE_PICKER [ req: %d engame: %d ]", num_requests, c.endgame());
#endif
		TORRENT_ASSERT(c.desired_queue_size() > 0);
		// if our request queue is already full, we
		// don't have to make any new requests yet
		if (num_requests <= 0) return;

		piece_picker& p = t.picker();
		std::vector<piece_block> interesting_pieces;
		interesting_pieces.reserve(100);

		int prefer_whole_pieces = c.prefer_whole_pieces();

		if (prefer_whole_pieces == 0)
		{
			prefer_whole_pieces = c.statistics().download_payload_rate()
				* t.settings().whole_pieces_threshold
				> t.torrent_file().piece_length() ? 1 : 0;
		}
	
		// if we prefer whole pieces, the piece picker will pick at least
		// the number of blocks we want, but it will try to make the picked
		// blocks be from whole pieces, possibly by returning more blocks
		// than we requested.
#ifdef TORRENT_DEBUG
		error_code ec;
		TORRENT_ASSERT(c.remote() == c.get_socket()->remote_endpoint(ec) || ec);
#endif

		aux::session_impl& ses = t.session();

		std::vector<pending_block> const& dq = c.download_queue();
		std::vector<pending_block> const& rq = c.request_queue();

		std::vector<int> const& suggested = c.suggested_pieces();
		bitfield const* bits = &c.get_bitfield();
		bitfield fast_mask;
		
		if (c.has_peer_choked())
		{
			// if we are choked we can only pick pieces from the
			// allowed fast set. The allowed fast set is sorted
			// in ascending priority order
			std::vector<int> const& allowed_fast = c.allowed_fast();

			// build a bitmask with only the allowed pieces in it
			fast_mask.resize(c.get_bitfield().size(), false);
			for (std::vector<int>::const_iterator i = allowed_fast.begin()
				, end(allowed_fast.end()); i != end; ++i)
				if ((*bits)[*i]) fast_mask.set_bit(*i);
			bits = &fast_mask;
		}

		piece_picker::piece_state_t state;
		peer_connection::peer_speed_t speed = c.peer_speed();
		if (speed == peer_connection::fast) state = piece_picker::fast;
		else if (speed == peer_connection::medium) state = piece_picker::medium;
		else state = piece_picker::slow;

		// picks the interesting pieces from this peer
		// the integer is the number of pieces that
		// should be guaranteed to be available for download
		// (if num_requests is too big, too many pieces are
		// picked and cpu-time is wasted)
		// the last argument is if we should prefer whole pieces
		// for this peer. If we're downloading one piece in 20 seconds
		// then use this mode.
		p.pick_pieces(*bits, interesting_pieces
			, num_requests, prefer_whole_pieces, c.peer_info_struct()
			, state, c.picker_options(), suggested, t.num_peers());

#ifdef TORRENT_VERBOSE_LOGGING
		c.peer_log("*** PIECE_PICKER [ prefer_whole: %d picked: %d ]"
			, prefer_whole_pieces, int(interesting_pieces.size()));
#endif

		// if the number of pieces we have + the number of pieces
		// we're requesting from is less than the number of pieces
		// in the torrent, there are still some unrequested pieces
		// and we're not strictly speaking in end-game mode yet
		// also, if we already have at least one outstanding
		// request, we shouldn't pick any busy pieces 
		bool dont_pick_busy_blocks = (ses.m_settings.strict_end_game_mode 
				&& p.num_have() + int(p.get_download_queue().size()) 
				    < t.torrent_file().num_pieces() - p.num_filtered()) 
				|| dq.size() + rq.size() > 0;

		// this is filled with an interesting piece
		// that some other peer is currently downloading
		piece_block busy_block = piece_block::invalid;

		for (std::vector<piece_block>::iterator i = interesting_pieces.begin(); 
				 i != interesting_pieces.end(); ++i)
		{
#ifdef TORRENT_STATS
			++ses.m_piece_picker_blocks;
#endif

			if (prefer_whole_pieces == 0 && num_requests <= 0) break;

			int num_block_requests = p.num_peers(*i);
			if (num_block_requests > 0)
			{
				// have we picked enough pieces?
				if (num_requests <= 0) break;

				// this block is busy. This means all the following blocks
				// in the interesting_pieces list are busy as well, we might
				// as well just exit the loop
				if (dont_pick_busy_blocks) break;

				TORRENT_ASSERT(p.num_peers(*i) > 0);
				busy_block = *i;
				continue;
			}

			TORRENT_ASSERT(p.num_peers(*i) == 0);

			// don't request pieces we already have in our request queue
			// This happens when pieces time out or the peer sends us
			// pieces we didn't request. Those aren't marked in the
			// piece picker, but we still keep track of them in the
			// download queue
			if (std::find_if(dq.begin(), dq.end(), has_block(*i)) != dq.end()
				|| std::find_if(rq.begin(), rq.end(), has_block(*i)) != rq.end())
			{
#ifdef TORRENT_DEBUG
				std::vector<pending_block>::const_iterator j
					= std::find_if(dq.begin(), dq.end(), has_block(*i));
				if (j != dq.end()) TORRENT_ASSERT(j->timed_out || j->not_wanted);
#endif
				continue;
			}

			// ok, we found a piece that's not being downloaded
			// by somebody else. request it from this peer
			// and return
			if (!c.add_request(*i, 0)) continue;
			TORRENT_ASSERT(p.num_peers(*i) == 1);
			TORRENT_ASSERT(p.is_requested(*i));
			num_requests--;
		}

		// we have picked as many blocks as we should
		// we're done!
		if (num_requests <= 0)
		{
			// since we could pick as many blocks as we
			// requested without having to resort to picking
			// busy ones, we're not in end-game mode
			c.set_endgame(false);
			return;
		}

		// we did not pick as many pieces as we wanted, because
		// there aren't enough. This means we're in end-game mode
		// as long as we have at least one request outstanding,
		// we shouldn't pick another piece
		// if we are attempting to download 'allowed' pieces
		// and can't find any, that doesn't count as end-game
		if (!c.has_peer_choked())
			c.set_endgame(true);
	
		// if we don't have any potential busy blocks to request
		// or if we already have outstanding requests, don't
		// pick a busy piece
		if (busy_block == piece_block::invalid 
				|| dq.size() + rq.size() > 0)
		{
			return;
		}

#ifdef TORRENT_STATS
		++ses.m_end_game_piece_picker_blocks;
#endif

#ifdef TORRENT_DEBUG
		piece_picker::downloading_piece st;
		p.piece_info(busy_block.piece_index, st);
		TORRENT_ASSERT(st.requested + st.finished + st.writing
			== p.blocks_in_piece(busy_block.piece_index));
#endif
		TORRENT_ASSERT(p.is_requested(busy_block));
		TORRENT_ASSERT(!p.is_downloaded(busy_block));
		TORRENT_ASSERT(!p.is_finished(busy_block));
		TORRENT_ASSERT(p.num_peers(busy_block) > 0);

		c.add_request(busy_block, peer_connection::req_busy);
	}

	policy::policy(torrent* t)
		: m_torrent(t)
		, m_round_robin(0)
		, m_num_connect_candidates(0)
		, m_num_seeds(0)
		, m_finished(false)
	{ TORRENT_ASSERT(t); }

	// disconnects and removes all peers that are now filtered
	void policy::ip_filter_updated()
	{
		INVARIANT_CHECK;

		aux::session_impl& ses = m_torrent->session();
		if (!m_torrent->apply_ip_filter()) return;

		for (iterator i = m_peers.begin(); i != m_peers.end();)
		{
			if ((ses.m_ip_filter.access((*i)->address()) & ip_filter::blocked) == 0)
			{
				++i;
				continue;
			}
		
			if (ses.m_alerts.should_post<peer_blocked_alert>())
				ses.m_alerts.post_alert(peer_blocked_alert(m_torrent->get_handle(), (*i)->address()));

			int current = i - m_peers.begin();
			TORRENT_ASSERT(current >= 0);
			TORRENT_ASSERT(m_peers.size() > 0);
			TORRENT_ASSERT(i != m_peers.end());

			if ((*i)->connection)
			{
				// disconnecting the peer here may also delete the
				// peer_info_struct. If that is the case, just continue
				int count = m_peers.size();
				peer_connection* p = (*i)->connection;
				
				p->disconnect(errors::banned_by_ip_filter);
				// what *i refers to has changed, i.e. cur was deleted
				if (m_peers.size() < count)
				{
					i = m_peers.begin() + current;
					continue;
				}
				TORRENT_ASSERT((*i)->connection == 0
					|| (*i)->connection->peer_info_struct() == 0);
			}

			erase_peer(i);
			i = m_peers.begin() + current;
		}
	}

	void policy::erase_peer(policy::peer* p)
	{
		INVARIANT_CHECK;

		std::pair<iterator, iterator> range = find_peers(p->address());
		iterator iter = std::find_if(range.first, range.second, match_peer_endpoint(p->ip()));
		if (iter == range.second) return;
		erase_peer(iter);
	}

	// any peer that is erased from m_peers will be
	// erased through this function. This way we can make
	// sure that any references to the peer are removed
	// as well, such as in the piece picker.
	void policy::erase_peer(iterator i)
	{
		INVARIANT_CHECK;
		TORRENT_ASSERT(i != m_peers.end());

		if (m_torrent->has_picker())
			m_torrent->picker().clear_peer(*i);
		if ((*i)->seed) --m_num_seeds;
		if (is_connect_candidate(**i, m_finished))
		{
			TORRENT_ASSERT(m_num_connect_candidates > 0);
			--m_num_connect_candidates;
		}
		TORRENT_ASSERT(m_num_connect_candidates < int(m_peers.size()));
		if (m_round_robin > i - m_peers.begin()) --m_round_robin;
		if (m_round_robin >= int(m_peers.size())) m_round_robin = 0;

#ifdef TORRENT_DEBUG
		TORRENT_ASSERT((*i)->in_use);
		(*i)->in_use = false;
#endif

#if TORRENT_USE_IPV6
		if ((*i)->is_v6_addr)
		{
			TORRENT_ASSERT(m_torrent->session().m_ipv6_peer_pool.is_from(
				static_cast<ipv6_peer*>(*i)));
			m_torrent->session().m_ipv6_peer_pool.destroy(
				static_cast<ipv6_peer*>(*i));
		}
		else
#endif
#if TORRENT_USE_I2P
		if ((*i)->is_i2p_addr)
		{
			TORRENT_ASSERT(m_torrent->session().m_i2p_peer_pool.is_from(
				static_cast<i2p_peer*>(*i)));
			m_torrent->session().m_i2p_peer_pool.destroy(
				static_cast<i2p_peer*>(*i));
		}
		else
#endif
		{
			TORRENT_ASSERT(m_torrent->session().m_ipv4_peer_pool.is_from(
				static_cast<ipv4_peer*>(*i)));
			m_torrent->session().m_ipv4_peer_pool.destroy(
				static_cast<ipv4_peer*>(*i));
		}
		m_peers.erase(i);
	}

	bool policy::should_erase_immediately(peer const& p) const
	{
		return p.source == peer_info::resume_data
			&& p.failcount > 0
			&& !p.banned;
	}

	bool policy::is_erase_candidate(peer const& pe, bool finished) const
	{
		return pe.connection == 0
			&& pe.last_connected != 0
			&& !pe.banned
			&& !is_connect_candidate(pe, m_finished);
	}

	void policy::erase_peers()
	{
		INVARIANT_CHECK;

		int max_peerlist_size = m_torrent->is_paused()
			? m_torrent->settings().max_paused_peerlist_size
			: m_torrent->settings().max_peerlist_size;

		if (max_peerlist_size == 0 || m_peers.empty()) return;

		int erase_candidate = -1;

		TORRENT_ASSERT(m_finished == m_torrent->is_finished());

		int round_robin = random() % m_peers.size();

		for (int iterations = (std::min)(int(m_peers.size()), 300);
			iterations > 0; --iterations)
		{
			if (int(m_peers.size()) < max_peerlist_size * 0.95)
				break;

			if (round_robin == int(m_peers.size())) round_robin = 0;

			peer& pe = *m_peers[round_robin];
			int current = round_robin;

			{
				if (is_erase_candidate(pe, m_finished)
					&& (erase_candidate == -1
						|| !compare_peer_erase(*m_peers[erase_candidate], pe)))
				{
					if (should_erase_immediately(pe))
					{
						if (erase_candidate > current) --erase_candidate;
						TORRENT_ASSERT(current >= 0 && current < int(m_peers.size()));
						--round_robin;
						erase_peer(m_peers.begin() + current);
					}
					else
					{
						erase_candidate = current;
					}
				}
			}

			++round_robin;
		}
		
		if (erase_candidate > -1)
		{
			TORRENT_ASSERT(erase_candidate >= 0 && erase_candidate < int(m_peers.size()));
			erase_peer(m_peers.begin() + erase_candidate);
		}
	}

	void policy::ban_peer(policy::peer* p)
	{
		INVARIANT_CHECK;

		if (is_connect_candidate(*p, m_finished))
			--m_num_connect_candidates;

#ifdef TORRENT_STATS
		aux::session_impl& ses = m_torrent->session();
		++ses.m_num_banned_peers;
#endif

		p->banned = true;
		TORRENT_ASSERT(!is_connect_candidate(*p, m_finished));
	}

	void policy::set_connection(policy::peer* p, peer_connection* c)
	{
		INVARIANT_CHECK;

		TORRENT_ASSERT(c);

		const bool was_conn_cand = is_connect_candidate(*p, m_finished);
		p->connection = c;
		if (was_conn_cand) --m_num_connect_candidates;
	}

	void policy::set_failcount(policy::peer* p, int f)
	{
		INVARIANT_CHECK;

		const bool was_conn_cand = is_connect_candidate(*p, m_finished);
		p->failcount = f;
		if (was_conn_cand != is_connect_candidate(*p, m_finished))
		{
			if (was_conn_cand) --m_num_connect_candidates;
			else ++m_num_connect_candidates;
		}
	}

	bool policy::is_connect_candidate(peer const& p, bool finished) const
	{
		if (p.connection
			|| p.banned
			|| !p.connectable
			|| (p.seed && finished)
			|| int(p.failcount) >= m_torrent->settings().max_failcount)
			return false;
		
		aux::session_impl const& ses = m_torrent->session();
		if (ses.m_port_filter.access(p.port) & port_filter::blocked)
			return false;

		// only apply this to peers we've only heard
		// about from the DHT
		if (ses.m_settings.no_connect_privileged_ports
			&& p.port < 1024
			&& p.source == peer_info::dht)
			return false;

		return true;
	}

	policy::iterator policy::find_connect_candidate(int session_time)
	{
		INVARIANT_CHECK;

		int candidate = -1;
		int erase_candidate = -1;

		TORRENT_ASSERT(m_finished == m_torrent->is_finished());

		int min_reconnect_time = m_torrent->settings().min_reconnect_time;
		address external_ip = m_torrent->session().external_address();

		// don't bias any particular peers when seeding
		if (m_finished || external_ip == address())
		{
			// set external_ip to a random value, to
			// radomize which peers we prefer
			address_v4::bytes_type bytes;
			std::generate(bytes.begin(), bytes.end(), &random);
			external_ip = address_v4(bytes);
		}

		if (m_round_robin >= int(m_peers.size())) m_round_robin = 0;

#ifndef TORRENT_DISABLE_DHT
		bool pinged = false;
#endif

		int max_peerlist_size = m_torrent->is_paused()
			?m_torrent->settings().max_paused_peerlist_size
			:m_torrent->settings().max_peerlist_size;

		for (int iterations = (std::min)(int(m_peers.size()), 300);
			iterations > 0; --iterations)
		{
			if (m_round_robin >= int(m_peers.size())) m_round_robin = 0;

			peer& pe = *m_peers[m_round_robin];
			int current = m_round_robin;

#ifndef TORRENT_DISABLE_DHT
			// try to send a DHT ping to this peer
			// as well, to figure out if it supports
			// DHT (uTorrent and BitComet doesn't
			// advertise support)
			if (!pinged && !pe.added_to_dht)
			{
				udp::endpoint node(pe.address(), pe.port);
				m_torrent->session().add_dht_node(node);
				pe.added_to_dht = true;
				pinged = true;
			}
#endif
			// if the number of peers is growing large
			// we need to start weeding.

			if (int(m_peers.size()) >= max_peerlist_size * 0.95
				&& max_peerlist_size > 0)
			{
				if (is_erase_candidate(pe, m_finished)
					&& (erase_candidate == -1
						|| !compare_peer_erase(*m_peers[erase_candidate], pe)))
				{
					if (should_erase_immediately(pe))
					{
						if (erase_candidate > current) --erase_candidate;
						if (candidate > current) --candidate;
						--m_round_robin;
						erase_peer(m_peers.begin() + current);
					}
					else
					{
						erase_candidate = current;
					}
				}
			}

			++m_round_robin;

			if (!is_connect_candidate(pe, m_finished)) continue;

			// compare peer returns true if lhs is better than rhs. In this
			// case, it returns true if the current candidate is better than
			// pe, which is the peer m_round_robin points to. If it is, just
			// keep looking.
			if (candidate != -1
				&& compare_peer(*m_peers[candidate], pe, external_ip)) continue;

			if (pe.last_connected
				&& session_time - pe.last_connected <
				(int(pe.failcount) + 1) * min_reconnect_time)
				continue;

			candidate = current;
		}
		
		if (erase_candidate > -1)
		{
			if (candidate > erase_candidate) --candidate;
			erase_peer(m_peers.begin() + erase_candidate);
		}

#if defined TORRENT_LOGGING || defined TORRENT_VERBOSE_LOGGING 
		if (candidate != -1)
		{
			(*m_torrent->session().m_logger) << time_now_string()
				<< " *** FOUND CONNECTION CANDIDATE ["
				" ip: " << m_peers[candidate]->ip() <<
				" d: " << cidr_distance(external_ip, m_peers[candidate]->address()) <<
				" external: " << external_ip <<
				" t: " << (session_time - m_peers[candidate]->last_connected) <<
				" ]\n";
		}
#endif

		if (candidate == -1) return m_peers.end();
		return m_peers.begin() + candidate;
	}

	void policy::pulse()
	{
		INVARIANT_CHECK;

		erase_peers();
	}

	bool policy::new_connection(peer_connection& c, int session_time)
	{
		TORRENT_ASSERT(!c.is_local());

		INVARIANT_CHECK;

		// if the connection comes from the tracker,
		// it's probably just a NAT-check. Ignore the
		// num connections constraint then.

		// TODO: only allow _one_ connection to use this
		// override at a time
		error_code ec;
		TORRENT_ASSERT(c.remote() == c.get_socket()->remote_endpoint(ec) || ec);
		TORRENT_ASSERT(!m_torrent->is_paused());

		aux::session_impl& ses = m_torrent->session();
		
		if (m_torrent->num_peers() >= m_torrent->max_connections()
			&& ses.num_connections() >= ses.settings().connections_limit
			&& c.remote().address() != m_torrent->current_tracker().address())
		{
			c.disconnect(errors::too_many_connections);
			return false;
		}

#if defined TORRENT_VERBOSE_LOGGING || defined TORRENT_LOGGING
		if (c.remote().address() == m_torrent->current_tracker().address())
		{
			m_torrent->debug_log("overriding connection limit for tracker NAT-check");
		}
#endif

		iterator iter;
		peer* i = 0;

		bool found = false;
		if (m_torrent->settings().allow_multiple_connections_per_ip)
		{
			tcp::endpoint remote = c.remote();
			std::pair<iterator, iterator> range = find_peers(remote.address());
			iter = std::find_if(range.first, range.second, match_peer_endpoint(remote));
	
			if (iter != range.second) found = true;
		}
		else
		{
			iter = std::lower_bound(
				m_peers.begin(), m_peers.end()
				, c.remote().address(), peer_address_compare()
			);

			if (iter != m_peers.end() && (*iter)->address() == c.remote().address()) found = true;
		}

		// make sure the iterator we got is properly sorted relative
		// to the connection's address
//		TORRENT_ASSERT(m_peers.empty()
//			|| (iter == m_peers.end() && (*(iter-1))->address() < c.remote().address())
//			|| (iter != m_peers.end() && c.remote().address() < (*iter)->address())
//			|| (iter != m_peers.end() && iter != m_peers.begin() && (*(iter-1))->address() < c.remote().address()));

		if (found)
		{
			i = *iter;
			TORRENT_ASSERT(i->connection != &c);

			if (i->banned)
			{
				c.disconnect(errors::peer_banned);
				return false;
			}

			if (i->connection != 0)
			{
				boost::shared_ptr<socket_type> other_socket
					= i->connection->get_socket();
				boost::shared_ptr<socket_type> this_socket
					= c.get_socket();

				error_code ec1;
				error_code ec2;
				bool self_connection =
					other_socket->remote_endpoint(ec2) == this_socket->local_endpoint(ec1)
					|| other_socket->local_endpoint(ec2) == this_socket->remote_endpoint(ec1);

				if (ec1)
				{
					c.disconnect(ec1);
					return false;
				}

				if (self_connection)
				{
					c.disconnect(errors::self_connection, 1);
					i->connection->disconnect(errors::self_connection, 1);
					TORRENT_ASSERT(i->connection == 0);
					return false;
				}

				TORRENT_ASSERT(i->connection != &c);
				// the new connection is a local (outgoing) connection
				// or the current one is already connected
				if (ec2)
				{
					i->connection->disconnect(ec2);
					TORRENT_ASSERT(i->connection == 0);
				}
				else if (!i->connection->is_connecting() || c.is_local())
				{
					c.disconnect(errors::duplicate_peer_id);
					return false;
				}
				else
				{
#if defined TORRENT_VERBOSE_LOGGING || defined TORRENT_LOGGING || defined TORRENT_MINIMAL_LOGGING
					m_torrent->debug_log("duplicate connection. existing connection"
					" is connecting and this connection is incoming. closing existing "
					"connection in favour of this one");
#endif
					i->connection->disconnect(errors::duplicate_peer_id);
					TORRENT_ASSERT(i->connection == 0);
				}
			}

			if (is_connect_candidate(*i, m_finished))
			{
				m_num_connect_candidates--;
				TORRENT_ASSERT(m_num_connect_candidates >= 0);
				if (m_num_connect_candidates < 0) m_num_connect_candidates = 0;
			}
		}
		else
		{
			// we don't have any info about this peer.
			// add a new entry
			error_code ec;
			TORRENT_ASSERT(c.remote() == c.get_socket()->remote_endpoint(ec) || ec);

			if (int(m_peers.size()) >= m_torrent->settings().max_peerlist_size)
			{
				// this may invalidate our iterator!
				erase_peers();
				if (int(m_peers.size()) >= m_torrent->settings().max_peerlist_size)
				{
					c.disconnect(errors::too_many_connections);
					return false;
				}
				// restore it
				iter = std::lower_bound(
					m_peers.begin(), m_peers.end()
					, c.remote().address(), peer_address_compare()
				);
			}

#if TORRENT_USE_IPV6
			bool is_v6 = c.remote().address().is_v6();
#endif
			peer* p =
#if TORRENT_USE_IPV6
				is_v6 ? (peer*)m_torrent->session().m_ipv6_peer_pool.malloc() :
#endif
				(peer*)m_torrent->session().m_ipv4_peer_pool.malloc();
			if (p == 0) return false;

//			TORRENT_ASSERT(p->in_use == false);

#if TORRENT_USE_IPV6
			if (is_v6)
				m_torrent->session().m_ipv6_peer_pool.set_next_size(500);
			else
#endif
				m_torrent->session().m_ipv4_peer_pool.set_next_size(500);

#if TORRENT_USE_IPV6
			if (is_v6)
				new (p) ipv6_peer(c.remote(), false, 0);
			else
#endif
				new (p) ipv4_peer(c.remote(), false, 0);

#ifdef TORRENT_DEBUG
			p->in_use = true;
#endif

			iter = m_peers.insert(iter, p);

			if (m_round_robin >= iter - m_peers.begin()) ++m_round_robin;

			i = *iter;
#ifndef TORRENT_DISABLE_GEO_IP
			int as = ses.as_for_ip(c.remote().address());
#ifdef TORRENT_DEBUG
			i->inet_as_num = as;
#endif
			i->inet_as = ses.lookup_as(as);
#endif
			i->source = peer_info::incoming;
		}
	
		TORRENT_ASSERT(i);
		c.set_peer_info(i);
		TORRENT_ASSERT(i->connection == 0);
		c.add_stat(size_type(i->prev_amount_download) << 10, size_type(i->prev_amount_upload) << 10);

		// restore transfer rate limits
		int rate_limit;
		rate_limit = i->upload_rate_limit;
		if (rate_limit) c.set_upload_limit(rate_limit);
		rate_limit = i->download_rate_limit;
		if (rate_limit) c.set_download_limit(rate_limit);

		i->prev_amount_download = 0;
		i->prev_amount_upload = 0;
		i->connection = &c;
		TORRENT_ASSERT(i->connection);
		if (!c.fast_reconnect())
			i->last_connected = session_time;

		// this cannot be a connect candidate anymore, since i->connection is set
		TORRENT_ASSERT(!is_connect_candidate(*i, m_finished));
		TORRENT_ASSERT(has_connection(&c));
		return true;
	}

	bool policy::update_peer_port(int port, policy::peer* p, int src)
	{
		TORRENT_ASSERT(p != 0);
		TORRENT_ASSERT(p->connection);

		INVARIANT_CHECK;

		if (p->port == port) return true;

		if (m_torrent->settings().allow_multiple_connections_per_ip)
		{
			tcp::endpoint remote(p->address(), port);
			std::pair<iterator, iterator> range = find_peers(remote.address());
			iterator i = std::find_if(range.first, range.second
				, match_peer_endpoint(remote));
			if (i != range.second)
			{
				policy::peer& pp = **i;
				if (pp.connection)
				{
					bool was_conn_cand = is_connect_candidate(pp, m_finished);
					// if we already have an entry with this
					// new endpoint, disconnect this one
					pp.connectable = true;
					pp.source |= src;
					if (!was_conn_cand && is_connect_candidate(pp, m_finished))
						++m_num_connect_candidates;

					if (pp.connection->is_connecting())
					{
						// Accept this peer, and disconnect the one
						// that is still trying to connect. If the one
						// trying to connect can never complete the
						// connection (due to a firewall dropping the
						// connect), then it only serves to block
						// other connections until it times out.
						pp.connection->disconnect(errors::duplicate_peer_id);
					}
					else
					{
						p->connection->disconnect(errors::duplicate_peer_id);
						erase_peer(p);
						return false;
					}
				}
				erase_peer(i);
			}
		}
#ifdef TORRENT_DEBUG
		else
		{
			std::pair<iterator, iterator> range = find_peers(p->address());
			TORRENT_ASSERT(range.second - range.first == 1);
		}
#endif

		bool was_conn_cand = is_connect_candidate(*p, m_finished);
		p->port = port;
		p->source |= src;
		p->connectable = true;

		if (was_conn_cand != is_connect_candidate(*p, m_finished))
		{
			m_num_connect_candidates += was_conn_cand ? -1 : 1;
			TORRENT_ASSERT(m_num_connect_candidates >= 0);
			if (m_num_connect_candidates < 0) m_num_connect_candidates = 0;
		}
		return true;
	}

	// it's important that we don't dereference
	// p here, since it is allowed to be a dangling
	// pointer. see smart_ban.cpp
	bool policy::has_peer(policy::peer const* p) const
	{
		// find p in m_peers
		for (const_iterator i = m_peers.begin()
			, end(m_peers.end()); i != end; ++i)
		{
			if (*i == p) return true;
		}
		return false;
	}

	void policy::set_seed(policy::peer* p, bool s)
	{
		if (p == 0) return;
		if (p->seed == s) return;
		bool was_conn_cand = is_connect_candidate(*p, m_finished);
		p->seed = s;
		if (was_conn_cand && !is_connect_candidate(*p, m_finished))
		{
			--m_num_connect_candidates;
			TORRENT_ASSERT(m_num_connect_candidates >= 0);
			if (m_num_connect_candidates < 0) m_num_connect_candidates = 0;
		}

		if (s) ++m_num_seeds;
		else --m_num_seeds;
		TORRENT_ASSERT(m_num_seeds >= 0);
		TORRENT_ASSERT(m_num_seeds <= int(m_peers.size()));
	}

	bool policy::insert_peer(policy::peer* p, iterator iter, int flags)
	{
		TORRENT_ASSERT(p);

		int max_peerlist_size = m_torrent->is_paused()
			?m_torrent->settings().max_paused_peerlist_size
			:m_torrent->settings().max_peerlist_size;

		if (max_peerlist_size
			&& int(m_peers.size()) >= max_peerlist_size)
		{
			if (p->source == peer_info::resume_data) return false;

			erase_peers();
			if (int(m_peers.size()) >= max_peerlist_size)
				return 0;

			// since some peers were removed, we need to
			// update the iterator to make it valid again
#if TORRENT_USE_I2P
			if (p->is_i2p_addr)
			{
				iter = std::lower_bound(
					m_peers.begin(), m_peers.end()
					, p->dest(), peer_address_compare());
			}
			else
#endif
			iter = std::lower_bound(
				m_peers.begin(), m_peers.end()
				, p->address(), peer_address_compare());
		}

		iter = m_peers.insert(iter, p);

		if (m_round_robin >= iter - m_peers.begin()) ++m_round_robin;

#ifndef TORRENT_DISABLE_ENCRYPTION
		if (flags & 0x01) p->pe_support = true;
#endif
		if (flags & 0x02)
		{
			p->seed = true;
			++m_num_seeds;
		}
		if (flags & 0x04)
			p->supports_utp = true;
		if (flags & 0x08)
			p->supports_holepunch = true;

#ifndef TORRENT_DISABLE_GEO_IP
		int as = m_torrent->session().as_for_ip(p->address());
#ifdef TORRENT_DEBUG
		p->inet_as_num = as;
#endif
		p->inet_as = m_torrent->session().lookup_as(as);
#endif
		if (is_connect_candidate(*p, m_finished))
			++m_num_connect_candidates;

		return true;
	}

	void policy::update_peer(policy::peer* p, int src, int flags
		, tcp::endpoint const& remote, char const* destination)
	{
		bool was_conn_cand = is_connect_candidate(*p, m_finished);

		p->connectable = true;

		TORRENT_ASSERT(p->address() == remote.address());
		p->port = remote.port();
		p->source |= src;
			
		// if this peer has failed before, decrease the
		// counter to allow it another try, since somebody
		// else is appearantly able to connect to it
		// only trust this if it comes from the tracker
		if (p->failcount > 0 && src == peer_info::tracker)
			--p->failcount;

		// if we're connected to this peer
		// we already know if it's a seed or not
		// so we don't have to trust this source
		if ((flags & 0x02) && !p->connection)
		{
			if (!p->seed) ++m_num_seeds;
			p->seed = true;
		}
		if (flags & 0x04)
			p->supports_utp = true;
		if (flags & 0x08)
			p->supports_holepunch = true;

#if defined TORRENT_VERBOSE_LOGGING || defined TORRENT_LOGGING
		if (p->connection)
		{
			// this means we're already connected
			// to this peer. don't connect to
			// it again.

			error_code ec;
			char hex_pid[41];
			to_hex((char*)&p->connection->pid()[0], 20, hex_pid);
			char msg[200];
			snprintf(msg, 200, "already connected to peer: %s %s"
				, print_endpoint(remote).c_str(), hex_pid);
			m_torrent->debug_log(msg);

			TORRENT_ASSERT(p->connection->associated_torrent().lock().get() == m_torrent);
		}
#endif

		if (was_conn_cand != is_connect_candidate(*p, m_finished))
		{
			m_num_connect_candidates += was_conn_cand ? -1 : 1;
			if (m_num_connect_candidates < 0) m_num_connect_candidates = 0;
		}
	}

#if TORRENT_USE_I2P
	policy::peer* policy::add_i2p_peer(char const* destination, int src, char flags)
	{
		INVARIANT_CHECK;
	
		bool found = false;
		iterator iter = std::lower_bound(
			m_peers.begin(), m_peers.end()
			, destination, peer_address_compare()
		);

		if (iter != m_peers.end() && strcmp((*iter)->dest(), destination) == 0)
			found = true;

		peer* p = 0;

		if (!found)
		{
			// we don't have any info about this peer.
			// add a new entry
			p = (peer*)m_torrent->session().m_i2p_peer_pool.malloc();
			if (p == 0) return 0;
			m_torrent->session().m_i2p_peer_pool.set_next_size(500);
			new (p) i2p_peer(destination, true, src);

#ifdef TORRENT_DEBUG
			p->in_use = true;
#endif

			if (!insert_peer(p, iter, flags))
			{
#ifdef TORRENT_DEBUG
				p->in_use = false;
#endif

				m_torrent->session().m_i2p_peer_pool.free((i2p_peer*)p);
				return 0;
			}
		}
		else
		{
			p = *iter;
			update_peer(p, src, flags, tcp::endpoint(), destination);
		}
		return p;
	}
#endif // TORRENT_USE_I2P

	policy::peer* policy::add_peer(tcp::endpoint const& remote, peer_id const& pid
		, int src, char flags)
	{
		INVARIANT_CHECK;

		// just ignore the obviously invalid entries
		if (remote.address() == address() || remote.port() == 0)
			return 0;

		aux::session_impl& ses = m_torrent->session();

		// if this is an i2p torrent, and we don't allow mixed mode
		// no regular peers should ever be added!
		if (!ses.m_settings.allow_i2p_mixed && m_torrent->torrent_file().is_i2p())
		{
			if (ses.m_alerts.should_post<peer_blocked_alert>())
				ses.m_alerts.post_alert(peer_blocked_alert(m_torrent->get_handle(), remote.address()));
			return 0;
		}

		port_filter const& pf = ses.m_port_filter;
		if (pf.access(remote.port()) & port_filter::blocked)
		{
			if (ses.m_alerts.should_post<peer_blocked_alert>())
				ses.m_alerts.post_alert(peer_blocked_alert(m_torrent->get_handle(), remote.address()));
#ifndef TORRENT_DISABLE_EXTENSIONS
			m_torrent->notify_extension_add_peer(remote, src, torrent_plugin::filtered);
#endif
			return 0;
		}

		if (ses.m_settings.no_connect_privileged_ports && remote.port() < 1024)
		{
			if (ses.m_alerts.should_post<peer_blocked_alert>())
				ses.m_alerts.post_alert(peer_blocked_alert(m_torrent->get_handle(), remote.address()));
#ifndef TORRENT_DISABLE_EXTENSIONS
			m_torrent->notify_extension_add_peer(remote, src, torrent_plugin::filtered);
#endif
			return 0;
		}

		// if the IP is blocked, don't add it
		if (m_torrent->apply_ip_filter()
			&& (ses.m_ip_filter.access(remote.address()) & ip_filter::blocked))
		{
			if (ses.m_alerts.should_post<peer_blocked_alert>())
				ses.m_alerts.post_alert(peer_blocked_alert(m_torrent->get_handle(), remote.address()));
#ifndef TORRENT_DISABLE_EXTENSIONS
			m_torrent->notify_extension_add_peer(remote, src, torrent_plugin::filtered);
#endif
			return 0;
		}

		iterator iter;
		peer* p = 0;

		bool found = false;
		if (m_torrent->settings().allow_multiple_connections_per_ip)
		{
			std::pair<iterator, iterator> range = find_peers(remote.address());
			iter = std::find_if(range.first, range.second, match_peer_endpoint(remote));
			if (iter != range.second) found = true;
		}
		else
		{
			iter = std::lower_bound(
				m_peers.begin(), m_peers.end()
				, remote.address(), peer_address_compare()
			);

			if (iter != m_peers.end() && (*iter)->address() == remote.address()) found = true;
		}

		if (!found)
		{
			// we don't have any info about this peer.
			// add a new entry

#if TORRENT_USE_IPV6
			bool is_v6 = remote.address().is_v6();
#endif
			p =
#if TORRENT_USE_IPV6
				is_v6 ? (peer*)m_torrent->session().m_ipv6_peer_pool.malloc() :
#endif
				(peer*)m_torrent->session().m_ipv4_peer_pool.malloc();
			if (p == 0) return 0;
#if TORRENT_USE_IPV6
			if (is_v6)
				m_torrent->session().m_ipv6_peer_pool.set_next_size(500);
			else
#endif
				m_torrent->session().m_ipv4_peer_pool.set_next_size(500);

#if TORRENT_USE_IPV6
			if (is_v6)
				new (p) ipv6_peer(remote, true, src);
			else
#endif
				new (p) ipv4_peer(remote, true, src);

#ifdef TORRENT_DEBUG
			p->in_use = true;
#endif

			if (!insert_peer(p, iter, flags))
			{
#ifdef TORRENT_DEBUG
				p->in_use = false;
#endif
#if TORRENT_USE_IPV6
				if (is_v6) m_torrent->session().m_ipv6_peer_pool.free((ipv6_peer*)p);
				else
#endif
				m_torrent->session().m_ipv4_peer_pool.free((ipv4_peer*)p);
				return 0;
			}
#ifndef TORRENT_DISABLE_EXTENSIONS
			m_torrent->notify_extension_add_peer(remote, src, torrent_plugin::first_time);
#endif
		}
		else
		{
			p = *iter;
			update_peer(p, src, flags, remote, 0);
#ifndef TORRENT_DISABLE_EXTENSIONS
			m_torrent->notify_extension_add_peer(remote, src, 0);
#endif
		}

		return p;
	}

	bool policy::connect_one_peer(int session_time)
	{
		INVARIANT_CHECK;

		TORRENT_ASSERT(m_torrent->want_more_peers());
		
		iterator i = find_connect_candidate(session_time);
		if (i == m_peers.end()) return false;
		peer& p = **i;

		TORRENT_ASSERT(!p.banned);
		TORRENT_ASSERT(!p.connection);
		TORRENT_ASSERT(p.connectable);

		TORRENT_ASSERT(m_finished == m_torrent->is_finished());
		TORRENT_ASSERT(is_connect_candidate(p, m_finished));
		if (!m_torrent->connect_to_peer(&p))
		{
			// failcount is a 5 bit value
			const bool was_conn_cand = is_connect_candidate(p, m_finished);
			if (p.failcount < 31) ++p.failcount;
			if (was_conn_cand && !is_connect_candidate(p, m_finished))
				--m_num_connect_candidates;
			return false;
		}
		TORRENT_ASSERT(p.connection);
		TORRENT_ASSERT(!is_connect_candidate(p, m_finished));
		return true;
	}

	// this is called whenever a peer connection is closed
	void policy::connection_closed(const peer_connection& c, int session_time)
	{
		INVARIANT_CHECK;

		peer* p = c.peer_info_struct();

		TORRENT_ASSERT((std::find_if(
			m_peers.begin()
			, m_peers.end()
			, match_peer_connection(c))
			!= m_peers.end()) == (p != 0));
		
		// if we couldn't find the connection in our list, just ignore it.
		if (p == 0) return;

		TORRENT_ASSERT(p->connection == &c);
		TORRENT_ASSERT(!is_connect_candidate(*p, m_finished));

		// save transfer rate limits
		p->upload_rate_limit = c.upload_limit();
		p->download_rate_limit = c.download_limit();

		p->connection = 0;
		p->optimistically_unchoked = false;

		// if fast reconnect is true, we won't
		// update the timestamp, and it will remain
		// the time when we initiated the connection.
		if (!c.fast_reconnect())
			p->last_connected = session_time;

		if (c.failed())
		{
			// failcount is a 5 bit value
			if (p->failcount < 31) ++p->failcount;
		}

		if (is_connect_candidate(*p, m_finished))
			++m_num_connect_candidates;

		// if we're already a seed, it's not as important
		// to keep all the possibly stale peers
		// if we're not a seed, but we have too many peers
		// start weeding the ones we only know from resume
		// data first
		// at this point it may be tempting to erase peers
		// from the peer list, but keep in mind that we might
		// have gotten to this point through new_connection, just
		// disconnecting an old peer, relying on this policy::peer
		// to still exist when we get back there, to assign the new
		// peer connection pointer to it. The peer list must
		// be left intact.

		// if we allow multiple connections per IP, and this peer
		// was incoming and it never advertised its listen
		// port, we don't really know which peer it was. In order
		// to avoid adding one entry for every single connection
		// the peer makes to us, don't save this entry
		if (m_torrent->settings().allow_multiple_connections_per_ip
			&& !p->connectable)
		{
			erase_peer(p);
		}
	}

	void policy::peer_is_interesting(peer_connection& c)
	{
		INVARIANT_CHECK;

		// no peer should be interesting if we're finished
		TORRENT_ASSERT(!m_torrent->is_finished());

		if (c.in_handshake()) return;
		c.send_interested();
		if (c.has_peer_choked()
			&& c.allowed_fast().empty())
			return;
		request_a_block(*m_torrent, c);
		c.send_block_requests();
	}

	void policy::recalculate_connect_candidates()
	{
		INVARIANT_CHECK;

		const bool is_finished = m_torrent->is_finished();
		if (is_finished == m_finished) return;

		m_num_connect_candidates = 0;
		m_finished = is_finished;
		for (const_iterator i = m_peers.begin();
			i != m_peers.end(); ++i)
		{
			m_num_connect_candidates += is_connect_candidate(**i, m_finished);
		}
	}

#if defined TORRENT_DEBUG || TORRENT_RELEASE_ASSERTS
	bool policy::has_connection(const peer_connection* c)
	{
		INVARIANT_CHECK;

		TORRENT_ASSERT(c);
		error_code ec;
		if (c->remote() != c->get_socket()->remote_endpoint(ec) && !ec)
		{
			fprintf(stderr, "c->remote: %s\nc->get_socket()->remote_endpoint: %s\n"
				, print_endpoint(c->remote()).c_str()
				, print_endpoint(c->get_socket()->remote_endpoint(ec)).c_str());
			TORRENT_ASSERT(false);
		}

		return std::find_if(
			m_peers.begin()
			, m_peers.end()
			, match_peer_connection_or_endpoint(*c)) != m_peers.end();
	}
#endif

#ifdef TORRENT_DEBUG
	void policy::check_invariant() const
	{
		TORRENT_ASSERT(m_num_connect_candidates >= 0);
		TORRENT_ASSERT(m_num_connect_candidates <= int(m_peers.size()));
		if (m_torrent->is_aborted()) return;

#ifdef TORRENT_EXPENSIVE_INVARIANT_CHECKS
		int connected_peers = 0;

		int total_connections = 0;
		int nonempty_connections = 0;
		int connect_candidates = 0;

		std::set<tcp::endpoint> unique_test;
		const_iterator prev = m_peers.end();
		for (const_iterator i = m_peers.begin();
			i != m_peers.end(); ++i)
		{
			if (prev != m_peers.end()) ++prev;
			if (i == m_peers.begin() + 1) prev = m_peers.begin();
			if (prev != m_peers.end())
			{
				if (m_torrent->settings().allow_multiple_connections_per_ip)
					TORRENT_ASSERT(!((*i)->address() < (*prev)->address()));
				else
					TORRENT_ASSERT((*prev)->address() < (*i)->address());
			}
			peer const& p = **i;
			if (is_connect_candidate(p, m_finished)) ++connect_candidates;
#ifndef TORRENT_DISABLE_GEO_IP
			TORRENT_ASSERT(p.inet_as == 0 || p.inet_as->first == p.inet_as_num);
#endif
			if (!m_torrent->settings().allow_multiple_connections_per_ip)
			{
				std::pair<const_iterator, const_iterator> range = find_peers(p.address());
				TORRENT_ASSERT(range.second - range.first == 1);
			}
			else
			{
				TORRENT_ASSERT(unique_test.count(p.ip()) == 0);
				unique_test.insert(p.ip());
//				TORRENT_ASSERT(p.connection == 0 || p.ip() == p.connection->remote());
			}
			++total_connections;
			if (!p.connection)
			{
				continue;
			}
			if (p.optimistically_unchoked)
			{
				TORRENT_ASSERT(p.connection);
				TORRENT_ASSERT(!p.connection->is_choked());
			}
			TORRENT_ASSERT(p.connection->peer_info_struct() == 0
				|| p.connection->peer_info_struct() == &p);
			++nonempty_connections;
			if (!p.connection->is_disconnecting())
				++connected_peers;
		}

		TORRENT_ASSERT(m_num_connect_candidates == connect_candidates);

		int num_torrent_peers = 0;
		for (torrent::const_peer_iterator i = m_torrent->begin();
			i != m_torrent->end(); ++i)
		{
			if ((*i)->is_disconnecting()) continue;
			// ignore web_peer_connections since they are not managed
			// by the policy class
			if ((*i)->type() != peer_connection::bittorrent_connection) continue;
			++num_torrent_peers;
		}

		if (m_torrent->has_picker())
		{
			piece_picker& p = m_torrent->picker();
			std::vector<piece_picker::downloading_piece> downloaders = p.get_download_queue();

			std::set<void*> peer_set;
			std::vector<void*> peers;
			for (std::vector<piece_picker::downloading_piece>::iterator i = downloaders.begin()
				, end(downloaders.end()); i != end; ++i)
			{
				p.get_downloaders(peers, i->index);
				std::copy(peers.begin(), peers.end()
					, std::insert_iterator<std::set<void*> >(peer_set, peer_set.begin()));
			}
			
			for (std::set<void*>::iterator i = peer_set.begin()
				, end(peer_set.end()); i != end; ++i)
			{
				policy::peer* p = static_cast<policy::peer*>(*i);
				if (p == 0) continue;
				if (p->connection == 0) continue;
				TORRENT_ASSERT(std::find_if(m_peers.begin(), m_peers.end()
					, match_peer_connection_or_endpoint(*p->connection)) != m_peers.end());
			}
		}
#endif // TORRENT_EXPENSIVE_INVARIANT_CHECKS

		// this invariant is a bit complicated.
		// the usual case should be that connected_peers
		// == num_torrent_peers. But when there's an incoming
		// connection, it will first be added to the policy
		// and then be added to the torrent.
		// When there's an outgoing connection, it will first
		// be added to the torrent and then to the policy.
		// that's why the two second cases are in there.
/*
		TORRENT_ASSERT(connected_peers == num_torrent_peers
			|| (connected_peers == num_torrent_peers + 1
				&& connected_peers > 0)
			|| (connected_peers + 1 == num_torrent_peers
				&& num_torrent_peers > 0));
*/
	}
#endif // TORRENT_DEBUG

	policy::peer::peer(boost::uint16_t port, bool conn, int src)
		: prev_amount_upload(0)
		, prev_amount_download(0)
		, connection(0)
#ifndef TORRENT_DISABLE_GEO_IP
		, inet_as(0)
#endif
		, last_optimistically_unchoked(0)
		, last_connected(0)
		, port(port)
		, upload_rate_limit(0)
		, download_rate_limit(0)
		, hashfails(0)
		, failcount(0)
		, connectable(conn)
		, optimistically_unchoked(false)
		, seed(false)
		, fast_reconnects(0)
		, trust_points(0)
		, source(src)
#ifndef TORRENT_DISABLE_ENCRYPTION
		, pe_support(true)
#endif
#if TORRENT_USE_IPV6
		, is_v6_addr(false)
#endif
#if TORRENT_USE_I2P
		, is_i2p_addr(false)
#endif
		, on_parole(false)
		, banned(false)
#ifndef TORRENT_DISABLE_DHT
		, added_to_dht(false)
#endif
		, supports_utp(true) // assume peers support utp
		, confirmed_supports_utp(false)
		, supports_holepunch(false)
#ifdef TORRENT_DEBUG
		, in_use(false)
#endif
	{
		TORRENT_ASSERT((src & 0xff) == src);
	}

	size_type policy::peer::total_download() const
	{
		if (connection != 0)
		{
			TORRENT_ASSERT(prev_amount_download == 0);
			return connection->statistics().total_payload_download();
		}
		else
		{
			return size_type(prev_amount_download) << 10;
		}
	}

	size_type policy::peer::total_upload() const
	{
		if (connection != 0)
		{
			TORRENT_ASSERT(prev_amount_upload == 0);
			return connection->statistics().total_payload_upload();
		}
		else
		{
			return size_type(prev_amount_upload) << 10;
		}
	}

	// this returns true if lhs is a better erase candidate than rhs
	bool policy::compare_peer_erase(policy::peer const& lhs, policy::peer const& rhs) const
	{
		bool lhs_resume_data_source = lhs.source == peer_info::resume_data;
		bool rhs_resume_data_source = rhs.source == peer_info::resume_data;

		// prefer to drop peers whose only source is resume data
		if (lhs_resume_data_source != rhs_resume_data_source)
			return lhs_resume_data_source > rhs_resume_data_source;

		if (lhs.connectable != rhs.connectable)
			return lhs.connectable < rhs.connectable;

		// prefer peers with higher failcount
		return lhs.failcount > rhs.failcount;
	}

	// this returns true if lhs is a better connect candidate than rhs
	bool policy::compare_peer(policy::peer const& lhs, policy::peer const& rhs
		, address const& external_ip) const
	{
		// prefer peers with lower failcount
		if (lhs.failcount != rhs.failcount)
			return lhs.failcount < rhs.failcount;

		// Local peers should always be tried first
		bool lhs_local = is_local(lhs.address());
		bool rhs_local = is_local(rhs.address());
		if (lhs_local != rhs_local) return lhs_local > rhs_local;

		if (lhs.last_connected != rhs.last_connected)
			return lhs.last_connected < rhs.last_connected;

		int lhs_rank = source_rank(lhs.source);
		int rhs_rank = source_rank(rhs.source);
		if (lhs_rank != rhs_rank) return lhs_rank > rhs_rank;

#ifndef TORRENT_DISABLE_GEO_IP
		// don't bias fast peers when seeding
		if (!m_finished && m_torrent->session().has_asnum_db())
		{
			int lhs_as = lhs.inet_as ? lhs.inet_as->second : 0;
			int rhs_as = rhs.inet_as ? rhs.inet_as->second : 0;
			if (lhs_as != rhs_as) return lhs_as > rhs_as;
		}
#endif
		int lhs_distance = cidr_distance(external_ip, lhs.address());
		int rhs_distance = cidr_distance(external_ip, rhs.address());
		if (lhs_distance < rhs_distance) return true;
		return false;
	}
}

