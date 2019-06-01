# This is a bitmap compressor for the Bornhack Badge 2019
# Copyright (C) 2019  Emil Renner Berthing
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

MAKEFLAGS  = -rR

NAME       = compress
CC         = $(CROSS_COMPILE)gcc -std=c99
AS         = $(CROSS_COMPILE)gcc
INSTALL    = install
RM_F       = rm -f

IN         = avatar.bmp
OUT        = /dev/null

OPT        = -O2
CFLAGS     = $(OPT) -pipe -Wall -Wextra
LIBS       =
LDFLAGS    = $(OPT) -Wl,-O1,--gc-sections

prefix     = /usr/local
bindir     = $(prefix)/bin

ifdef V
E=@\#
Q=
else
E=@echo
Q=@
endif

.PHONY: all release clean install
.PRECIOUS: %.o

all: $(NAME)
	$E '  COMPRESS < $(IN) > $(OUT)'
	$Q'./$(NAME)' < '$(IN)' > '$(OUT)'

release: CPPFLAGS += -DNDEBUG
release: $(NAME)

clean:
	$E '  RM      $(NAME)'
	$Q$(RM_F) ./$(NAME)

install: $(DESTDIR)$(bindir)/$(NAME)

$(NAME): $(NAME).c display.h $(MAKEFILE_LIST)
	$E '  CC       $@'
	$Q$(CC) -o '$@' $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) '$<' $(LIBS)

$(DESTDIR)$(bindir)/:
	$E '  INSTALL  $@'
	$Q$(INSTALL) -dm755 '$@'

$(DESTDIR)$(bindir)/$(NAME): $(NAME) | $(DESTDIR)$(bindir)/
	$E '  INSTALL  $@'
	$Q$(INSTALL) -m755 '$<' '$@'

