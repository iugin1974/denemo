;;; CustomRehearsalMark
(let ((value CustomRehearsalMark::params) (tag "CustomRehearsalMark"))
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
        (let ((text (cdr value))(data (car value))(position (RadioBoxMenu (cons (_ "left") "left") (cons (_ "center") "center") (cons (_ "right") "right")))
        (eof (RadioBoxMenu (cons (_ "Visible at end of line") " \\tweak break-visibility  #begin-of-line-invisible ")
			(cons (_ "Visible at start of line") " \\tweak break-visibility  #begin-of-line-visible ") (cons (_ "default") "")))
        
        ) 
         (if position
         	(begin
		  (d-Directive-standalone tag)
		  (d-DirectivePut-standalone-display tag data)
		  (d-DirectivePut-standalone-graphic tag "RehearsalMark")
		  (d-DirectivePut-standalone-gx tag 15)
		  (d-DirectivePut-standalone-postfix tag  (string-append  " \\once \\override Score.RehearsalMark.self-alignment-X = #" position eof " \\mark \\markup \\column {" text "}" ) )
		  (d-DirectivePut-standalone-grob  tag  "RehearsalMark")
		  (d-DirectivePut-standalone-minpixels  tag  30)
		  (d-DirectivePut-standalone-data tag (string-append "(list (cons  'value \""  (scheme-escape data) "\"))"))
		  (d-SetSaved #f)
		  (d-RefreshDisplay))))))

