;;;NumericTimeSig
(let ((tag "NumericTimeSig")(params NumericTimeSig::params))
	(if (not params)
		(begin
			(d-PushPosition)
			(d-GoToPosition #f 1 #f 1)))
  (if (d-Directive-timesig? tag)
    (d-DirectiveDelete-timesig tag)
    (begin
      (d-DirectivePut-timesig-prefix tag "\\numericTimeSignature\n")
      (d-DirectivePut-timesig-graphic tag "\nN\nDenemo\n12")
      (d-DirectivePut-timesig-gy tag 0)
      (d-DirectivePut-timesig-minpixels tag 30)))

  
  (while (MoveDownStaffOrVoice)
  	(d-NumericTimeSig 'recursive))
 (if (not params)
	(begin
		(d-PopPosition)
		(d-SetSaved #f)
		(d-RefreshDisplay))))
