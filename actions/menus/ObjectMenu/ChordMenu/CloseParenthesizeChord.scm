;;; CloseParenthesizeChord
(let ((tag "CloseParenthesizeChord"))
 (if (d-Directive-chord? tag)
 	(d-DirectiveDelete-chord  tag)
 	(begin
		(d-LilyPondDefinition (cons "endParenthesize"
		(if (d-CheckLilyVersion "2.24.0")
" \\once \\override Parentheses.stencils = #(lambda (grob)
        (let ((par-list (parentheses-interface::calc-parenthesis-stencils grob)))
          (list point-stencil (cadr par-list))))"			
" \\once \\override ParenthesesItem.stencils = #(lambda (grob)
        (let ((par-list (parentheses-item::calc-parenthesis-stencils grob)))
          (list point-stencil (cadr par-list))))")))
		(d-DirectivePut-chord-prefix tag "\\endParenthesize\\parenthesize ")
		(d-DirectivePut-chord-override tag DENEMO_OVERRIDE_AFFIX)
		(d-DirectivePut-chord-display tag ")")))
(d-SetSaved #f))
