/*
 * Copyright 2014 by Heiko Schäfer <heiko@rangun.de>
 *
 * This file is part of NetMauMau.
 *
 * NetMauMau is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * NetMauMau is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with NetMauMau.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef NETMAUMAU_GAME_H
#define NETMAUMAU_GAME_H

#if defined(HAVE_CONFIG_H) || defined(IN_IDE_PARSER)
#include "config.h"
#endif

#include "engine.h"
#include "stdplayer.h"

#ifndef _WIN32
#define TIMEFORMAT "%T - "
#else
#define TIMEFORMAT "%H:%M:%S - "
#endif

namespace NetMauMau {

namespace Event {
class IEventHandler;
}

namespace Server {

class Game {
	DISALLOW_COPY_AND_ASSIGN(Game)
public:

	typedef enum { ACCEPTED, REFUSED, ACCEPTED_READY, REFUSED_FULL } COLLECT_STATE;

	Game(Event::IEventHandler &evtHdlr, long aiDelay, bool aiPlayer = false,
		 const std::string *aiName = 0L, bool aceRound = false);
	~Game();

	COLLECT_STATE collectPlayers(std::size_t minPlayers, Player::IPlayer *player);

	inline std::size_t getPlayerCount() const {
		return m_engine.getPlayerCount();
	}

	void start(bool ultimate = false) throw(Common::Exception::SocketException);
	void reset(bool playerLost) throw();
	void shutdown() const throw();

private:
	bool addPlayer(Player::IPlayer *player);

private:
	Engine m_engine;
	const bool m_aiOpponent;
	std::vector<Player::StdPlayer *> m_aiPlayers;
	std::vector<Player::IPlayer *> m_players;
};

}

}

#endif /* NETMAUMAU_GAME_H */

// kate: indent-mode cstyle; indent-width 4; replace-tabs off; tab-width 4; 
