/* _Types.pmod
 *
 * Kludge for pike-0.5
 *
 * $Id: _Types.pmod,v 1.3 1998/04/19 00:31:08 grubba Exp $
 */

/*
 *    Protocols.X, a Pike interface to the X Window System
 *
 *    Copyright (C) 1998, Niels Möller, Per Hedbor, Marcus Comstedt,
 *    Pontus Hagland, David Hedbor.
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA 
 */

/* Questions, bug fixes and bug reports can be sent to the pike
 * mailing list, pike@idonex.se, or to the athors (see AUTHORS for
 * email addresses. */

program pixmap_class;

program get_pixmap_class() { return pixmap_class; }

void set_pixmap_class(program p) { pixmap_class = p; }
