/* tests/arexx.rexx: GFOpenEdit's ARexx port against ARexx-RULE.md
   (return codes 0/5/10/20, no requester holds up a command).
   rx tests/arexx.rexx   (GFOpenEdit running; prints PASS/FAIL lines, then ALL PASS or n FAILED) */
OPTIONS RESULTS
port = 'GFOPENEDIT'
IF ~SHOW('P', port) THEN DO
    SAY 'FAIL' port 'is not there: start GFOpenEdit first'
    EXIT 20
END
ADDRESS VALUE port
fails = 0

CALL want 'VERSION', 0
CALL want 'HELP', 0
CALL want 'HELP GOTO', 0
CALL want 'HELP NOSUCHCOMMAND', 10
CALL want 'FAULT 205', 0
CALL want 'NOSUCHCOMMAND', 20
CALL want 'LASTERROR', 0
IF res = '' THEN CALL bad 'LASTERROR after an unknown command is empty'
ELSE SAY '     LASTERROR:' res
CALL want 'NEW', 0
CALL want 'INSERT "hello world"', 0
CALL want 'FIND nosuchtext', 5
CALL want 'GOTO 0', 10
CALL want 'OPEN', 10
/* these would have asked: now they answer at once */
CALL want 'OPEN RAM:no/such/file.txt', 10
CALL want 'LASTERROR', 0
SAY '     LASTERROR:' res
CALL want 'SAVE', 10                        /* Untitled: no file requester */
CALL want 'CLOSE', 5                        /* unsaved: no Save? requester */
CALL want 'QUIT', 5                         /* unsaved: stays */
CALL want 'SAVEAS RAM:arexx-test.txt', 0
CALL want 'GETFILE', 0
CALL want 'DEACTIVATE', 5                   /* no icon to go to */
CALL want 'ACTIVATE', 0
CALL want 'QUIT', 0

IF fails = 0 THEN SAY 'ALL PASS'
ELSE SAY fails 'FAILED'
EXIT fails

want: PROCEDURE EXPOSE fails res
    PARSE ARG cmd, rcwant
    res = ''
    cmd
    got = rc
    IF got = 0 THEN res = result
    IF got = rcwant THEN SAY 'PASS' cmd 'RC' got
    ELSE CALL bad cmd 'RC' got '(wanted' rcwant')'
    RETURN

bad:
    PARSE ARG why
    SAY 'FAIL' why
    fails = fails + 1
    RETURN
