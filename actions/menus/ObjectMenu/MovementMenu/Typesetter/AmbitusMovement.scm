;;;AmbitusMovement
(let ((tag "AmbitusMovement"))
	(if (d-Directive-layout? tag)
		(d-DirectiveDelete-layout tag)
 		(d-DirectivePut-layout-postfix tag 
" \\context {
      \\Staff
      \\consists \"Ambitus_engraver\"
    }")))