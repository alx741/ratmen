# Copyright 1999-2003 Gentoo Technologies, Inc.
# Distributed under the terms of the GNU General Public License v2
# $Header: $

# x11-misc/ratmen -- Gentoo ebuild file.
#     By Zrajm C Akfohg <ratmen-mail@klingonska.org>
#     [2003-03-29] & [2007-10-25]

# To install, do the following (as root):
#
#     DIR=/usr/local/portage/x11-misc/ratmen
#     mkdir -p $DIR
#     cd $DIR
#     wget http://zrajm.klingonska.org/programs/ratmen/ratmen-2.2.3.ebuild
#     ebuild ratmen-2.2.3.ebuild digest
#     echo x11-misc/ratmen >>/etc/portage/package.keywords
#     emerge -v ratmen
#


DESCRIPTION="Shell command for displaying menus in X"
HOMEPAGE="http://zrajm.klingonska.org/programs/"
SRC_URI="http://zrajm.klingonska.org/programs/${PN}/${P}.tar.gz"
LICENSE=""

SLOT="0"
KEYWORDS="~x86"
IUSE=""
DEPEND="dev-lang/perl" # pod2man used in Makefile is part of perl
#RDEPEND="virtual/x11"
# How do I create a runtime dependency satisfied on any X11?
# Plz tell me, anyone.

src_compile() {
	emake || die
}

src_install() {
	dobin ratmen
	doman ratmen.1
	dodoc ChangeLog README*
}

