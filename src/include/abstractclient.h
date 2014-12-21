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

/**
 * @file
 * @brief The main class of a client
 * @author Heiko Schäfer <heiko@rangun.de>
 *
 * @mainpage %NetMauMau Client API
 *
 * The %NetMauMau Client API is for developers of clients for %NetMauMau.
 * It handles the connection to a server and provides callbacks via pure virtual functions.
 *
 * To use the @b %NetMauMau @b Client @b API please read at first how to implement
 * NetMauMau::Client::AbstractClient. @n Useful functions you'll find in NetMauMau::Common.
 *
 * Link your client against @c -lnetmaumauclient and @c -lnetmaumaucommon
 *
 * Get the lastest code via git: `git clone https://github.com/velnias75/NetMauMau.git`
 *
 * @htmlinclude README.md
 */

#ifndef NETMAUMAU_ABSTRACTCLIENT_H
#define NETMAUMAU_ABSTRACTCLIENT_H

#include <vector>

#include "iplayerpiclistener.h"
#include "clientconnection.h"
#include "icard.h"

namespace NetMauMau {

/// @brief Classes and functions used by clients only
namespace Client {

/**
 * @brief Main client interface to communicate with the server
 *
 * In your client subclass @c AbstractClient and implement all pure virtual methods. In the
 * constructor @c AbstractClient() you can setup the connection.\n
 * To actually join the game you need to call @c play(). The pure virtual functions are
 * translated events and requests of the server, which your client has to handle accordingly.\n
 * If you just want to query the player list, you can call @c playerList() and to get the
 * servers capabilities you can call @c capabilities()
 *
 * A complete proof of concept client can be obtained via git:
 * `git clone https://github.com/velnias75/NetMauMau-Qt-Client.git`
 *
 * @note All data is transferred as UTF-8 encoded byte strings
 * @note Starting with version 0.6 and with *libmagic* enabled, the server checks if the player
 * images are really in the PNG format
 */
class _EXPORT AbstractClientV05 : protected IPlayerPicListener {
	DISALLOW_COPY_AND_ASSIGN(AbstractClientV05)
	friend class AbstractClientV07;
public:
	/// @copydoc Connection::CAPABILITIES
	typedef Connection::CAPABILITIES CAPABILITIES;

	/// @copydoc Connection::PLAYERLIST
	typedef Connection::PLAYERLIST PLAYERLIST;

	/// @copydoc Connection::PLAYERINFOS
	typedef Connection::PLAYERINFOS PLAYERINFOS;

	/**
	 * @brief A vector of @c Common::ICard pointers
	 */
	typedef std::vector<Common::ICard *> CARDS;

	/**
	* @brief Statistics entry about the other player's card count
	*/
	typedef struct {
		std::string playerName; ///< the player's name
		std::size_t cardCount; ///< the player's card count
	} STAT;

	/**
	 * @brief A vector with statistics about the other player's card count
	 * @see STAT
	 */
	typedef std::vector<STAT> STATS;

	virtual ~AbstractClientV05();

	/**
	 * @brief Attempt to start a game on the servers
	 *
	 * @param timeout the time to wait for a connection, if @c NULL there will be no timeout
	 *
	 * @throw Common::Exception::SocketException if the connection failed
	 * @throw Client::Exception::TimeoutException if the connection attempt timed out
	 * @throw Client::Exception::ProtocolErrorException if there was a aprotocol error
	 * @throw Client::Exception::ConnectionRejectedException if the connection got rejected
	 * @throw Client::Exception::NoNetMauMauServerException
	 * if the remote host is no %NetMauMau server
	 * @throw Client::Exception::ShutdownException if the server is shutting down
	 * @throw Client::Exception::VersionMismatchException if the client is not supported
	 */
	void play(timeval *timeout = NULL) throw(NetMauMau::Common::Exception::SocketException);

	/**
	 * @brief Disconnects the client from the server
	 */
	void disconnect();

	/**
	 * @name Server query methods
	 * @{
	 */

	/**
	 * @brief Returns the server capabilities
	 *
	 * @param timeout the time to wait for a connection, if @c NULL there will be no timeout
	 *
	 * @throw Common::Exception::SocketException if the connection failed
	 * @throw Client::Exception::CapabilitiesException if the capabilities cannot get retrieved
	 * @throw Client::Exception::TimeoutException if the connection attempt timed out
	 *
	 * @return CAPABILITIES the server capabilities
	 */
	CAPABILITIES capabilities(timeval *timeout = NULL)
	throw(NetMauMau::Common::Exception::SocketException);

	/**
	 * @brief Returns the list of currently registered player names
	 *
	 * @note The image data returned in @c NetMauMau::Client::AbstractClient::PLAYERLIST must
	 * be freed by the user @code delete [] x->pngData @endcode
	 *
	 * @param playerPNG @c true if the player images should get retrieved
	 * @param timeout the time to wait for a connection, if @c NULL there will be no timeout
	 *
	 * @throw Common::Exception::SocketException if the connection failed
	 * @throw Client::Exception::PlayerlistException if the player list cannot get retrieved
	 * @throw Client::Exception::TimeoutException if the connection attempt timed out
	 *
	 * @return NetMauMau::Client::AbstractClient::PLAYERLIST
	 * the list of currently registered player names
	 *
	 * @since 0.4
	 */
	PLAYERINFOS playerList(bool playerPNG, timeval *timeout = NULL)
	throw(NetMauMau::Common::Exception::SocketException);

	/**
	 * @brief Returns the list of currently registered player names
	 *
	 * It does not retrieve the player images
	 *
	 * @overload
	 */
	PLAYERLIST playerList(timeval *timeout = NULL)
	throw(NetMauMau::Common::Exception::SocketException);

	/**
	 * @brief Returns the version of the client's implemented protocol version
	 *
	 * You can retrieve major and minor version as following: @code
	 * uint16_t major = static_cast<uint16_t>(AbstractClient::getClientProtocolVersion() >> 16);
	 * uint16_t minor = static_cast<uint16_t>(AbstractClient::getClientProtocolVersion());
	 * @endcode
	 *
	 * @return uint32_t the protocol version
	 */
	static uint32_t getClientProtocolVersion() _CONST;

	/**
	 * @brief Parses a version string and returns the resulting protocol version
	 *
	 * @param version the protocol version as string
	 * @return the protocol version
	 *
	 * @since 0.3
	 */
	static uint32_t parseProtocolVersion(const std::string &version);

	/**
	 * @brief Checks if an player image is uploadable to the server
	 *
	 * @note it is possible that the server rejects the image anyway if configured to use
	 * a different (smaller) maximum file size than this client
	 *
	 * @param pngData the image data
	 * @param pngDataLen length of the image data
	 * @return bool @true if the file will most probably accepted by the server, @c false otherwise
	 *
	 * @since 0.5
	 */
	static bool isPlayerImageUploadable(const unsigned char *pngData, std::size_t pngDataLen);

	/**
	 * @brief Gets the default port of the server
	 *
	 * @return uint16_t the default port of the server
	 */
	static uint16_t getDefaultPort() _CONST;

	/**
	 * @brief Gets the player's name
	 *
	 * @return std::string the player's name
	 */
	std::string getPlayerName() const;

	/**
	 * @brief Gets the compiled in default AI player name
	 *
	 * @return the compiled in default AI player name
	 */
	static const char *getDefaultAIName() _CONST;

	/// @}

protected:
	/**
	 * @brief Creates an @c AbstractClientV05 instance
	 *
	 * Sets up all information to connect to a server
	 *
	 * @see play
	 *
	 * @param player the player's name
	 * @param server the server to connect to
	 * @param port the server port to connect to
	 */
	AbstractClientV05(const std::string &player, const std::string &server, uint16_t port);

	/**
	 * @brief Creates an @c AbstractClientV05 instance
	 *
	 * Sets up all information to connect to a server. Additionally a player picture can be
	 * submitted
	 *
	 * @see play
	 *
	 * @param player the player's name
	 * @param pngData pointer to a buffer containg PNG image data or @c NULL
	 * @param pngDataLen length of the data in the buffer pointed to by @c pngData
	 * @param server the server to connect to
	 * @param port the server port to connect to
	 *
	 * @since 0.4
	 */
	AbstractClientV05(const std::string &player, const unsigned char *pngData,
					  std::size_t pngDataLen, const std::string &server, uint16_t port);

	/**
	 * @name Server requests
	 * @{
	 */

	/**
	 * @brief The server requests a card to play
	 *
	 * @note If the client send an <em>illegal card</em> the client will be requsted to
	 * choose a card again. Before the client will receive the amount of extra cards
	 * to take by played out SEVEN rank cards.
	 *
	 * @see Common::getIllegalCard
	 *
	 * @param cards playable cards, which will get accepted by the server
	 * @return NetMauMau::Common::ICard* the card the player wants to play
	 * or @c NULL if the player cannot play a card and/or suspends the turn or
	 * the <em>illegal card</em>
	 */
	virtual Common::ICard *playCard(const CARDS &cards) const = 0;

	/**
	 * @brief Gets the current Jack suit
	 *
	 * @return NetMauMau::Common::ICard::SUIT the current Jack suit
	 */
	virtual Common::ICard::SUIT getJackSuitChoice() const = 0;

	//@}

	/**
	 * @name Sever events
	 * @{
	 */

	/**
	 * @brief The server send a general message
	 *
	 * @param msg the general message
	 */
	virtual void message(const std::string &msg) const = 0;

	/**
	 * @brief The server send a error message
	 *
	 * @param msg the error message
	 */
	virtual void error(const std::string &msg) const = 0;

	/**
	 * @brief A new turn has started
	 *
	 * @param turn number of the current turn
	 */
	virtual void turn(std::size_t turn) const = 0;

	/**
	 * @brief The server sent statistics about the other player's card count
	 *
	 * @param stats the statistics about the other player's card count
	 */
	virtual void stats(const STATS &stats) const = 0;

	/**
	 * @brief The server announced the game is over
	 */
	virtual void gameOver() const = 0;

	/**
	 * @brief A new player joined the game
	 *
	 * Transmits a PNG picture of the player if available
	 *
	 * @param player the new player's name
	 * @param pngData PNG data of the players picture or @c 0L
	 * @param pngDataLen length of the PNG data
	 */
	virtual void playerJoined(const std::string &player, const unsigned char *pngData,
							  std::size_t len) const = 0;

	/**
	 * @brief A player got rejected to join the game
	 *
	 * @param player the rejected player's name
	 */
	virtual void playerRejected(const std::string &player) const = 0;

	/**
	 * @brief A player suspends this turn
	 *
	 * @param player the suspending player's name
	 */
	virtual void playerSuspends(const std::string &player) const = 0;

	/**
	 * @brief A player played a card
	 *
	 * @param player the player's name
	 * @param card the card the player played
	 */
	virtual void playedCard(const std::string &player, const Common::ICard *card) const = 0;

	/**
	 * @brief A player has won the game
	 *
	 * @param player the player's name
	 * @param turn the number of the turn the player has won
	 */
	virtual void playerWins(const std::string &player, std::size_t turn) const = 0;

	/**
	 * @brief A player has lost the game
	 *
	 * @param player the player's name
	 * @param turn the number of the turn the player has lost
	 * @param points the points the loosing player had in hand
	 */
	virtual void playerLost(const std::string &player, std::size_t turn,
							std::size_t points) const = 0;

	/**
	 * @brief A player picks up a card
	 *
	 * @note The card is @c NULL if the player is a remote player
	 *
	 * @param player the player's name
	 * @param card the card the player picked up
	 */
	virtual void playerPicksCard(const std::string &player, const Common::ICard *card) const = 0;

	/**
	 * @brief A player picks up an amount of cards
	 *
	 * @param player the player's name
	 * @param count the count of picked up cards
	 */
	virtual void playerPicksCard(const std::string &player, std::size_t count) const = 0;

	/**
	 * @brief Name of the next player
	 *
	 * @param player the next player's name
	 */
	virtual void nextPlayer(const std::string &player) const = 0;

	/**
	 * @brief Notes if suspending and taking a card possible
	 *
	 * If there are no more cards on the talon, except the open card, suspending and
	 * taking card a is not possible
	 *
	 * @param enable @c true if it is possible to take a card, @c false otherwise
	 */
	virtual void enableSuspend(bool enable) const = 0;

	/**
	 * @brief The card set distributed to the player, or if the
	 * player picked up cards off the talon
	 *
	 * @param cards the card set given to the player
	 */
	virtual void cardSet(const CARDS &cards) const = 0;

	/**
	 * @brief The initial card
	 *
	 * @param card the initial card
	 */
	virtual void initialCard(const Common::ICard *card) const = 0;

	/**
	 * @brief The current open card
	 *
	 * If there is a suit chosen by a Jack the jackSuit contains it and can get converted to
	 * @c NetMauMau::Common::ICard::SUIT by NetMauMau::Common::symbolToSuit, else it will be
	 * a empty string.
	 *
	 * @see NetMauMau::Common::symbolToSuit
	 *
	 * @param card the current open card
	 * @param jackSuit the current jack suit
	 */
	virtual void openCard(const Common::ICard *card, const std::string &jackSuit) const = 0;

	/**
	 * @brief The talon was empty and shuffled anew
	 *
	 * All played cards, except the open top card are shuffled and added to the Talon.
	 *
	 * This event can be used for displaying an shuffle animation.
	 *
	 */
	virtual void talonShuffled() const = 0;

	/**
	 * @brief The player's played card got rejected
	 *
	 * @param player the player's name
	 * @param card the rejected card
	 */
	virtual void cardRejected(const std::string &player, const Common::ICard *card) const = 0;

	/**
	 * @brief The player's played card got accepted
	 *
	 * @param card the accepted card
	 */
	virtual void cardAccepted(const Common::ICard *card) const = 0;

	/**
	 * @brief The server announces a Jack suit
	 *
	 * @param suit the current Jack suit
	 */
	virtual void jackSuit(Common::ICard::SUIT suit) const = 0;

	// @}

	/**
	 * @name Player image notifications
	 *
	 * The notifications can be overloaded if the client is interested in events
	 * regarding the player pictures.
	 *
	 * This functions all do nothing at default.
	 *
	 * @{
	 */

	/**
	 * @brief A download of a player image has started
	 *
	 * @param player the player the image is downloaded for
	 *
	 * @since 0.4
	 */
	virtual void beginReceivePlayerPicture(const std::string &player) const throw() _CONST;

	/**
	 * @brief A download of a player image has ended
	 *
	 * @param player the player the image is downloaded for
	 *
	 * @since 0.4
	 */
	virtual void endReceivePlayerPicture(const std::string &player) const throw() _CONST;

	/**
	 * @brief The upload of the player image has succeded
	 *
	 * @param player the player the image is uploaded for
	 *
	 * @since 0.4
	 */
	virtual void uploadSucceded(const std::string &player) const throw() _CONST;

	/**
	 * @brief The upload of the player image has failed
	 *
	 * @param player the player the image is uploaded for
	 *
	 * @since 0.4
	 */
	virtual void uploadFailed(const std::string &player) const throw() _CONST;

	/// @}

	/**
	 * @brief The server sent a message not understood by the client
	 *
	 * @param message the unknown message
	 */
	virtual void unknownServerMessage(std::string message) const = 0;

private:
	typedef enum { OK, NOT_UNDERSTOOD, BREAK } PIRET;
	virtual PIRET playInternal(std::string &msg, std::size_t *cturn, bool *initCardShown,
							   std::string &cjackSuit,
							   const NetMauMau::Common::ICard **lastPlayedCard)
	throw(NetMauMau::Common::Exception::SocketException);

private:
	Connection m_connection;
	const std::string m_pName;
	unsigned char *m_pngData;
	std::size_t m_pngDataLen;
	CARDS m_cards;
	const Common::ICard *m_openCard;
	bool m_disconnectNow;
};

class _EXPORT AbstractClientV07 : public AbstractClientV05 {
	DISALLOW_COPY_AND_ASSIGN(AbstractClientV07)
protected:
	AbstractClientV07(const std::string &player, const std::string &server, uint16_t port);
	AbstractClientV07(const std::string &player, const unsigned char *pngData,
					  std::size_t pngDataLen, const std::string &server, uint16_t port);

	virtual bool getAceRoundChoice() const = 0;
	virtual void aceRoundStarted() const = 0;
	virtual void aceRoundEnded() const = 0;

private:
	using AbstractClientV05::playInternal;

	virtual AbstractClientV05::PIRET playInternal(std::string &msg, std::size_t *cturn,
			bool *initCardShown, std::string &cjackSuit, const Common::ICard **lastPlayedCard)
	throw(NetMauMau::Common::Exception::SocketException);
};

typedef AbstractClientV07 AbstractClient;

}

}

#endif /* NETMAUMAU_ABSTRACTCLIENT_H */

// kate: indent-mode cstyle; indent-width 4; replace-tabs off; tab-width 4; 
