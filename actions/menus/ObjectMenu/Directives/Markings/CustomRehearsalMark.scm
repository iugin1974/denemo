;;; CustomRehearsalMark
(let ((value CustomRehearsalMark::params) (tag "CustomRehearsalMark")(grob "RehearsalMark")(newSyntax (d-CheckLilyVersion "2.24")))
    (if (equal? value "edit")
        (set! value #f))
    (if (and (not value) (d-Directive-standalone? tag))
        (begin
            (set! value (d-DirectiveGet-standalone-data tag))
            (if value 
                (begin
                    (set! value (eval-string value))
                    (set! value (assq-ref value 'value))))))
    (if (not value)
        (set! value "ral."))
    (set! value (d-GetUserInputWithSnippets (_ "Text or Custom Rehearsal Mark") (_ "Give text to use for Mark") value))
    (if value   ;in case the user pressed Escape do nothing  
      (if (and newSyntax (d-MakeChoice  (_ "yes") (_ "no") (_ "Allow multiple marks at one moment")))
        (let ((text (cdr value))
			(data (car value))
			(position (if newSyntax
								""
								(RadioBoxMenu (cons (_ "left") "LEFT") (cons (_ "center") "CENTER") (cons (_ "right") "RIGHT"))))
			(eof (RadioBoxMenu (cons (_ "Visible at end of line") 'end)
				(cons (_ "Visible at start of line") 'begin) (cons (_ "default") ""))) ) 
			 (if position
				(begin
					(if newSyntax
						(case eof
							((begin)
								(set! eof "\\textMark ")
								(set! grob "TextMark"))
							(else
								(set! eof "\\textEndMark")
								(set! grob "TextMark")))
						(begin
							(case eof
								((begin)
									(set! eof  " \\tweak break-visibility  #begin-of-line-visible \\mark "))
								((end)
									(set! eof  " \\tweak break-visibility  #begin-of-line-invisible \\mark"))
									(else 
										(set! eof "\\mark ")))
							(set! eof (string-append  " \\once \\override Score.RehearsalMark.self-alignment-X = #" position eof))))
					(d-Directive-standalone tag)
					(d-DirectivePut-standalone-display tag data)
					(d-DirectivePut-standalone-graphic tag "RehearsalMark")
					(d-DirectivePut-standalone-gx tag 15)
					(d-DirectivePut-standalone-postfix tag  (string-append  eof "\\markup \\column {" text "}" ) )
					(d-DirectivePut-standalone-grob  tag grob)
					(d-DirectivePut-standalone-minpixels  tag  30)
					(d-DirectivePut-standalone-data tag (string-append "(list (cons  'value \""  (scheme-escape data) "\"))"))
					(d-SetSaved #f)
					(d-RefreshDisplay))))
			(let ((text (cdr value))(data (car value))(position (RadioBoxMenu (cons (_ "left") "left") (cons (_ "center") "center") (cons (_ "right") "right")))
				 (eof (RadioBoxMenu (cons (_ "Visible anywhere") "") (cons (_ "Visible at end of line") " \\tweak break-visibility  #begin-of-line-invisible ")
					(cons (_ "Visible at start of line") " \\tweak break-visibility  #begin-of-line-visible ") ))) 
				 (if position
					(begin
					  (d-Directive-standalone tag)
					  (d-DirectivePut-standalone-display tag data)
					  (d-DirectivePut-standalone-graphic tag "RehearsalMark")
					  (d-DirectivePut-standalone-gx tag 15)
					  (d-DirectivePut-standalone-postfix tag  (string-append  " \\once \\override Score.RehearsalMark.self-alignment-X = #" position eof " \\mark \\markup \\column {" text "}" ) )
					  (d-DirectivePut-standalone-grob  tag  "RehearsalMark")
					  (d-DirectivePut-standalone-minpixels  tag  30)
					  (d-DirectivePut-standalone-data tag (string-append "(list (cons  'value \""  (scheme-escape data) "\"))"))))))))	
