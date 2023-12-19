;;;MovementRaggedLast
(let ((tag "MovementRaggedLast"))
  (if (d-Directive-layout? tag)
    (d-DirectiveDelete-layout tag)
    (begin
      (d-DirectivePut-layout-postfix tag "\nragged-last = ##t")
      (d-DirectivePut-layout-override tag DENEMO_OVERRIDE_GRAPHIC) ) )
  (d-SetSaved #f))