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

#if _WIN32
#define COPY "\270"
#define AUML "\204"
#else
#define COPY "\u00a9"
#define AUML "\u00e4"
#endif

#include <set>
#include <ctime>
#include <cerrno>
#include <climits>
#include <cstring>
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

#include <sys/stat.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifndef _WIN32
#include <ifaddrs.h>
#include <pwd.h>
#include <grp.h>
#else
#define HOST_NAME_MAX 64
#endif

#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif

#include <signal.h>

#include <popt.h>

#include "game.h"
#include "serverplayer.h"
#include "serverconnection.h"
#include "servereventhandler.h"
#include "logger.h"

#ifndef DP_USER
#define DP_USER "nobody"
#endif

#ifndef DP_GROUP
#define DP_GROUP "nogroup"
#endif

namespace {

bool aceRound = false;
bool ultimate = false;
std::size_t minPlayers = 1;
uint16_t port = SERVER_PORT;

volatile bool refuse = false;

#pragma GCC diagnostic ignored "-Wwrite-strings"
#pragma GCC diagnostic push
char bind[HOST_NAME_MAX] = { 0 };
char *host = bind;
char *aiName = AI_NAME;
std::string aiNames[4];
long aiDelay = 1000L;
#ifndef _WIN32
char *user  = DP_USER;
char *grp = DP_GROUP;
char *dpErr = 0L;
const char *interface;
#endif
#pragma GCC diagnostic pop

poptOption poptOptions[] = {
	{
		"players", 'p', POPT_ARG_INT | POPT_ARGFLAG_SHOW_DEFAULT, &minPlayers,
		'p', "Set amount of players", "AMOUNT"
	},
	{ "ultimate", 'u', POPT_ARG_NONE, NULL, 'u', "Play until last player wins", NULL },
	{
		"ace-round", 'a', POPT_ARG_NONE, NULL, 'a', "Enable ace rounds (requires all clients " \
		"to be at least of version 0.7)", NULL
	},
	{
		"ai-name", 'A', POPT_ARG_STRING | POPT_ARGFLAG_SHOW_DEFAULT, &aiName, 'A',
		"Set the name of one AI player. Can be given up to 4 times", "NAME"
	},
	{
		"ai-delay", 'D', POPT_ARG_LONG | POPT_ARGFLAG_SHOW_DEFAULT, &aiDelay,
		0, "Delay after AI turns", "MILLISECONDS"
	},
	{ "bind", 'b', POPT_ARG_STRING, &host, 0, "Bind to HOST", "HOST" },
#ifndef _WIN32
	{ "iface", 'I', POPT_ARG_STRING, &interface, 'I', "Bind to INTERFACE", "INTERFACE" },
#endif
	{
		"port", 0, POPT_ARG_INT | POPT_ARGFLAG_SHOW_DEFAULT, &port, 0,
		"Set the port to listen to", "PORT"
	},
#ifndef _WIN32
	{
		"user", 0, POPT_ARG_STRING | POPT_ARGFLAG_SHOW_DEFAULT, &user, 0,
		"Set the user to run as (only if started as root)", "USER"
	},
	{
		"group", 0, POPT_ARG_STRING | POPT_ARGFLAG_SHOW_DEFAULT, &grp, 0,
		"Set the group to run as (only if started as root)", "GROUP"
	},
#endif
	{ "version", 'V', POPT_ARG_NONE, NULL, 'V', "Display version information", NULL },
	POPT_AUTOHELP
	POPT_TABLEEND
};

void updatePlayerCap(NetMauMau::Server::Connection::CAPABILITIES &caps, std::size_t count,
					 NetMauMau::Server::Connection &con, bool aiOpponent) {

	std::ostringstream os;

	os << count - (aiOpponent ? 1 : 0);
	caps["CUR_PLAYERS"] = os.str();
	con.setCapabilities(caps);
}

volatile bool interrupt = false;

void sh_interrupt(int) {
	logWarning(NetMauMau::Common::Logger::time(TIMEFORMAT) << "Server is about to shut down");
	NetMauMau::Server::EventHandler::setInterrupted();
	interrupt = true;
}

#ifndef _WIN32
int getGroup(gid_t *gid, const char *group) {

	errno = 0;

	// cppcheck-suppress nonreentrantFunctionsgetgrnam
	struct group *g = getgrnam(group);

	if(g) {
		*gid = g->gr_gid;
		return 0;
	} else if(errno) {
		dpErr = std::strerror(errno);
	} else {
		dpErr = "unknown group";
	}

	return -1;
}


int getIPForIF(char *addr = NULL, size_t len = 0, const char *iface = NULL) {

	bool listOnly = !addr && !len && !iface;

	std::set<std::string> ifaces;

	struct ifaddrs *ifas;

	if(!::getifaddrs(&ifas)) {

		for(struct ifaddrs *ifa = ifas; ifa != NULL; ifa = ifa->ifa_next) {

			if(ifa->ifa_addr && (ifa->ifa_addr->sa_family == AF_INET ||
								 ifa->ifa_addr->sa_family == AF_INET6)) {

				ifaces.insert(ifa->ifa_name);

				if(!listOnly && !::strcmp(ifa->ifa_name, iface)) {
					::getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in),
								  addr, len, NULL, 0, NI_NUMERICHOST);
					::freeifaddrs(ifas);
					return 0;
				}
			}
		}

		if(listOnly) {
			for(std::set<std::string>::const_iterator iter(ifaces.begin()); iter != ifaces.end();
					++iter) {
				logger(*iter);
			}
		}

		::freeifaddrs(ifas);
	}

	return -1;
}

int getUser(uid_t *uid, const char *usr) {

	errno = 0;

	// cppcheck-suppress nonreentrantFunctionsgetpwnam
	struct passwd *u = getpwnam(usr);

	if(u) {
		*uid = u->pw_uid;
		return 0;
	} else if(errno) {
		dpErr = std::strerror(errno);
	} else {
		dpErr = "unknown user";
	}

	return -1;
}

int dropPrivileges(const char *usr, const char *group) {

	if(!getuid()) {

		uid_t uid;
		gid_t gid;

		if(!getUser(&uid, usr) && !getGroup(&gid, group)) {

#if !defined(_WIN32) && defined(PIDFILE) && defined(HAVE_ATEXIT) && defined(HAVE_CHOWN)

			if(chown(PIDFILE, uid, gid)) logWarning("Couldn't change ownership of " << PIDFILE);

			if(chmod(PIDFILE, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH)) {
				logWarning("Couldn't change mode of " << PIDFILE);
			}

#endif

			if(!setegid(gid) && !seteuid(uid)) {
				return 0;
			} else {
				dpErr = std::strerror(errno);
			}
		}

	} else {
		return 0;
	}

	return -1;
}
#endif

#ifdef HAVE_ATEXIT
void exit_hdlr() {
#if !defined(_WIN32) && defined(PIDFILE) && defined(HAVE_CHOWN)

	if(unlink(PIDFILE)) logWarning("Couldn't remove " << PIDFILE << ": " << std::strerror(errno));

#endif

	logInfo(NetMauMau::Common::Logger::time(TIMEFORMAT) << "Server shut down normally");
}
#endif

}

using namespace NetMauMau;

int main(int argc, const char **argv) {

	std::size_t numAI = 0;
	poptContext pctx = poptGetContext(NULL, argc, argv, poptOptions, 0);
	int c;

	while((c = poptGetNextOpt(pctx)) >= 0) {
		switch(c) {
		case 'A':

			if(std::count_if(aiNames, aiNames + numAI, std::bind2nd(std::equal_to<std::string>(),
							 aiName))) {
				logWarning("Duplicate AI player name: \"" << aiName << "\"");
			} else if(numAI < 4) {
				aiNames[numAI++] = aiName;
			} else {
				logWarning("At maximum 4 AI players are allowed; ignoring: \"" << aiName << "\"");
			}

			break;

		case 'V': {

#if !(defined(_WIN32) || defined(NOH2M))

			if(!getenv("HELP2MAN_OUTPUT")) {
#endif
				logger(PACKAGE_STRING << " " << BUILD_TARGET);
				logger("");

				std::ostringstream node;

				if(std::string("(none)") != BUILD_NODE) {
					node << " on " << BUILD_NODE;
				} else {
					node << "";
				}

				std::ostringstream cppversion;
#if defined(__GNUC__) && defined(__VERSION__)
				cppversion << " with g++ " << __VERSION__;
#else
				cppversion << "";
#endif

				char dateOut[1024];
				std::time_t t = BUILD_DATE;
				// cppcheck-suppress nonreentrantFunctionslocaltime
				std::strftime(dateOut, sizeof(dateOut), "%x", std::localtime(&t));

				logger("Built " << dateOut << node.str() << " (" << BUILD_HOST << ")"
					   << cppversion.str());

				logger("");
				// cppcheck-suppress nonreentrantFunctionslocaltime
				std::strftime(dateOut, sizeof(dateOut), "%Y", std::localtime(&t));
				logger("Copyright " COPY " " << dateOut << " Heiko Sch" AUML "fer <"
					   << PACKAGE_BUGREPORT << ">");

#ifdef PACKAGE_URL

				if(*PACKAGE_URL) {
					logger("");
					logger("WWW: " << PACKAGE_URL);
				}

#endif
				logger("");
				logger("There is NO warranty; not even for MERCHANTABILITY or FITNESS FOR A "
					   "PARTICULAR PURPOSE.");

#if !(defined(_WIN32) || defined(NOH2M))
			} else {

				char dateOut[1024];
				std::time_t t = BUILD_DATE;
				// cppcheck-suppress nonreentrantFunctionslocaltime
				std::strftime(dateOut, sizeof(dateOut), "%Y", std::localtime(&t));

				logger(PACKAGE_NAME << " - Copyright (c) " << dateOut
					   << ", Heiko Schaefer <heiko@rangun.de>");
			}

#endif
		}

		poptFreeContext(pctx);
		return EXIT_SUCCESS;

		case 'p':
			minPlayers = std::min<std::size_t>(minPlayers, 5);
			break;

		case 'u':
			ultimate = true;
			break;

		case 'a':
			aceRound = true;
			break;

		case 'I':
#if !WIN32
			if(interface[0] == '?') {
				getIPForIF();
				poptFreeContext(pctx);
				return EXIT_SUCCESS;
			} else if(getIPForIF(host, HOST_NAME_MAX, interface)) {
				if(getIPForIF(host, HOST_NAME_MAX, interface)) {
					logError("Couldn't bind to interface" << interface)
					poptFreeContext(pctx);
					return EXIT_FAILURE;
				}
			}

#endif
			break;
		}
	}

	if(c < -1) {
		std::cerr << poptBadOption(pctx, POPT_BADOPTION_NOALIAS) << ": " << poptStrerror(c)
				  << std::endl;;
		return EXIT_FAILURE;
	}

	poptFreeContext(pctx);

#ifndef HAVE_ARC4RANDOM_UNIFORM
#if HAVE_INITSTATE
	char istate[256];
	initstate(std::time(0L), istate, 256);
#else
	std::srand(std::time(0L));
#endif
#endif

#ifdef HAVE_ATEXIT
#if !defined(_WIN32) && defined(PIDFILE) && defined(HAVE_CHOWN)
	std::ofstream pidFile(PIDFILE);

	if(pidFile.is_open()) {
		pidFile << getpid();
		pidFile.close();
	} else {
		logWarning("Couldn't create " << PIDFILE);
	}

#endif

	if(std::atexit(exit_hdlr)) logWarning("Couldn't register atexit function");

#endif

	logInfo("Welcome to " << PACKAGE_STRING);

#ifndef _WIN32
	struct sigaction sa;

	std::memset(&sa, 0, sizeof(struct sigaction));

	sa.sa_handler = sh_interrupt;
	sa.sa_flags = SA_RESETHAND;
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);

	if(!dropPrivileges(user, grp)) {

#else
	signal(SIGINT, sh_interrupt);

#endif

		const bool aiOpponent = minPlayers <= 1;

		if(aiOpponent && !numAI) {
			aiNames[numAI++] = aiName;
		} else {
			minPlayers = std::min<std::size_t>(5, numAI + minPlayers);
		}

		Server::Connection con(aceRound ? std::max(7, MAKE_VERSION(MIN_MAJOR, MIN_MINOR)) :
							   MAKE_VERSION(MIN_MAJOR, MIN_MINOR), port, *host ? host : NULL);

		ultimate = (!aiOpponent && minPlayers > 2) ? ultimate : numAI > 1;

		if(ultimate) logInfo("Running in ultimate mode");

		logInfo("Server accepts clients >= "
				<< static_cast<uint16_t>(con.getMinClientVersion() >> 16)
				<< "." << static_cast<uint16_t>(con.getMinClientVersion()));

		try {

			con.connect();

			Server::EventHandler evtHdlr(con);
			Server::Game game(evtHdlr, ::labs(aiDelay), aiOpponent, aiNames, aceRound);

			Server::Connection::CAPABILITIES caps;
			caps.insert(std::make_pair("SERVER_VERSION", PACKAGE_VERSION));
			caps.insert(std::make_pair("AI_OPPONENT", aiOpponent ? "true" : "false"));
			caps.insert(std::make_pair("ULTIMATE", ultimate ? "true" : "false"));
			caps.insert(std::make_pair("ACEROUND", aceRound ? "true" : "false"));

			if(aiOpponent) caps.insert(std::make_pair("AI_NAME", aiNames[0]));

			std::ostringstream mvos;
			mvos << static_cast<uint16_t>(con.getMinClientVersion() >> 16)
				 << '.' << static_cast<uint16_t>(con.getMinClientVersion());

			caps.insert(std::make_pair("MIN_VERSION", mvos.str()));

			std::ostringstream mpos;
			mpos << (aiOpponent ? numAI + 1 : minPlayers);
			caps.insert(std::make_pair("MAX_PLAYERS", mpos.str()));

			std::ostringstream cpos;
			cpos << game.getPlayerCount();
			caps.insert(std::make_pair("CUR_PLAYERS", cpos.str()));

			con.setCapabilities(caps);

			while(!interrupt) {

				timeval tv = { 1, 0 };

				int r;

				if((r = con.wait((game.getPlayerCount() > 0 && !aiOpponent) ? &tv : NULL)) > 0) {

					Server::Connection::INFO info;
					Server::Connection::ACCEPT_STATE state = Server::Connection::NONE;

					if(!interrupt) {

						try {
							state = con.accept(info, refuse);
						} catch(const Common::Exception::SocketException &e) {
							logDebug("Client accept failed: " << e.what());
							state = Server::Connection::NONE;
						}

						if(state == Server::Connection::PLAY) {

							logInfo(NetMauMau::Common::Logger::time(TIMEFORMAT) <<
									"Connection from " << info.host << ":" << info.port << " as \""
									<< info.name << "\" (" << info.maj << "." << info.min << ") "
									<< NetMauMau::Common::Logger::nonl());

							Server::Game::COLLECT_STATE cs =
								game.collectPlayers(minPlayers, new Server::Player(info.name,
													info.sockfd, con));

							if(cs == Server::Game::ACCEPTED || cs == Server::Game::ACCEPTED_READY) {

								logger("accepted");
								updatePlayerCap(caps, game.getPlayerCount(), con, aiOpponent);

								if(cs == Server::Game::ACCEPTED_READY) {

									refuse = true;
									game.start(ultimate);
									updatePlayerCap(caps, game.getPlayerCount(), con, aiOpponent);
									refuse = false;
								}

							} else {
								logger("refused");
							}
						}
					}
				} else if(r == -2) {
					game.reset(true);
				}

				if(interrupt) game.shutdown();

				updatePlayerCap(caps, game.getPlayerCount(), con, aiOpponent);
			}

		} catch(const Common::Exception::SocketException &e) {
			logError(e.what());
			return EXIT_FAILURE;
		}

#ifndef _WIN32
	} else {
		logError("Changing user/group failed" << (dpErr ? ": " : "") << (dpErr ? dpErr : ""));
	}

#endif

	return EXIT_SUCCESS;
}

// kate: indent-mode cstyle; indent-width 4; replace-tabs off; tab-width 4; 
