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

#include <sstream>
#include <algorithm>

#include "servereventhandler.h"

#include "cardtools.h"
#include "iplayer.h"
#include "logger.h"

using namespace NetMauMau::Server;

bool EventHandler::m_interrupt = false;

EventHandler::EventHandler(Connection &con) : DefaultEventHandler(), m_connection(con),
	m_lastMsg() {}

EventHandler::~EventHandler() {}

Connection *EventHandler::getConnection() const {
	return &m_connection;
}

bool EventHandler::shutdown() const {
	return m_interrupt;
}

void EventHandler::setInterrupted() {
	m_interrupt = true;
	Connection::setInterrupted();
}

void EventHandler::reset() {
	m_lastMsg.clear();
	m_connection.reset();
	m_interrupt = false;
}

void EventHandler::message_internal(const std::string &type, const std::string &msg,
									const std::vector<std::string> &except)
throw(NetMauMau::Common::Exception::SocketException) {

	for(Connection::PLAYERINFOS::const_iterator i(m_connection.getPlayers().begin());
			i != m_connection.getPlayers().end(); ++i) {

		if(std::find(except.begin(), except.end(), i->name) == except.end()) {
			m_connection.write(i->sockfd, type);
			m_connection.write(i->sockfd, msg);
		}
	}
}

void EventHandler::message(const std::string &msg, const std::vector<std::string> &except)
throw(NetMauMau::Common::Exception::SocketException) {
	if(msg != m_lastMsg) {
		m_lastMsg = msg;
		message_internal("MESSAGE", msg, except);
	}
}

void EventHandler::error(const std::string &msg, const std::vector<std::string> &except)
throw(NetMauMau::Common::Exception::SocketException) {
	message_internal("ERROR", "ERROR: " + msg, except);
	logError(msg);
}

void EventHandler::playerAdded(const NetMauMau::Player::IPlayer *player)
throw(NetMauMau::Common::Exception::SocketException) {
	m_connection << "PLAYERJOINED" << player->getName();
}

void EventHandler::playerRejected(const NetMauMau::Player::IPlayer *player)
throw(NetMauMau::Common::Exception::SocketException) {
	if(player->getSerial() != -1) {
		m_connection.write(player->getSerial(), "PLAYERREJECTED");
		m_connection.write(player->getSerial(), player->getName());
	}
}

void EventHandler::initialCard(const NetMauMau::ICard *initialCard)
throw(NetMauMau::Common::Exception::SocketException) {
	m_connection << "INITIALCARD" << initialCard->description();
}

void EventHandler::turn(std::size_t turn) throw(NetMauMau::Common::Exception::SocketException) {
	std::ostringstream ts;
	ts << turn;
	m_connection << "TURN" << ts.str();
}

void EventHandler::stats(const NetMauMau::Engine::PLAYERS &m_players)
throw(NetMauMau::Common::Exception::SocketException) {

	for(Connection::PLAYERINFOS::const_iterator i(m_connection.getPlayers().begin());
			i != m_connection.getPlayers().end(); ++i) {

		m_connection.write(i->sockfd, "STATS");

		for(NetMauMau::Engine::PLAYERS::const_iterator j(m_players.begin()); j != m_players.end();
				++j) {

			if((*j)->getName() != i->name) {

				std::ostringstream os;
				os << (*j)->getCardCount();

				try {
					m_connection.write(i->sockfd, (*j)->getName());
					m_connection.write(i->sockfd, os.str());
				} catch(const NetMauMau::Common::Exception::SocketException &) {
					m_connection.write(i->sockfd, "0");
				}
			}
		}

		m_connection.write(i->sockfd, "ENDSTATS");
	}
}

void EventHandler::playerWins(const NetMauMau::Player::IPlayer *player,
							  std::size_t) throw(NetMauMau::Common::Exception::SocketException) {
	m_connection << "PLAYERWINS" << player->getName();
}

void EventHandler::playerPlaysCard(const NetMauMau::Player::IPlayer *player,
								   const NetMauMau::ICard *playedCard,
								   const NetMauMau::ICard *)
throw(NetMauMau::Common::Exception::SocketException) {
	m_connection << "PLAYEDCARD" << player->getName() << playedCard->description();
}

void EventHandler::cardRejected(NetMauMau::Player::IPlayer *player, const NetMauMau::ICard *,
								const NetMauMau::ICard *playedCard)
throw(NetMauMau::Common::Exception::SocketException) {

	for(Connection::PLAYERINFOS::const_iterator i(m_connection.getPlayers().begin());
			i != m_connection.getPlayers().end(); ++i) {

		if(player->getName() == i->name) {
			m_connection.write(i->sockfd, "CARDREJECTED");
			m_connection.write(i->sockfd, player->getName());
			m_connection.write(i->sockfd, playedCard->description());
		}
	}
}

void EventHandler::playerSuspends(const NetMauMau::Player::IPlayer *player,
								  const NetMauMau::ICard *)
throw(NetMauMau::Common::Exception::SocketException) {
	m_connection << "SUSPENDS" << player->getName();
}

void EventHandler::playerPicksCard(const NetMauMau::Player::IPlayer *player,
								   const NetMauMau::ICard *card)
throw(NetMauMau::Common::Exception::SocketException) {

	for(Connection::PLAYERINFOS::const_iterator i(m_connection.getPlayers().begin());
			i != m_connection.getPlayers().end(); ++i) {

		m_connection.write(i->sockfd, "PLAYERPICKSCARD");
		m_connection.write(i->sockfd, player->getName());

		if(i->name == player->getName()) {
			m_connection.write(i->sockfd, "CARDTAKEN");
			m_connection.write(i->sockfd, card->description());
		} else {
			m_connection.write(i->sockfd, "HIDDENCARDTAKEN");
		}
	}
}

void EventHandler::playerPicksCards(const NetMauMau::Player::IPlayer *player, std::size_t cardCount)
throw(NetMauMau::Common::Exception::SocketException) {
	std::ostringstream os;
	os << cardCount;
	m_connection << "PLAYERPICKSCARDS" << player->getName() << "TAKECOUNT" << os.str();
}

void EventHandler::playerChooseJackSuite(const NetMauMau::Player::IPlayer *,
		NetMauMau::ICard::SUITE suite) throw(NetMauMau::Common::Exception::SocketException) {
	m_connection << "JACKSUITE" << NetMauMau::Common::suiteToSymbol(suite, false);
}

void EventHandler::nextPlayer(const NetMauMau::Player::IPlayer *player)
throw(NetMauMau::Common::Exception::SocketException) {
	for(Connection::PLAYERINFOS::const_iterator i(m_connection.getPlayers().begin());
			i != m_connection.getPlayers().end(); ++i) {
		if(player->getName() != i->name) {
			m_connection.write(i->sockfd, "NEXTPLAYER");
			m_connection.write(i->sockfd, player->getName());
		}
	}
}

// kate: indent-mode cstyle; indent-width 4; replace-tabs off; tab-width 4; 
