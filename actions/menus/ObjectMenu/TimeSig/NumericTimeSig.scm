;;;NumericTimeSig
(let ((tag "NumericTimeSig"))
	(if (d-Directive-timesig? tag)
		(d-DirectiveDelete-timesig tag)
		(begin
		  (d-DirectivePut-timesig-prefix tag "\\numericTimeSignature\n")
		  (d-DirectivePut-timesig-graphic tag "\nN\nDenemo\n12")
		  (d-DirectivePut-timesig-gy tag 0)
		  (d-DirectivePut-timesig-minpixels tag 30)))
		  
	(if (d-Directive-timesig? tag)
		(if (Timesignature?)
			(d-WarningDialog (_ "Time signatures will typeset as numeric from this moment in this movement"))
			(d-WarningDialog (_ "Time signatures will typeset as numeric in this movement")))
		(d-WarningDialog (_ "Removed numeric time signatures directive"))))
