;;;GenerateMusicSignatureSearchScript with movement number to search in
(use-modules (ice-9 ftw))
(d-GoToPosition #f 1 1 1)
(let ((current (d-GetNoteFromTopAsMidi))
	(source (d-GetFilename))
	(movement-number "1") (skips (d-GetUserInput (_"Create Music Signature")
				(_ "Give number of matching scores to skip: ") "0"))
        (count (string->number (d-GetUserInput (_"Create Music Signature") 
												(_ "Give number of notes to match: ") "5")))
        (startdir #f)
        (theproc #f)
        (sig "")
        (test #f)
        (found #f)
        (next #f))
        
      (define (theproc filename statinfo flag) ;(disp "searching file " filename "\nwith flag " flag "\n")
        (d-KeepAlive)
	    (if (and (not (equal? filename source)) (eq? flag 'regular) (or (string-suffix? ".denemo" filename) (string-suffix? ".denemo.gz" filename)));;; also check for .denemo or .denemo.gz in filename
            (let ((status 0))
                (set! status  (d-System 
                    (string-append (string-append "\"" DENEMO_BIN_DIR  "/denemo" "\"")
                    " -n " 
                    " -a \"" (string-append "(define DenemoSearchMovement " movement-number ") (define DenemoMusicSignature '(" sig "))")
                    "\" -i " (string-append "\"" DENEMO_ACTIONS_DIR "checkMusicSignature.scm" "\"") 
                    " \"" filename "\""))) (disp "Returned " status "\n")
                (if (zero? status)
                    (begin
						(if (zero? skips)
							(begin
								(d-OpenNewWindow filename)
								(d-GoToPosition (string->number movement-number) 1 1 1)
								(set! found #t)
								#f)
							(begin
								(set! skips (1- skips))
								#t)))
                    (begin
						;(disp "File " filename " is not a match - continue\n")
						#t)) ;not a match, continue traversal
			)
            (begin
            #t))) ;not a file, continue traversal
;;;;actual procedure        
    (if (not current)
        (d-NextNote))
    (set! current (d-GetNoteFromTopAsMidi))
    (while (and (d-NextNote) (> count 0)) 
        (set! next (d-GetNoteFromTopAsMidi))
        (set! test (- next current))
        (set! sig (string-append sig " " (number->string test)))
        (set! current next)
        (set! count (1- count)))
  (set! startdir (d-ChooseDirectory "Where to search" DENEMO_HOME_DIR '() ))
  (set! movement-number (d-GetUserInput (_ "Search for Similar") (_"Which movement to search in (0 for all movements)?") "1"))
  (if (not movement-number)
    (set! movement-number "1"))
  (if (string? skips)
		(begin
			(set! skips (string->number skips))
			(if (and skips (negative? skips))
				(set! skips 0)))
		(set! skips 0))
  (d-InfoDialog "Searching ... the display will be very sluggish!")
  (ftw startdir theproc)
  (if found
    (if (equal? "0" movement-number)
		(d-InfoDialog (_ "This score has a movement that starts with the same intervals"))
		(d-InfoDialog (_ "This score starts with the same intervals at the given movement number")))
    (d-InfoDialog (string-append "No (other) score in or below " startdir " starts with the same intervals"))))
