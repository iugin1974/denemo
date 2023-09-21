;;GraceAfter
(let ((tag "GraceAfter"))
(if (d-Directive-chord? tag)
	(d-DirectiveDelete-chord tag)
	(if (d-IsGrace)
		(begin
			(let ((pos (d-GetUserInput (_ "Attach Grace Notes") (_ "Give fractional position") "3/4")))
				(if pos
					(begin
						(set! pos (string->number pos))
						(if (and (exact? pos) (< pos 1) (> pos 0))
							(begin
								
										(while (and (d-IsGrace) (d-MoveCursorLeft)))
										(if ( not (d-IsGrace))
											(if (d-Directive-chord? tag)
												(d-DirectiveDelete-chord tag)
												(begin
													(d-PushPosition)
													(d-DirectivePut-chord-prefix tag (string-append "\\attachGraces #" (number->string pos) " "))
													(d-DirectivePut-chord-display tag (_ "Grace(s) Attached"))
													(d-DirectivePut-chord-ty tag -40)
													(d-DirectivePut-chord-override tag DENEMO_OVERRIDE_AFFIX)
											(d-DirectivePut-score-prefix tag "attachGraces = #(define-music-function (parser location frac note graces) (number? ly:music? ly:music?)
 (if (null? (ly:music-property graces 'element))
	 (begin
	  (ly:warning (_ \"No grace notes follow this note the afterGrace will be ignored.\"))
	  (make-music
		  'SequentialMusic 
		  'elements
		  (list note graces)))
	  ;(make-music 'Music 'void #t))
	 (let ((skip (* frac (/ (ly:moment-main-numerator (ly:duration-length (ly:music-property note 'duration)))
		  (ly:moment-main-denominator (ly:duration-length (ly:music-property note 'duration))))))
	   (thegraces  (ly:music-property (list-ref (ly:music-property 
					 (ly:music-property graces 'element) 'elements) 1) 'elements)))
	(define gracenotes (make-music 'GraceMusic 'element (make-music
			'SequentialMusic
			'elements
			thegraces)))
	   (make-music
		  'SimultaneousMusic 
		  'elements
		  (list
		 note 
		(make-music
		 'SequentialMusic
		  'elements
		  (list
			 (make-music
				  'SkipMusic
				  'duration
				  (ly:make-duration 0 0 skip))
				gracenotes)))))))")
											(d-DirectivePut-score-override tag DENEMO_OVERRIDE_AFFIX)
											(d-PopPosition)
											(d-SetSaved #f)))))
									(d-WarningDialog (_ "Not a fraction of the main note duration"))))
								(d-WarningDialog (_ "Cancelled")))))
							(d-WarningDialog (_ "Cursor not on Grace Note")))))
