;-----------------------------------------------------------------------------
; MICROFS_ASM.S - Funciones auxiliares de MicroFS en assembler
; Funciones simples optimizadas para reducir tamaño de ROM
;-----------------------------------------------------------------------------

        .export _mfs_streq
        .export _mfs_strcpy_n

        .import pushax, popax
        .importzp ptr1, ptr2, tmp1

;-----------------------------------------------------------------------------
; Código
;-----------------------------------------------------------------------------
        .code

;-----------------------------------------------------------------------------
; _mfs_streq - Comparar dos strings
; uint8_t mfs_streq(const char *a, const char *b)
; Retorna: 1 si iguales, 0 si diferentes
;-----------------------------------------------------------------------------
_mfs_streq:
        sta     ptr2            ; b low
        stx     ptr2+1          ; b high
        jsr     popax
        sta     ptr1            ; a low
        stx     ptr1+1          ; a high
        
        ldy     #0
@loop:  lda     (ptr1),y
        cmp     (ptr2),y
        bne     @diff
        cmp     #0              ; ¿Fin de string?
        beq     @equal
        iny
        bne     @loop           ; Max 256 chars
@diff:  lda     #0
        ldx     #0
        rts
@equal: lda     #1
        ldx     #0
        rts

;-----------------------------------------------------------------------------
; _mfs_strcpy_n - Copiar string con límite
; void mfs_strcpy_n(char *d, const char *s, uint8_t n)
;-----------------------------------------------------------------------------
_mfs_strcpy_n:
        sta     tmp1            ; n
        jsr     popax
        sta     ptr2            ; s low
        stx     ptr2+1          ; s high
        jsr     popax
        sta     ptr1            ; d low
        stx     ptr1+1          ; d high
        
        ldy     #0
@loop:  lda     tmp1
        beq     @term           ; n == 0?
        dec     tmp1
        lda     (ptr2),y
        beq     @term           ; *s == 0?
        sta     (ptr1),y
        iny
        bne     @loop
@term:  lda     #0
        sta     (ptr1),y        ; Terminar string
        rts
