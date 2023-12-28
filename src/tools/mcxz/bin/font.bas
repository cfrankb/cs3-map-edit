SCREEN 13
COLOR 15
CLS
DEF SEG = &HA000

OPEN "data.bin" FOR OUTPUT AS #1
FOR c = 32 TO 126
   LOCATE 1, 1
   PRINT CHR$(c)
   FOR y = 0 TO 7
      FOR x = 0 TO 7
         i = PEEK(x + y * 320)
         PRINT #1, CHR$(i);
      NEXT
   NEXT
   
NEXT
CLOSE #1




