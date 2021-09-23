; Skeleton for Acorn MOS ROMs
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

ROM_BASE        = $8000

.MOS_SAVED_A    = $EF
.MOS_SAVED_X    = $F0
.MOS_SAVED_Y    = $F1
.MOS_TEXT_PTR   = $F2
.PAGEREGSHADOW  = $F4
.MOS_FS_ROM_ID  = $F5
.MOS_ROMFS_PTR  = $F6
.SRVC_A         = $F8

    !ifdef ROMFS_COMPRESSED_DATA {
; Usable zero page locations
; A8 - AF (unused by OS)
; B8, B9 (.printMessageAddressLow, .printMessageAddressHigh)
; C5 (unused by OS)
; C7 (.tapeInterBlockGap)
; CB (CRC calc clobbers)
; CE, CF (unused by OS)
; F9
.CURRENT_ROMFS_PAGE = $C5

; Buffer used to hold 256 bytes of decompressed data
.DECOMPRESSED_PAGE_BUF = $900
}

OSRDRM          = $FFB9

    !ifndef .ABS_FS_SPACE_CLAIM {
.ABS_FS_SPACE_CLAIM = .SERVICE_DUMMY
}

    !ifndef .RELATIVE_SPACE_CLAIM {
.RELATIVE_SPACE_CLAIM = .SERVICE_DUMMY
}

    !ifndef .AUTO_BOOT_CALL {
.AUTO_BOOT_CALL = .SERVICE_DUMMY
}

    !ifndef .UNKNOWN_OSCLI {
.UNKNOWN_OSCLI = .SERVICE_DUMMY
}

    !ifndef .UNKNOWN_IRQ {
.UNKNOWN_IRQ = .SERVICE_DUMMY
}

    !ifndef .BRK_HANDLER {
.BRK_HANDLER = .SERVICE_DUMMY
}

    !ifdef .HANDLE_UNKNOWN_OSBYTE {
.UNKNOWN_OSBYTE = .UNKNOWN_OSBYTE_HANDLER
} else {
.UNKNOWN_OSBYTE = .SERVICE_DUMMY
}

    !ifdef .HANDLE_UNKNOWN_OSWORD {
.UNKNOWN_OSWORD = .UNKNOWN_OSWORD_HANDLER
} else {
.UNKNOWN_OSWORD = .SERVICE_DUMMY
}

    !ifdef INCLUDE_ROMFS_DATA {
.ROMFS_INIT = .ROMFS_INIT_HANDLER
.ROMFS_GET_BYTE = .ROMFS_GET_BYTE_HANDLER
} else {
.ROMFS_INIT = .SERVICE_DUMMY
.ROMFS_GET_BYTE = .SERVICE_DUMMY
}

    !ifndef .HELP_COMMAND {
.HELP_COMMAND = .SERVICE_DUMMY
}

    !ifndef .CLAIM_ABSOLUTE_WORKSPACE {
.CLAIM_ABSOLUTE_WORKSPACE = .SERVICE_DUMMY
}

    !ifndef .NMI_RELEASE {
.NMI_RELEASE = .SERVICE_DUMMY
}

    !ifndef .NMI_CLAIM {
.NMI_CLAIM = .SERVICE_DUMMY
}

    !ifndef .VECTORS_CLAIMED {
.VECTORS_CLAIMED = .SERVICE_DUMMY
}

    !ifndef .SPOOL_EXEC_CLOSURE {
.SPOOL_EXEC_CLOSURE = .SERVICE_DUMMY
}

    !ifndef .FONT_IMPL_EXPL {
.FONT_IMPL_EXPL = .SERVICE_DUMMY
}

    !ifndef .INIT_FILE_SYS {
.INIT_FILE_SYS = .SERVICE_DUMMY
}


    * = ROM_BASE

    JMP     LANGUAGE_ENTRY
    JMP     SERVICE_ENTRY

    !BYTE   $82
    !BYTE   .COPYRIGHT-ROM_BASE
    !BYTE   ROM_VERSION
    !BINARY "title.txt"
    !BYTE   0
    !BINARY "version_string.txt"
.COPYRIGHT
    !BYTE   0
    !BINARY "copyright.txt"
    !BYTE   0

SERVICE_ENTRY
    STA     .SRVC_A
    CMP     #$13
    BCS     .SERVICE_EXIT
    ASL
    TAX
    JSR     .SERVICE_DISPATCH
.SERVICE_EXIT
    LDA     .SRVC_A
    LDX     .PAGEREGSHADOW
    RTS
.SERVICE_DISPATCH
    LDA     .SERVICE_DISPATCH_TABLE+1,X
    PHA
    LDA     .SERVICE_DISPATCH_TABLE,X
    PHA
.SERVICE_DUMMY
    RTS

.SERVICE_DISPATCH_TABLE
    !16     .SERVICE_DUMMY-1
    !16     .ABS_FS_SPACE_CLAIM-1
    !16     .RELATIVE_SPACE_CLAIM-1
    !16     .AUTO_BOOT_CALL-1
    !16     .UNKNOWN_OSCLI-1
    !16     .UNKNOWN_IRQ-1
    !16     .BRK_HANDLER-1
    !16     .UNKNOWN_OSBYTE-1
    !16     .UNKNOWN_OSWORD-1
    !16     .HELP_COMMAND-1
    !16     .CLAIM_ABSOLUTE_WORKSPACE-1
    !16     .NMI_RELEASE-1
    !16     .NMI_CLAIM-1
    !16     .ROMFS_INIT-1
    !16     .ROMFS_GET_BYTE-1
    !16     .VECTORS_CLAIMED-1
    !16     .SPOOL_EXEC_CLOSURE-1
    !16     .FONT_IMPL_EXPL-1
    !16     .INIT_FILE_SYS-1

    !ifndef ROM_HAS_LANGUAGE_ENTRY {
LANGUAGE_ENTRY
    JMP     LANGUAGE_ENTRY
}

    !ifdef .HANDLE_UNKNOWN_OSBYTE {
.UNKNOWN_OSBYTE_HANDLER
    LDA     .MOS_SAVED_A
    SEC
    SBC     #.FIRST_HANDLED_OSBYTE
    CMP     #(.OSBYTE_DISPATCH_TABLE_END - .OSBYTE_DISPATCH_TABLE)/2
    BCS     .UNKNOWN_OSBYTE_HANDLER_EXIT
    ASL
    TAX
    JSR     .UNKNOWN_OSBYTE_DISPATCH
    LDA     #0
    STA     .SRVC_A
.UNKNOWN_OSBYTE_HANDLER_EXIT
    RTS
.UNKNOWN_OSBYTE_DISPATCH
    LDA     .OSBYTE_DISPATCH_TABLE+1,X
    PHA
    LDA     .OSBYTE_DISPATCH_TABLE,X
    PHA
.OSBYTE_DUMMY
    RTS
}

    !ifdef .HANDLE_UNKNOWN_OSWORD {
.UNKNOWN_OSWORD_HANDLER
    LDA     .MOS_SAVED_A
    SEC
    SBC     #.FIRST_HANDLED_OSWORD
    CMP     #(.OSWORD_DISPATCH_TABLE_END - .OSWORD_DISPATCH_TABLE)/2
    BCS     .UNKNOWN_OSWORD_HANDLER_EXIT
    ASL
    TAX
    JSR     .UNKNOWN_OSWORD_DISPATCH
    LDA     #0
    STA     .SRVC_A
.UNKNOWN_OSWORD_HANDLER_EXIT
    RTS
.UNKNOWN_OSWORD_DISPATCH
    LDA     .OSWORD_DISPATCH_TABLE+1,X
    PHA
    LDA     .OSWORD_DISPATCH_TABLE,X
    PHA
.OSWORD_DUMMY
    RTS
}

    !ifdef INCLUDE_ROMFS_DATA {
.ROMFS_INIT_HANDLER
    JSR     .INVMOSROMID
    CMP     .PAGEREGSHADOW
    BCC     .ROMFS_INIT_HANDLER0

    !ifdef ROMFS_USE_OFFSET_ADDRESSES {
    LDA     #0
    STA     .MOS_ROMFS_PTR
    STA     .MOS_ROMFS_PTR+1
    !ifdef ROMFS_COMPRESSED_DATA {
    LDA     #255
    STA     .CURRENT_ROMFS_PAGE
    JSR     ROMFS_INIT_DECOMPRESS
    }
    } else {
    LDA     #<ROMFS_DATA
    STA     .MOS_ROMFS_PTR
    LDA     #>ROMFS_DATA
    STA     .MOS_ROMFS_PTR+1
    }
    LDA     .PAGEREGSHADOW
    JSR     .INVROMNUM
    STA     .MOS_FS_ROM_ID
    JMP     .ROMFS_CLAIM

.INVMOSROMID
    LDA     .MOS_FS_ROM_ID
.INVROMNUM
    EOR     #$FF
    AND     #$0F
.ROMFS_INIT_HANDLER0
    RTS

.ROMFS_GET_BYTE_HANDLER
    !ifndef ROMFS_USE_OFFSET_ADDRESSES {
    TYA
    BMI     .ROMFS_GET_BYTE_HANDLER120
    }
    JSR     .INVMOSROMID
    CMP     .PAGEREGSHADOW
    BNE     .ROMFS_GET_BYTE_HANDLER0

    !ifdef ROMFS_USE_OFFSET_ADDRESSES {
    !ifdef ROMFS_COMPRESSED_DATA {
.ROMFS_CHECK_PAGE
    LDA     .MOS_ROMFS_PTR+1
    CMP     .CURRENT_ROMFS_PAGE
    BEQ     .ROMFS_GET_DECOMPRESSED_BYTE
    JSR     ROMFS_DECOMPRESS_PAGE
    INC     .CURRENT_ROMFS_PAGE
    JMP     .ROMFS_CHECK_PAGE
.ROMFS_GET_DECOMPRESSED_BYTE
    LDY     .MOS_ROMFS_PTR
    LDA     .DECOMPRESSED_PAGE_BUF, Y
    TAY
    } else {
    LDA     .MOS_ROMFS_PTR+1
    PHA
    CLC
    ADC     #>ROMFS_DATA
    STA     .MOS_ROMFS_PTR+1
    LDY     #<ROMFS_DATA
    LDA     (.MOS_ROMFS_PTR),Y
    TAY
    PLA
    STA     .MOS_ROMFS_PTR+1
    }
    } else {
    LDY     #0
    LDA     (.MOS_ROMFS_PTR),Y
    TAY
    }

.ROMFS_GET_BYTE_HANDLER1
    INC     .MOS_ROMFS_PTR
    BNE     .ROMFS_CLAIM
    INC     .MOS_ROMFS_PTR+1
.ROMFS_CLAIM
    LDA     #0
    STA     .SRVC_A
.ROMFS_GET_BYTE_HANDLER0
    RTS

    !ifndef ROMFS_USE_OFFSET_ADDRESSES {
.ROMFS_GET_BYTE_HANDLER120
    JSR     .INVMOSROMID
    TAY
    JSR     OSRDRM
    TAY
    JMP     .ROMFS_GET_BYTE_HANDLER1
    }

    !ifdef ROMFS_COMPRESSED_DATA {
.ROMFS_DECOMPRESS_PAGE
    }
}
