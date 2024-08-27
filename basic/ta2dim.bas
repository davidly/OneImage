10 dim aa%( 3, 5 )
11 aa%( 0, 0 ) = 1
12 aa%( 0, 1 ) = 2
13 aa%( 0, 2 ) = 3
14 aa%( 1, 0 ) = 4
15 aa%( 1, 1 ) = 5
16 aa%( 1, 2 ) = 6
17 aa%( 2, 0 ) = 7
18 aa%( 2, 1 ) = 8
19 aa%( 2, 2 ) = 9

100 gosub 400

200 for a% = 0 to 2
210 for b% = 0 to 4
220 aa%( a%, b% ) = a% * b%
230 next b%
240 next a%

250 gosub 400
260 system

400 for a% = 0 to 2
410 for b% = 0 to 4
420 print "a "; a%; " b "; b%; " aa(a,b) "; aa%(a%,b%)
430 next b%
440 next a%
450 return

