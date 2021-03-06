Build instructions, binaries and rules
======================================

Prerequisites
-------------

* a GNU GCC compiler (currently NetMauMau uses a certain amount of features exclusive to GCC)

* the [POPT library >= 1.10](http://rpm5.org/files/popt/)

* xxd (from the vim package [on **Debian** based distributions *vim-common*; on **Gentoo** *vim-core*])

* libmagic (*optionally, but recommended*)

* GNU Scientific Library >= 1.9 (*optionally*, for better random number generator)

Setting up the build environment
--------------------------------

NetMauMau prefers *automake-1.11*

After checkout run `env AUTOMAKE=automake-1.11 ACLOCAL=aclocal-1.11 autoreconf -ifv` to set up the build environment.

`./configure && make` builds the projects and `make install` installs it.
See `configure --help` for more options and help.

Client
------

A proof of concept Qt client can be found at
[https://github.com/velnias75/NetMauMau-Qt-Client](https://github.com/velnias75/NetMauMau-Qt-Client)


Binary releases
===============

Gentoo
------
NetMauMau is available on Gentoo via the overlay **games-overlay** which
can be added by `layman`
The GitHub repository of **games-overlay** is here:
[https://github.com/hasufell/games-overlay](https://github.com/hasufell/games-overlay)

**Adding the overlay**

With paludis: see [Paludis repository configuration](http://paludis.exherbo.org/configuration/repositories/index.html)

With layman:
```layman -f -o https://raw.github.com/hasufell/games-overlay/master/repository.xml -a games-overlay``` or ```layman -a games-overlay```

Install NetMauMau with `emerge games-server/netmaumau`

Ubuntu
------
Binary packages are available for Precise, Trusty, Utopic and Vivid
in my Launchpad PPA at
[https://launchpad.net/~velnias/+archive/ubuntu/velnias](https://launchpad.net/~velnias/+archive/ubuntu/velnias)

Add the repository to your system:

`sudo add-apt-repository ppa:velnias/velnias`

Debian 7
--------
[http://download.opensuse.org/repositories/home:/velnias/Debian_7.0](http://download.opensuse.org/repositories/home:/velnias/Debian_7.0)


Windows
-------
[https://sourceforge.net/projects/netmaumau/](https://sourceforge.net/projects/netmaumau/)


Rules
=====

Following are the rules currently implemented in NetMauMau:

General rules
-------------

At the beginning every player gets five random cards out of a set of 32
cards. Of the remaining cards one card's front is visible to all players and
the rest serves as pool of cards to take if a player cannot play a card or
if the player has to take cards due to the *Seven* rule. If all pool cards
are either in players hands or played out all cards except the visible card
are shuffled again and made available as pool again.

The aim of NetMauMau is to get away all cards as fast as possible. To achieve
this a player can play out any card of either the same rank or the same
suit. If a player cannot play out any card the player has to take one from the
pool and to suspend. Some cards trigger specific actions as described below.

If a player has lost the points of the player's cards are summed up. If the
player has lost due a played out Jack, the points are doubled. The higher
that value the worse the game is lost.

Specific card rules
-------------------

All rules apply also to the visible card at the beginning of the game for
the first player.

* **Seven** (1 point)

   if a *Seven* is played out than the next player has either to take two
   more cards or play out another *Seven*. In that case the next player has
   either to take plus two (i.e. four) cards or can also play out a *Seven*
   and so forth. At maximum one player has to take eight cards if a sequence
   of four *Seven* are played out

* **Eight** (2 points)

   if an *Eight* is played out, the next player has to suspend and the next
   player can play a card. The player has **not** to take an extra card. An
   *Eight* played before takes precedence, i.e. even if the next player has
   an *Eight*, the player has to suspend

* **Nine** (3 points)

   there is no special rule for that rank

* **Ten** (4 points)

   there is no special rule for that rank

* **Queen** (5 points)

   there is no special rule for that rank

* **King** (6 points)

   there is no special rule for that rank

* **Ace** (11 points)

   if the **Ace rounds** are enabled (`--ace-round|-a`), a player can, if there
   are two or more *Aces* on hand, start an **Ace round**. Within the 
   **Ace round**  other players can only play out *Aces* or have to suspend and 
   take a card. The player can stop the **Ace round** at any time. The 
   **Ace round** is also stopped if the player plays the last *Ace*

* **Jack** (20 points)

   if a *Jack* of any suite is played out, the player can wish a new suit. A
   *Jack* can get played over any card except another *Jack*. An *Eight*
   played before takes precedence, i.e. even if the next player has a *Jack*,
   the player has to suspend
