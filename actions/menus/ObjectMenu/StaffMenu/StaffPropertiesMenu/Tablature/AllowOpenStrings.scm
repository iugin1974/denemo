;;AllowOpenStrings
(let ((tag "AllowOpenStrings"))
			(d-Directive-standalone tag)
			(d-DirectivePut-standalone-postfix tag (string-append "\\set TabStaff.restrainOpenStrings=##f"))
			(d-DirectivePut-standalone-display tag "+0")
			(d-DirectivePut-standalone-minpixels tag 30)
			(d-SetSaved #f)(d-RefreshDisplay))