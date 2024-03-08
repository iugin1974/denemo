;;;PianoStaffName
(let ((tag "PianoStaffStart") (name "Piano")(shortName "Pno"))
	(if (d-DirectiveGet-staff-prefix tag)
		(begin
		    (set! name (d-GetUserInput (_ "Instrument Name") (_ "Give name of instrument for staff group starting here:") name))
		    (set! shortName (d-GetUserInput (_ "Short Instrument Name") (_ "Give short name of instrument for staff group starting here:") shortName))
		    (if (not shortName)
		    			(set! shortName ""))
		    (if name
		    	(set! name (string-append "\\set PianoStaff.instrumentName = #\"" name "\" " "\\set PianoStaff.shortInstrumentName = #\"" shortName "\" "))
			(set! name ""))
		    (AttachDirective "staff"  "prefix"  "PianoStaffStart" (string-append " \\new PianoStaff << " name " \n") DENEMO_OVERRIDE_GRAPHIC DENEMO_OVERRIDE_AFFIX))
		  (begin
		  	(d-WarningDialog "This command must be issued on the first staff of a piano staff group"))))
