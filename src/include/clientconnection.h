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
 * @brief Handles the connection from the client to a server
 * @author Heiko Schäfer <heiko@rangun.de>
 */

#ifndef NETMAUMAU_CLIENTCONNECTION_H
#define NETMAUMAU_CLIENTCONNECTION_H

#include "abstractconnection.h"

namespace NetMauMau {

namespace Client {

class IPlayerPicListener;
class ConnectionImpl;

/**
 * @brief Handles the connection from the client to a server
 */
class Connection : public Common::AbstractConnection {
	DISALLOW_COPY_AND_ASSIGN(Connection)
	friend class ConnectionImpl;
public:
	using AbstractConnection::connect;

	/**
	 * @brief Holds the name as well as the PNG data of the player image
	 */
	typedef struct {
		std::string name; ///< the player name
		const unsigned char *pngData; ///< raw data of the player image, must be freed by the client
		std::size_t pngDataLen; ///< length of the raw data of the player image
	} PLAYERINFO;

	/**
	 * @brief List of currently registered player names
	 */
	typedef std::vector<std::string> PLAYERLIST;

	/**
	 * @brief List of currently registered player infos
	 */
	typedef std::vector<PLAYERINFO> PLAYERINFOS;

	Connection(const std::string &pName, const std::string &server, uint16_t port);
	virtual ~Connection();

	void setClientVersion(uint32_t clientVersion);

	virtual void connect(const IPlayerPicListener *l, const unsigned char *pngData,
						 std::size_t pngDataLen) throw(Common::Exception::SocketException);
	CAPABILITIES capabilities() throw(NetMauMau::Common::Exception::SocketException);
	PLAYERINFOS playerList(const IPlayerPicListener *hdl,
						   bool playerPNG) throw(Common::Exception::SocketException);

	void setTimeout(struct timeval *timeout);

	Connection &operator>>(std::string &msg) throw(Common::Exception::SocketException);
	Connection &operator<<(const std::string &msg) throw(Common::Exception::SocketException);

protected:
	virtual bool wire(SOCKET sockfd, const struct sockaddr *addr, socklen_t addrlen) const;
	virtual std::string wireError(const std::string &err) const;

private:
	ConnectionImpl *const _pimpl;
};

_EXPORT bool operator<(const std::string &, const Connection::PLAYERINFO &) _PURE;
_EXPORT bool operator<(const Connection::PLAYERINFO &, const std::string &) _PURE;

}

}

#endif /* NETMAUMAU_CLIENTCONNECTION_H */

// kate: indent-mode cstyle; indent-width 4; replace-tabs off; tab-width 4; 
