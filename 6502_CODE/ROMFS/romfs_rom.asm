; Top level file for Acorn MOS ROMs containing a ROMFS file system.
;
; Copyright 2021, Dennis May
; First Published 2021
;
; This file is part of Miscellaneous Electron Software.
;
; Miscellaneous Electron Software is free software: you can redistribute it
; and/or modify it under the terms of the GNU General Public License as
; published by the Free Software Foundation, either version 3 of the License, or
; (at your option) any later version.
;
; Miscellaneous Electron Software is distributed in the hope that it will be
; useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
;
; You should have received a copy of the GNU General Public License along with
; Miscellaneous Electron Software.  If not, see <https://www.gnu.org/licenses/>.
;

	!source "rom_skeleton.asm"
    !ifdef ROMFS_COMPRESSED_DATA {
	!source "romfs_inflate.asm"
}

ROMFS_DATA
    !ifndef ROMFS_PRELIMINARY_BUILD {
	!binary "_romfs_image.bin"
}
ROMFS_DATA_END

    !align $3FFF, 0, $FF
