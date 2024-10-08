1  REM Tic Tac Toe solving app that learns what WOPR learned: you can't win
2  REM Only three starting positions are examined. Others are just reflections of these
3  REM B%   -- The board
4  REM AL%  -- Alpha, for pruning
5  REM BE%  -- Beta, for pruning
6  REM L%   -- Top-level loop iteration
7  REM WI%  -- The winning piece (0 none, 1 X, 2, O )
8  REM RE%  -- Resulting score of 4000/minmax board position. 0 draw, 1 X win, -1 Y win )
9  REM SX%  -- Stack array for "recursion"
10 REM V%   -- Value of a board position
11 REM ST%  -- Stack Pointer. Even for alpha/beta pruning Minimize plys, Odd for Maximize
12 REM R%   -- Current Row where a new piece is played
13 REM C%   -- Current Column where a new piece is played
14 REM RW%  -- Row in the Winner function (2000)
15 REM CW%  -- Column in the Winner function (2000)
16 REM RP%  -- Row when printing the board (1000)
17 REM CA%, RC%, CC% -- result, row, and column in Cat's game detection (3000) (unused)
18 REM MC%  -- Move count total for debugging
19 REM Note: Can't use real recursion with GOSUB because stack is limited to roughly 5 deep
20 DIM B%(3, 3)
22 DIM SR%(10)
23 DIM SC%(10)
24 DIM SV%(10)
26 DIM SA%(10)
27 DIM SB%(10)
29 PRINT "start time: "; TIME$
30 FOR L% = 1 TO 1
31 MC% = 0
42 AL% = -100
43 BE% = 100
44 B%(0, 0) = 1
45 GOSUB 4000
46 REM print "after pass 1: "; mc%
58 AL% = -100
59 BE% = 100
60 B%(0, 0) = 0
61 B%(0, 1) = 1
62 GOSUB 4000
63 REM print "after pass 2: "; mc%
68 AL% = -100
69 BE% = 100
70 B%(0, 1) = 0
71 B%(1, 1) = 1
72 GOSUB 4000
73 B%(1, 1) = 0
80 NEXT L%
85 PRINT "end time for "; l% - 1; " iterations: "; TIME$
86 PRINT "final move count: "; MC%
100 END

999 REM function to print the board
1000 FOR RP% = 0 TO 2
1010 PRINT B%(RP%, 0); " "; B%(RP%, 1); " "; B%(RP%, 2)
1020 NEXT RP%
1025 RETURN

1999 REM function to find a winner returned in WI%. 0 = nobody, 1 = X, 2 = O
2000 FOR RW% = 0 TO 2
2005 WI% = B%(RW%, 0)
2010 IF WI% <> 0 AND WI% = B%(RW%, 1) AND WI% = B%(RW%, 2) THEN RETURN
2015 NEXT RW%
2020 FOR CW% = 0 TO 2
2025 WI% = B%(0, CW%)
2030 IF WI% <> 0 AND WI% = B%(1, CW%) AND WI% = B%(2, CW%) THEN RETURN
2035 NEXT CW%
2040 WI% = B%(1, 1)
2045 IF WI% = 0 THEN RETURN
2050 IF WI% = B%(0, 0) AND WI% = B%(2, 2) THEN RETURN
2055 IF WI% = B%(0, 2) AND WI% = B%(2, 0) THEN RETURN
2060 WI% = 0
2065 rem print "winner " ; wi%
2070 RETURN

2999 REM function to find a cat's game (unused)
3000 CA% = 0
3005 FOR RC% = 0 TO 2
3010 FOR CC% = 0 TO 2
3015 IF 0 = B%(RC%, CC%) THEN RETURN
3020 NEXT CC%
3025 NEXT RC%
3030 CA% = 1
3035 RETURN

4000 REM minmax function to find score of a board position
4001 REM recursion is simulated with GOTOs
4030 ST% = 0
4040 V% = 0
4060 RE% = 0
4100 MC% = MC% + 1
4101 rem print "move count "; mc%; " stack "; st%; " a "; al%; " b "; be%
4102 rem gosub 1000
4104 IF ST% < 4 THEN GOTO 4150
4105 GOSUB 2000
4110 IF WI% = 1 THEN RE% = 1: GOTO 4280
4120 IF WI% = 2 THEN RE% = -1: GOTO 4280
4140 IF ST% = 8 THEN RE% = 0: GOTO 4280
4150 IF ST% AND 1 THEN V% = -100 ELSE V% = 100
4160 R% = 0
4170 C% = 0
4180 IF 0 <> B%(R%, C%) THEN GOTO 4500
4200 IF ST% AND 1 THEN B%(R%, C%) = 1 ELSE B%(R%, C%) = 2
4210 SR%(ST%) = R%
4220 SC%(ST%) = C%
4230 SV%(ST%) = V%
4245 SA%(ST%) = AL%
4246 SB%(ST%) = BE%
4260 ST% = ST% + 1
4270 GOTO 4100
4280 ST% = ST% - 1
4290 R% = SR%(ST%)
4300 C% = SC%(ST%)
4310 V% = SV%(ST%)
4325 AL% = SA%(ST%)
4326 BE% = SB%(ST%)
4328 B%(R%, C%) = 0
4330 IF ST% AND 1 GOTO 4340
4332 IF RE% < V% THEN V% = RE%
4334 IF V% < BE% THEN BE% = V%
4336 IF BE% <= AL% THEN GOTO 4520
4337 IF V% = -1 THEN GOTO 4520
4338 GOTO 4500
4340 IF RE% > V% THEN V% = RE%
4342 IF V% > AL% THEN AL% = V%
4344 IF AL% >= BE% THEN GOTO 4520
4346 IF V% = 1 THEN GOTO 4520
4500 C% = C% + 1
4501 REM IF 0 = ST% THEN PRINT "c ";C%
4505 IF C% < 3 THEN GOTO 4180
4510 R% = R% + 1
4511 REM IF 0 = ST% THEN PRINT "r ";R%
4515 IF R% < 3 THEN GOTO 4170
4520 RE% = V%
4530 IF ST% = 0 THEN RETURN
4540 GOTO 4280

