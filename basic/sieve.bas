10 SI% = 8190
20 DIM FL%(8191)
40 FOR IT% = 0 to 10
50 CO% = 0
60 FOR I% = 0 TO 8190
70 FL%(I%) = 1
80 NEXT I%
90 FOR I% = 0 TO 8190
100 IF 0 = FL%(I%) goto 180
105 PR% = I% + I% + 3
120 K% = I% + PR%
130 IF K% > SI% goto 170
140 FL%(K%) = 0
150 K% = K% + PR%
160 GOTO 130
170 CO% = CO% + 1
180 NEXT I%
190 NEXT IT%
200 PRINT CO%;" PRIMES"
210 PRINT "ITERATIONS: "; IT% - 1
