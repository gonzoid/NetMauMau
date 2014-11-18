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

void EventHandler::gameOver() const throw(NetMauMau::Common::Exception::SocketException) {
	m_connection << "BYE";
}

bool EventHandler::shutdown() const {
	return m_interrupt;
}

void EventHandler::setInterrupted() {
	m_interrupt = true;
	Connection::setInterrupted();
}

void EventHandler::reset() throw() {
	m_lastMsg.clear();
	m_connection.reset();
	m_interrupt = false;
}

void EventHandler::message_internal(const std::string &type, const std::string &msg,
									const std::vector<std::string> &except) const
throw(NetMauMau::Common::Exception::SocketException) {

	for(Connection::PLAYERINFOS::const_iterator i(m_connection.getPlayers().begin());
			i != m_connection.getPlayers().end(); ++i) {

		if(std::find(except.begin(), except.end(), i->name) == except.end()) {
			try {
				m_connection.write(i->sockfd, type);
				m_connection.write(i->sockfd, msg);
			} catch(const NetMauMau::Common::Exception::SocketException &) {
				logError("Couldn't send \"" << type << "\" to \"" << i->name << "\"");
			}
		}
	}
}

void EventHandler::message(const std::string &msg, const std::vector<std::string> &except) const
throw(NetMauMau::Common::Exception::SocketException) {
	if(msg != m_lastMsg) {
		m_lastMsg = msg;
		message_internal("MESSAGE", msg, except);
	}
}

void EventHandler::error(const std::string &msg, const std::vector<std::string> &except) const
throw(NetMauMau::Common::Exception::SocketException) {
	message_internal("ERROR", msg, except);
	logError(msg);
}

void EventHandler::playerAdded(const NetMauMau::Player::IPlayer *player) const
throw(NetMauMau::Common::Exception::SocketException) {
	m_connection << "PLAYERJOINED" << player->getName();
}

void EventHandler::playerRejected(const NetMauMau::Player::IPlayer *player) const
throw(NetMauMau::Common::Exception::SocketException) {
	if(player->getSerial() != -1) {
		m_connection.write(player->getSerial(), "PLAYERREJECTED");
		m_connection.write(player->getSerial(), player->getName());
	}
}

void EventHandler::initialCard(const NetMauMau::Common::ICard *ic) const
throw(NetMauMau::Common::Exception::SocketException) {
	m_connection << "INITIALCARD" << ic->description();
}

void EventHandler::uncoveredCard(const NetMauMau::Common::ICard *uc) const
throw(NetMauMau::Common::Exception::SocketException) {
	m_connection << "OPENCARD";
	m_connection << uc->description();
}

void EventHandler::talonEmpty(bool empty) const
throw(NetMauMau::Common::Exception::SocketException) {
	m_connection << std::string("SUSPEND ").append(empty ? "OFF" : "ON");
}

void EventHandler::turn(std::size_t t) const throw(NetMauMau::Common::Exception::SocketException) {
	std::ostringstream ts;
	ts << t;
	m_connection << "TURN" << ts.str();
}

void EventHandler::stats(const NetMauMau::Engine::PLAYERS &m_players) const
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

void EventHandler::playerWins(const NetMauMau::Player::IPlayer *player, std::size_t,
							  bool ultimate) const
throw(NetMauMau::Common::Exception::SocketException) {

	std::string pw("PLAYERWINS");

	if(ultimate) pw.append(1, '+');

	m_connection << pw << player->getName();
}

void EventHandler::playerLost(const NetMauMau::Player::IPlayer *player, std::size_t) const
throw(NetMauMau::Common::Exception::SocketException) {
	m_connection << "PLAYERLOST" << player->getName();
}

void EventHandler::playerPlaysCard(const NetMauMau::Player::IPlayer *player,
								   const NetMauMau::Common::ICard *playedCard,
								   const NetMauMau::Common::ICard *) const
throw(NetMauMau::Common::Exception::SocketException) {
	m_connection << "PLAYEDCARD" << player->getName() << playedCard->description();
}

void EventHandler::cardRejected(NetMauMau::Player::IPlayer *player,
								const NetMauMau::Common::ICard *,
								const NetMauMau::Common::ICard *playedCard) const
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
								  const NetMauMau::Common::ICard *) const
throw(NetMauMau::Common::Exception::SocketException) {
	m_connection << "SUSPENDS" << player->getName();
}

void EventHandler::playerPicksCard(const NetMauMau::Player::IPlayer *player,
								   const NetMauMau::Common::ICard *card) const
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

void EventHandler::playerPicksCards(const NetMauMau::Player::IPlayer *player,
									std::size_t cardCount) const
throw(NetMauMau::Common::Exception::SocketException) {

	std::ostringstream os;
	os << cardCount;
	m_connection << "PLAYERPICKSCARDS" << player->getName() << "TAKECOUNT" << os.str();
}

void EventHandler::playerChooseJackSuit(const NetMauMau::Player::IPlayer *,
										NetMauMau::Common::ICard::SUIT suit) const
throw(NetMauMau::Common::Exception::SocketException) {
	m_connection << "JACKSUIT" << NetMauMau::Common::suitToSymbol(suit, false);
}

void EventHandler::nextPlayer(const NetMauMau::Player::IPlayer *player) const
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
