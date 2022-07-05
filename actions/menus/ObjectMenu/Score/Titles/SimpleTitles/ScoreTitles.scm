;;;SetMovementTitles
(let ((tagline (d-DirectiveGet-scoreheader-display "ScoreTagline")))
	(if tagline
		(begin 
			(d-PutTextClipboard tagline)
			(d-DirectiveDelete-scoreheader "ScoreTagline")
			(d-WarningDialog (string-append (_ "You had set the score tagline:\n") 
					tagline 
					(_ "\nThis is now copied to the clipboard\nIf you wish to keep it set it by pasting it in Simple Titles")))))
	(DenemoSetTitles "ScoreTitles" #f #f))
