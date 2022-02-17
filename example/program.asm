    processor 6502

    org $0000,0
    ds $8000

    org $8000
start:
    lda #1
    adc #2
    sta 0
trap:
    jmp trap

    org $FFFA
    dc.w start
    dc.w start
    dc.w start