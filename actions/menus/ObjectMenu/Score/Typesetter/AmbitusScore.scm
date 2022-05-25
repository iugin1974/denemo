;;;AmbitusScore
(let ((tag "AmbitusScore"))
	(if (d-Directive-score? tag)
		(d-DirectiveDelete-score tag)
 		(d-DirectivePut-score-prefix tag 
"\\layout { \\context {  \\Voice   \\consists \"Ambitus_engraver\" }}")))