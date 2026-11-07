/* LineCount.rexx: the file in GFOpenEdit's current tab and where the
   caret is. rx LineCount.rexx   (GFOpenEdit running) */
OPTIONS RESULTS
IF ~SHOW('P', 'GFOPENEDIT') THEN DO
    SAY 'GFOpenEdit is not running.'
    EXIT 5
END
ADDRESS 'GFOPENEDIT'
'VERSION'
SAY RESULT
'GETFILE'
SAY 'File:' RESULT
'GETPOS'
PARSE VAR RESULT line col
SAY 'Caret: line' line', column' col
