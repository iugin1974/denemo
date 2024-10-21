;TransposeOnPlayback
(let ((params TransposeOnPlayback::params)(interval #f))
(define (do-staff interval)
	(d-StaffProperties (string-append "transposition="
			(number->string 
				(+  (string->number interval) 
					(string->number  (d-StaffProperties "query=transposition")))))))

(if (not params)
	(let ((current  (string->number  (d-StaffProperties "query=transposition"))))
		(set! current (number->string (- current)))
		(set! interval (d-GetUserInput (_ "Relative Playback Transposition (relative to current value)") (_ "Give semitones (+/-)") 
			 current ))))
		
(if (number? interval)
	(set! interval (number->string interval)))
(if (string? interval)
	(begin
		(d-SetSaved #f)
		(while (d-StaffUp))
		(do-staff interval)
		(while (d-StaffDown)
			(do-staff interval)))
	(d-WarningDialog (_ "Cancelled"))))
