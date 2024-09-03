.skip 10

.skip 0x03

.section sekcija0

xchg %r2, %r6

jmp 5

add %r7, %r2

ld $h, %r5

.extern y, z

.global b,c , d

ld $h, %r8

ld $5, %r6

.global g

ld $2, %r2

h:

.section sekcija1

.word 5, 171, h

ld $k, %r13

ld $0x12, %r12

jmp k

add %r7, %r2

sub %r6, %r3

.section sekcija2

k:

ld $h, %r11

mul %r5, %r4

div %r10, %r11

.end
