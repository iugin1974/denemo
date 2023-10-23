;;;TweakSpacing
(let* ((tag "TweakSpacing")(ttag (d-DirectiveGetForTag-standalone))
		(padding "6")(offsetx "0")(offsety "-4"))
	
		(let ((data (d-DirectiveGet-standalone-data tag)))
			(if data
				(begin
					(set! data (eval-string data))
					(set! padding (list-ref data 0))
					(set! offsety (list-ref data 2))))
			(set! padding (d-GetUserInput (_ "Padding") (_ "Give padding amount") padding))
			(if padding
				(set! offsety (d-GetUserInput (_ "Vertical Shift") (_ "Give vertical adjustment") offsety)))
			(if (and padding offsety (string->number padding) (string->number offsety))
				(let ((newdata (string-append "(list \"" padding "\" \"" offsetx "\" \"" offsety "\")"))
					(setting (string-append "<>-\\tweak padding #" padding "  \\tweak extra-offset #'(" offsetx " . " offsety ") ")))
					(if data
						(begin
							(d-DirectivePut-standalone-postfix tag setting)
							(d-DirectivePut-standalone-data tag newdata))
						(begin
							(StandAloneDirectiveProto (cons tag setting)
								#f  "\n⬍\nDenemo\n20" 
								(_ "Tweak Spacing") #f newdata)
							(d-MoveCursorRight))))
				(d-WarningDialog (_ "Cancelled")))))
