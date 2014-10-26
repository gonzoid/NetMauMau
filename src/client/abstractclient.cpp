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

#if defined(HAVE_CONFIG_H) || defined(IN_IDE_PARSER)
#include "config.h"
#endif

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#include <algorithm>
#include <sstream>

#include "abstractclient.h"

#include "interceptederrorexception.h"
#include "clientcardfactory.h"
#include "cardtools.h"
#include "logger.h"

using namespace NetMauMau::Client;

AbstractClient::AbstractClient(const std::string &pName, const std::string &server, uint16_t port) :
	m_connection(pName, server, port), m_pName(pName), m_cards(), m_openCard(0L) {}

AbstractClient::~AbstractClient() {
	for(std::vector<NetMauMau::Common::ICard *>::const_iterator i(m_cards.begin());
			i != m_cards.end(); ++i) {
		delete *i;
	}

	delete m_openCard;
}

std::string AbstractClient::getPlayerName() const {
	return m_pName;
}

AbstractClient::CARDS
AbstractClient::getCards(const std::vector<NetMauMau::Common::ICard *>::const_iterator
						 &first) const {

	CARDS cards;

	for(std::vector<NetMauMau::Common::ICard *>::const_iterator i(first); i != m_cards.end(); ++i) {
		cards.push_back(*i);
	}

	return cards;
}

Connection::CAPABILITIES AbstractClient::capabilities(timeval *timeout)
throw(NetMauMau::Common::Exception::SocketException) {
	m_connection.setTimeout(timeout);
	return m_connection.capabilities();
}

void AbstractClient::play(timeval *timeout) throw(NetMauMau::Common::Exception::SocketException) {

	m_connection.setTimeout(timeout);
	m_connection.connect();

	NetMauMau::Common::ICard *lastPlayedCard = 0L;
	bool initCardShown = false;
	std::string msg, cjackSuit;
	std::size_t cturn = 0;

	while(true) {

		try {

			m_connection >> msg;

			if(!msg.empty()) {

				if(msg == "MESSAGE") {
					m_connection >> msg;
					message(msg);
				} else if(msg == "ERROR") {
					m_connection >> msg;
					error(msg);
					break;
				} else if(msg == "TURN") {

					m_connection >> msg;

					(std::istringstream(msg)) >> cturn;

					turn(cturn);

				} else if(msg == "NEXTPLAYER") {
					m_connection >> msg;
					nextPlayer(msg);
				} else if(msg == "STATS") {

					m_connection >> msg;

					STATS cstats;

					while(msg != "ENDSTATS") {

						std::string cntS;
						m_connection >> cntS;

						std::size_t cnt;
						(std::istringstream(cntS)) >> cnt;

						STAT stat = { msg, cnt };
						cstats.push_back(stat);

						m_connection >> msg;
					}

					stats(cstats);

				} else if(msg == "PLAYERJOINED") {
					m_connection >> msg;
					playerJoined(msg);
				} else if(msg == "PLAYERREJECTED") {
					m_connection >> msg;
					playerRejected(msg);
					break;
				} else if(msg == "PLAYERWINS") {
					m_connection >> msg;
					playerWins(msg, cturn);
					gameOver();
					break;
				} else if(msg == "GETCARDS") {

					m_connection >> msg;

					const bool initialCards = m_cards.empty();
					const std::vector<NetMauMau::Common::ICard *>::const_iterator &e(m_cards.end());

					while(msg != "CARDSGOT") {
						m_cards.push_back((NetMauMau::Client::CardFactory(msg)).create());
						m_connection >> msg;
					}

					cardSet(getCards(initialCards ? m_cards.begin() : e));

				} else if(msg == "INITIALCARD") {

					m_connection >> msg;
					NetMauMau::Common::ICard *ic = (NetMauMau::Client::CardFactory(msg)).create();

					if(ic->getRank() == NetMauMau::Common::ICard::JACK ||
							ic->getRank() == NetMauMau::Common::ICard::EIGHT) {
						initialCard(ic);
						initCardShown = true;
					}

					delete ic;

				} else if(msg == "OPENCARD") {

					m_connection >> msg;
					delete m_openCard;
					m_openCard = (NetMauMau::Client::CardFactory(msg)).create();

					if(!initCardShown) {
						openCard(m_openCard, cjackSuit);
					} else {
						initCardShown = false;
					}

				} else if(msg == "PLAYCARD") {

					lastPlayedCard = playCard(getCards(m_cards.begin()));

					if(lastPlayedCard) {
						m_connection << lastPlayedCard->description();
					} else {
						m_connection << "SUSPEND";
					}

				} else if(msg == "SUSPENDS") {
					m_connection >> msg;
					playerSuspends(msg);
				} else if(msg == "CARDACCEPTED") {

					m_connection >> msg;

					if(lastPlayedCard) {
						const std::vector<NetMauMau::Common::ICard *>::iterator
						&f(std::find(m_cards.begin(), m_cards.end(), lastPlayedCard));

						if(f != m_cards.end()) {
							delete *f;
							m_cards.erase(f);
						}
					}

				} else if(msg == "CARDREJECTED") {

					std::string player;
					m_connection >> player >> msg;

					NetMauMau::Common::ICard *c = (NetMauMau::Client::CardFactory(msg)).create();
					cardRejected(player, c);
					delete c;

				} else if(msg == "CARDCOUNT") {

					std::ostringstream os;
					os << m_cards.size();
					m_connection << os.str();

				} else if(msg == "PLAYEDCARD") {

					std::string player;
					m_connection >> player >> msg;

					NetMauMau::Common::ICard *c = (NetMauMau::Client::CardFactory(msg)).create();
					playedCard(player, c);
					delete c;

					cjackSuit.clear();

				} else if(msg == "JACKSUIT") {
					m_connection >> msg;
					cjackSuit = msg;
					jackSuit(NetMauMau::Common::symbolToSuit(cjackSuit));
				} else if(msg == "JACKCHOICE") {
					m_connection << NetMauMau::Common::suitToSymbol(getJackSuitChoice(), false);
				} else if(msg == "PLAYERPICKSCARD") {

					std::string player, extra;
					m_connection >> player >> extra;

					if(extra == "CARDTAKEN") {
						m_connection >> msg;
						NetMauMau::Common::ICard *c = (NetMauMau::Client::CardFactory(msg)).create();
						playerPicksCard(player, c);
						delete c;
					} else {
						playerPicksCard(player, static_cast<NetMauMau::Common::ICard *>(0L));
					}

				} else if(msg == "PLAYERPICKSCARDS") {

					std::string player;
					m_connection >> player >> msg;
					m_connection >> msg;

					std::size_t count;
					(std::istringstream(msg)) >> count;

					playerPicksCard(player, count);

				} else if(msg == "BYE") {
					gameOver();
					break;
				} else {
					logDebug(__PRETTY_FUNCTION__ << ": " << msg);
				}
			}

		} catch(const Exception::InterceptedErrorException &e) {
			error(e.what());
			break;
		}
	}
}

// kate: indent-mode cstyle; indent-width 4; replace-tabs off; tab-width 4; 
