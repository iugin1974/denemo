;;;tuplet numbering toggle
(let* ((tag "ToggleTupletNumbering")(current (d-DirectiveGet-standalone-data tag)))
  (d-Directive-standalone tag)
  (d-DirectivePut-standalone-minpixels tag 30)
  (if current
	(begin
		(d-DirectivePut-standalone-postfix tag "\\override TupletNumber #'transparent = ##f ")
		(d-DirectivePut-standalone-data tag "")
        (d-DirectivePut-standalone-graphic tag "\n3►\ndenemo\n24\n1\n1"))
      (begin
		(d-DirectivePut-standalone-postfix tag "\\override TupletNumber #'transparent = ##t ")		
		(d-DirectivePut-standalone-data tag "on")
		;(d-DirectivePut-standalone-gy tag 0)
		(d-DirectivePut-standalone-graphic tag "\nX3\ndenemo\n24\n1\n1"))))
(d-RefreshDisplay)
