;;;SetSystemCount
(let ((tag "SetSystemCount"))
	(if SetSystemCount::params
		(let ((count (d-GetUserInput (_ "Total Systems Count") (_ "Give Systems Required\n(0 for optimal): ") "12")))
				(if (not (d-Directive-score? tag))
					(begin
						(d-DirectivePut-score-prefix tag "")
						(d-DirectivePut-score-display tag (_ "System Count"))))
								
					(if (and count (string->number count)) 
						(begin
							(if  (> (string->number count) 0)
								(begin
									(d-DirectivePut-score-prefix tag (string-append "\\paper { system-count=" count "}"))
									(d-DirectivePut-score-display tag (string-append (_ ": System Count") count)))
								(begin
									(d-DirectivePut-score-display tag (_ "System Count (Optimal)"))
									(d-WarningDialog (_ "Optimal System Count Set"))))	
							(d-SetSaved #f))
						(d-WarningDialog (_ "Cancelled"))))
		(ConditionalValue "score" (cons "score" tag))))
