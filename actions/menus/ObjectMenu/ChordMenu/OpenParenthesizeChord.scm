;;; OpenParenthesizeChord
(let ((tag "OpenParenthesizeChord"))
 (if (d-Directive-chord? tag)
    (d-DirectiveDelete-chord  tag)
    (begin
        (d-LilyPondDefinition (cons "startParenthesize" 
		(if (d-CheckLilyVersion "2.24.0")
" \\once \\override Parentheses.stencils = #(lambda (grob)
        (let ((par-list (parentheses-interface::calc-parenthesis-stencils grob)))
          (list (car par-list) point-stencil)))"        
" \\once \\override ParenthesesItem.stencils = #(lambda (grob)
        (let ((par-list (parentheses-item::calc-parenthesis-stencils grob)))
          (list (car par-list) point-stencil )))")))
        (d-DirectivePut-chord-prefix tag "\\startParenthesize\\parenthesize ")
        (d-DirectivePut-chord-override tag DENEMO_OVERRIDE_AFFIX)
        (d-DirectivePut-chord-display tag "(")))
(d-SetSaved #f))
