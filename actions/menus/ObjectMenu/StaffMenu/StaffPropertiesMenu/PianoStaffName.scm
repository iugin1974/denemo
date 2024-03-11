;;;PianoStaffName
(let ((stag "PianoStaffStart") (tag "PianoStaffName")(name "Piano")(shortName "Pno"))
	(if (d-DirectiveGet-staff-prefix stag)
		(begin
		    (set! name (d-GetUserInput (_ "Instrument Name") (_ "Give name of instrument for staff group starting here:") name))
		    (set! shortName (d-GetUserInput (_ "Short Instrument Name") (_ "Give short name of instrument for staff group starting here:") shortName))
		    (if (not shortName)
		    			(set! shortName ""))
		    (if name
					(d-DirectivePut-voice-postfix tag (string-append "\\set PianoStaff.instrumentName = #\"" name 
							"\" " "\\set PianoStaff.shortInstrumentName = #\"" shortName "\" "))
					(d-WarningDialog (_ "Cancelled"))))
		(d-WarningDialog "This command must be issued on the first staff of a piano staff group")))
