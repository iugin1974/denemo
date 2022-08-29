;;;;;;TwoTimeSignatures
(let ((tag "TwoTimeSignatures") (num1 #f) (den1 #f) (text ""))
  (define (numerator)
    (car (string-split (d-GetPrevailingTimesig) #\/)))
  (define (denominator)
    (cadr (string-split (d-GetPrevailingTimesig) #\/)))
 (if (d-Directive-timesig? tag)
 	(begin
 		(d-DirectiveDelete-timesig tag)
 		(d-InfoDialog (_ "Double time signature removed")))
 	(begin
 		(while (d-MoveToStaffUp))
 		(d-LilyPondInclude "time-signatures.ily")
		(d-InsertTimeSig)
		(d-MoveCursorLeft)
  		(set! num1 (numerator))
  		(set! den1 (denominator))
  		(set! text (string-append "\\once \\override Staff.TimeSignature #'stencil = #(alternate-time
\"" num1 "\" \"" den1 "\" "))
  		(d-InsertTimeSig)
  		(d-MoveCursorLeft)
  		(set! text (string-append text "\"" (numerator) "\" \"" (denominator) "\")\n"))
 		(d-DirectivePut-timesig-prefix tag text)
  		(d-DirectivePut-timesig-display tag (string-append num1 "/" den1))
  		(d-SetMark)
  		(d-Copy)
  		(while (d-MoveToStaffDown)
  			(d-Paste)))))
