;;;CustomTitles
;;; Custom Titles Add lines of text at the top of the score. Each line can have some text at the left, center and right, and the text iteself can use any LilyPond markup commands.
(let ((tag "CustomTitles") (tags (list (cons (_ "Finish") #f))) (FTLtag "FilledTitleLine")(num 1))
	(define* (edit-line #:optional (new #f))
		(if (not new)
			(CopyDirective "score" tag FTLtag))
		(set! tag (string-append tag "\n" (number->string num)))
		(d-FilledTitleLine)
		(if (d-Directive-score? FTLtag)
				(begin
					(CopyDirective "score" FTLtag tag)
					(d-DirectivePut-score-display tag (string-append (_ "Title Line #") (number->string num)))
					(d-DirectiveDelete-score FTLtag))))
	(let loop ()
		(define nexttag (string-append tag "\n" (number->string num)))
		(if (and (< num 10) (d-Directive-score? nexttag))
				(begin
					(set! tags (cons (cons (string-append (_ "Edit line ") (number->string num)) num) tags))
					(set! num (1+ num))
					(loop))))
	(set! num (1- num))
	(if (zero? num)
		(begin 
			(set! num 1)
			(edit-line 'new))
		(let ((choice (TitledRadioBoxMenuList (_ "Choose Line to edit or new line") (cons (cons (_ "New Title Line") 'new) tags))))
			(if (eq? choice 'new)
				(begin
					(set! num (1+ num))
					(edit-line)
					(d-CustomTitles))
				(if (number? choice)
					(begin
						(set! num choice)
						(edit-line)
						
						(d-CustomTitles))
					(d-InfoDialog (_ "Finished")))))))