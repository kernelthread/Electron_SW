; inflate - uncompress data stored in the DEFLATE format
; by Piotr Fusik <fox@scene.pl>
; Last modified: 2017-11-07
; Modifications by Dennis May 2021:
;   Use ACME assembler instead of xasm
;   Support page-by-page decompression
; Original available from https://github.com/pfusik/zlib6502

; Assemble with acme (https://github.com/meonwax/acme)


; Usable zero page locations
; A8 - AF (unused by OS)
; B8, B9 (.printMessageAddressLow, .printMessageAddressHigh)
; C5 (unused by OS)
; C7 (.tapeInterBlockGap)
; CB (CRC calc clobbers)
; CE, CF (unused by OS)
; F9

    !ifdef DEBUG {
.DECOMPRESSED_PAGE_BUF = $600
.DEBUG_OUT_PTR = $84
}

.SAVED_EOF = $80
.SAVED_X = $81
.RESUME_POINT = $82

inflate_zp = $88

; Pointer to compressed data
inputPointer                    =	$86           ; 2 bytes

; Indexes into single page output buffer
pageBufWriteIndex               =	inflate_zp+0  ; 1 byte
pageBufReadIndex                =	inflate_zp+1  ; 1 byte

; Local variables

getBit_buffer                   =	inflate_zp+2  ; 1 byte
getBits_base                    =	inflate_zp+3  ; 1 byte
inflateStored_pageCounter       =	inflate_zp+3  ; 1 byte

inflateDynamic_symbol           =	inflate_zp+4  ; 1 byte
inflateDynamic_lastLength       =	inflate_zp+5  ; 1 byte
inflateDynamic_tempCodes        =	inflate_zp+5  ; 1 byte

inflateCodes_lengthMinus2       =	inflate_zp+6  ; 1 byte
inflateDynamic_allCodes         =	inflate_zp+6  ; 1 byte
inflateDynamic_primaryCodes     =	inflate_zp+7  ; 1 byte

; Argument values for getBits
GET_1_BIT                       =	$81
GET_2_BITS                      =	$82
GET_3_BITS                      =	$84
GET_4_BITS                      =	$88
GET_5_BITS                      =	$90
GET_6_BITS                      =	$a0
GET_7_BITS                      =	$c0

; Huffman trees
TREE_SIZE                       =	16
PRIMARY_TREE                    =	0
DISTANCE_TREE                   =	TREE_SIZE

; Alphabet
LENGTH_SYMBOLS                  =	1+29+2
DISTANCE_SYMBOLS                =	30
CONTROL_SYMBOLS                 =	LENGTH_SYMBOLS+DISTANCE_SYMBOLS

; Initialize the inflater
ROMFS_INIT_DECOMPRESS
    !ifndef DEBUG {
    LDA     #<ROMFS_DATA
    STA     inputPointer
    LDA     #>ROMFS_DATA
    STA     inputPointer+1
    }
    ldy #0
    sty pageBufWriteIndex
    sty pageBufReadIndex
    sty getBit_buffer
    sty .RESUME_POINT
    sty .SAVED_X
    rts

.ResumeAddrLo
    !byte   <(.inflateResume0-1)
    !byte   <(.inflateResume1-1)
    !byte   <(.inflateResume2-1)
    !byte   <(.inflateResume3-1)
    !byte   <(.inflateResume4-1)
    !byte   <(.inflateResume5-1)
    !byte   <(.inflateResume6-1)
.ResumeAddrHi
    !byte   >(.inflateResume0-1)
    !byte   >(.inflateResume1-1)
    !byte   >(.inflateResume2-1)
    !byte   >(.inflateResume3-1)
    !byte   >(.inflateResume4-1)
    !byte   >(.inflateResume5-1)
    !byte   >(.inflateResume6-1)

inflateEndOfPage1:
    iny
    sty .RESUME_POINT
    rts

; Uncompress DEFLATE stream starting from the address stored in inputPointer
; Writes output to .DECOMPRESSED_PAGE_BUF
; Stop after 256 bytes or end of file, whichever comes first.
ROMFS_DECOMPRESS_PAGE
    ldy .RESUME_POINT
    lda .ResumeAddrHi, y
    pha
    lda .ResumeAddrLo, y
    pha
    ldx .SAVED_X
.inflateResume6
    ldy #0              ; most of the code assumes Y=0
    rts
.inflateResume0
inflate_blockLoop
; Get a bit of EOF and two bits of block type
	sty	getBits_base
	lda	#GET_3_BITS
	jsr	getBits
    sta .SAVED_EOF
    lsr
	bne	inflateCompressed

; Copy uncompressed block
	sty	getBit_buffer  ; ignore bits until byte boundary
	jsr	getWord        ; skip the length we don't need
	jsr	getWord        ; get the one's complement length
	sta	inflateStored_pageCounter
	bcs	inflateStored_firstByte
inflateStored_copyByte
	jsr	getByte
	jsr	storeByte
    beq inflateEndOfPage1
.inflateResume1

inflateStored_firstByte
	inx
	bne	inflateStored_copyByte
	inc	inflateStored_pageCounter
	bne	inflateStored_copyByte

inflate_nextBlock
	lsr .SAVED_EOF
	bcc	inflate_blockLoop
    lda #6
    sta .RESUME_POINT
	rts

inflateCompressed
; A=1: fixed block, initialize with fixed codes
; A=2: dynamic block, start by clearing all code lengths
; A=3: invalid compressed data, not handled in this routine
	eor	#2

inflateCompressed_setCodeLengths
	tax
	beq	inflateCompressed_setLiteralCodeLength
; fixed Huffman literal codes:
; :144 dta 8
; :112 dta 9
	lda	#4
	cpy	#144
	rol
inflateCompressed_setLiteralCodeLength
	sta	literalSymbolCodeLength,y
	beq	inflateCompressed_setControlCodeLength
; fixed Huffman control codes:
; :24  dta 7
; :6   dta 8
; :2   dta 8 ; meaningless codes
; :30  dta 5+DISTANCE_TREE
	lda	#5+DISTANCE_TREE
	cpy	#LENGTH_SYMBOLS
	bcs	inflateCompressed_setControlCodeLength
	cpy	#24
	adc	#2-DISTANCE_TREE
inflateCompressed_setControlCodeLength
	cpy	#CONTROL_SYMBOLS
	bcs +
    sta	controlSymbolCodeLength,y
+
	iny
	bne	inflateCompressed_setCodeLengths

	tax
	bne	inflateCodes

; Decompress a block reading Huffman trees first

; Build the tree for temporary codes
	jsr	buildTempHuffmanTree

; Use temporary codes to get lengths of literal/length and distance codes
inflateDynamic_decodeLength
; C=1: literal codes
; C=0: control codes
	stx	inflateDynamic_symbol
	php
; Fetch a temporary code
	jsr	fetchPrimaryCode
; Temporary code 0..15: put this length
	bpl	inflateDynamic_verbatimLength
; Temporary code 16: repeat last length 3 + getBits(2) times
; Temporary code 17: put zero length 3 + getBits(3) times
; Temporary code 18: put zero length 11 + getBits(7) times
	tax
	jsr	getBits
	cpx	#GET_3_BITS
	bcc	inflateDynamic_repeatLast
	beq +
    adc	#7
+
	sty	inflateDynamic_lastLength
inflateDynamic_repeatLast
	tay
	lda	inflateDynamic_lastLength
	iny
    iny
inflateDynamic_verbatimLength
	iny
	plp
	ldx	inflateDynamic_symbol
inflateDynamic_storeLength
	bcc	inflateDynamic_controlSymbolCodeLength
	sta	literalSymbolCodeLength,x
    inx
	cpx	#1
inflateDynamic_storeNext
	dey
	bne	inflateDynamic_storeLength
	sta	inflateDynamic_lastLength
	beq	inflateDynamic_decodeLength
inflateDynamic_controlSymbolCodeLength
	cpx	inflateDynamic_primaryCodes
	bcc	inflateDynamic_storeControl
; the code lengths we skip here were zero-initialized
; in inflateCompressed_setControlCodeLength
	bne +
    ldx	#LENGTH_SYMBOLS
+
	ora	#DISTANCE_TREE
inflateDynamic_storeControl
	sta	controlSymbolCodeLength,x
    inx
	cpx	inflateDynamic_allCodes
	bcc	inflateDynamic_storeNext
	dey

; Decompress a block
inflateCodes
	jsr	buildHuffmanTree
	beq	inflateCodes_loop
inflateCodes_literal
	jsr	storeByte
    beq inflateEndOfPage2
.inflateResume2

inflateCodes_loop
	jsr	fetchPrimaryCode
	bcc	inflateCodes_literal
    bne +
	jmp	inflate_nextBlock
+
; Copy sequence from look-behind buffer
	sty	getBits_base
	cmp	#9
	bcc	inflateCodes_setSequenceLength
	tya
	cpx	#1+28
	bcs	inflateCodes_setSequenceLength
	dex
	txa
	lsr
	ror	getBits_base
	inc	getBits_base
	lsr
	rol	getBits_base
	jsr	getAMinus1BitsMax8
	adc	#0
inflateCodes_setSequenceLength
	sta	inflateCodes_lengthMinus2
	ldx	#DISTANCE_TREE
	jsr	fetchCode
	cmp	#4
	bcc	inflateCodes_setOffsetLowByte
	inc	getBits_base
	lsr
	jsr	getAMinus1BitsMax8
inflateCodes_setOffsetLowByte
	eor	#$ff
    clc
    adc pageBufWriteIndex
	sta	pageBufReadIndex

	lda	getBits_base
	cpx	#10
	bcs	.inflateDistanceTooBig
	jsr	copyByte
    beq inflateEndOfPage3
.inflateResume3
	jsr	copyByte
    beq inflateEndOfPage4
.inflateResume4
inflateCodes_copyByte
	jsr	copyByte
    beq inflateEndOfPage5
.inflateResume5
	dec	inflateCodes_lengthMinus2
	bne	inflateCodes_copyByte
	beq	inflateCodes_loop

inflateEndOfPage5:
    iny
inflateEndOfPage4:
    iny
inflateEndOfPage3:
    iny
inflateEndOfPage2:
    iny
    iny
    sty .RESUME_POINT
    rts

.inflateDistanceTooBig
    brk
    brk
    !text "??INFLATECODE??"
    brk
    brk

; Get dynamic block header and use it to build the temporary tree
buildTempHuffmanTree
; numberOfPrimaryCodes = 257 + getBits(5)
; numberOfDistanceCodes = 1 + getBits(5)
; numberOfTemporaryCodes = 4 + getBits(4)
	ldx	#3
inflateDynamic_getHeader
	lda	inflateDynamic_headerBits-1,x
	jsr	getBits
	adc	inflateDynamic_headerBase-1,x
	sta	inflateDynamic_tempCodes-1,x
	dex
	bne	inflateDynamic_getHeader

; Get lengths of temporary codes in the order stored in inflateDynamic_tempSymbols
inflateDynamic_getTempCodeLengths
	lda	#GET_3_BITS
	jsr	getBits
	ldy	inflateDynamic_tempSymbols,x
	sta	literalSymbolCodeLength,y
	ldy	#0
	inx
	cpx	inflateDynamic_tempCodes
	bcc	inflateDynamic_getTempCodeLengths


; Build Huffman trees basing on code lengths (in bits)
; stored in the *SymbolCodeLength arrays
buildHuffmanTree
; Clear nBitCode_literalCount, nBitCode_controlCount
	tya
-
	sta nBitCode_clearFrom,y
    iny
    bne -
; Count number of codes of each length
buildHuffmanTree_countCodeLengths
	ldx	literalSymbolCodeLength,y
	inc	nBitCode_literalCount,x
	bne +
    stx	allLiteralsCodeLength
+
	cpy	#CONTROL_SYMBOLS
	bcs	buildHuffmanTree_noControlSymbol
	ldx	controlSymbolCodeLength,y
	inc	nBitCode_controlCount,x
buildHuffmanTree_noControlSymbol
	iny
	bne	buildHuffmanTree_countCodeLengths
; Calculate offsets of symbols sorted by code length
	ldx	#-4*TREE_SIZE
buildHuffmanTree_calculateOffsets
	sta	nBitCode_literalOffset+4*TREE_SIZE-$100,x
    clc
	adc	nBitCode_literalCount+4*TREE_SIZE-$100,x
	inx
	bne	buildHuffmanTree_calculateOffsets
; Put symbols in their place in the sorted array
buildHuffmanTree_assignCode
	tya
	ldx	literalSymbolCodeLength,y
	ldy nBitCode_literalOffset,x
	inc	nBitCode_literalOffset,x
	sta	codeToLiteralSymbol,y
	tay
	cpy	#CONTROL_SYMBOLS
	bcs	buildHuffmanTree_noControlSymbol2
	ldx	controlSymbolCodeLength,y
	ldy	nBitCode_controlOffset,x
	inc	nBitCode_controlOffset,x
	sta	codeToControlSymbol,y
	tay
buildHuffmanTree_noControlSymbol2
	iny
	bne	buildHuffmanTree_assignCode
; return with Y=0
	rts

; Read Huffman code using the primary tree
fetchPrimaryCode
	ldx	#PRIMARY_TREE
; Read a code from input using the tree specified in X,
; return low byte of this code in A,
; return C flag reset for literal code, set for length code
fetchCode
	tya
fetchCode_nextBit
	jsr	getBit
	rol
	inx
	bcs	fetchCode_ge256
; are all 256 literal codes of this length?
	cpx	allLiteralsCodeLength
	beq	fetchCode_allLiterals
; is it literal code of length X?
    sec
	sbc	nBitCode_literalCount,x
	bcs	fetchCode_notLiteral
; literal code
	adc	nBitCode_literalOffset,x
	tax
	lda	codeToLiteralSymbol,x
fetchCode_allLiterals
	clc
	rts
; code >= 256, must be control
fetchCode_ge256
	sbc	nBitCode_literalCount,x
	sec
; is it control code of length X?
fetchCode_notLiteral
	sbc	nBitCode_controlCount,x
	bcs	fetchCode_nextBit
; control code
	adc	nBitCode_controlOffset,x
	tax
	lda	codeToControlSymbol,x
	and	#$1f	; make distance symbols zero-based
	tax
	rts

; Read A minus 1 bits, but no more than 8
getAMinus1BitsMax8
	rol	getBits_base
	tax
	cmp	#9
	bcs	getByte
	lda	getNPlus1Bits_mask-2,x
getBits
	jsr	getBits_loop
getBits_normalizeLoop
	lsr	getBits_base
	ror
	bcc	getBits_normalizeLoop
	rts

; Read 16 bits into XA
; return with C=1
getWord
	jsr	getByte
	tax
; Read 8 bits into A
getByte
	lda	#$80
getBits_loop
	jsr	getBit
	ror
	bcc	getBits_loop
	rts

; Read one bit, return in the C flag
; assumes Y=0
getBit
	lsr	getBit_buffer
	bne	getBit_return
	pha
	lda	(inputPointer),y
	inc	inputPointer
    bne +
	inc	inputPointer+1
+
	sec
	ror
	sta	getBit_buffer
	pla
getBit_return
	rts

; Copy a previously written byte
; Must return with Y=0
copyByte
	ldy	pageBufReadIndex
    lda .DECOMPRESSED_PAGE_BUF, Y
    iny
	sty	pageBufReadIndex
; Write a byte
storeByte
    ldy pageBufWriteIndex
    sta .DECOMPRESSED_PAGE_BUF, Y
    iny
    sty pageBufWriteIndex
	ldy	#0
    !ifdef DEBUG {
    sta (.DEBUG_OUT_PTR),y
    inc .DEBUG_OUT_PTR
    bne +
    inc .DEBUG_OUT_PTR+1
+
    }
    stx .SAVED_X
    lda pageBufWriteIndex
	rts


getNPlus1Bits_mask
	!byte	GET_1_BIT,GET_2_BITS,GET_3_BITS,GET_4_BITS,GET_5_BITS,GET_6_BITS,GET_7_BITS

inflateDynamic_tempSymbols
	!byte	GET_2_BITS,GET_3_BITS,GET_7_BITS,0,8,7,9,6,10,5,11,4,12,3,13,2,14,1,15

inflateDynamic_headerBits
	!byte	GET_4_BITS,GET_5_BITS,GET_5_BITS
inflateDynamic_headerBase
	!byte	3,LENGTH_SYMBOLS,0

.CODE_LOC = *

; Data for building trees
; CodeLengthTable 318 bytes (0B00-0C3D)
	* =	$0B00
literalSymbolCodeLength
	* =	*+256
controlSymbolCodeLength
	* =	*+CONTROL_SYMBOLS

; SymbolTable 318 bytes (0C3E-0D7B)
    * = $0C3E
codeToLiteralSymbol
	* =	*+256
codeToControlSymbol
	* =	*+CONTROL_SYMBOLS

; Huffman trees
; 129 bytes (077F-07FF)
    * = $077F
nBitCode_clearFrom
nBitCode_literalCount
	* =	*+2*TREE_SIZE
nBitCode_controlCount
	* =	*+2*TREE_SIZE
nBitCode_literalOffset
	* =	*+2*TREE_SIZE
nBitCode_controlOffset
	* =	*+2*TREE_SIZE
allLiteralsCodeLength
	* =	*+1

    * = .CODE_LOC
